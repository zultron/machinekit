import logging, os
import shmdrv_api, rtapi_bindings
from machinekit.rtapi.util import Util
from machinekit.rtapi import rtapi_common

from shmdrv_api import SHMDrvAPIRuntimeError
from machinekit.mk_config import HAL_SIZE

log = logging.getLogger(__name__)

class MKSHMSegment(shmdrv_api.SHMSegment):
    """
    This class is a wrapper around shmdrv_api.SHMSegment, allowing
    segments to be referenced by Machinekit names like 'global' or
    'rtapi' rather than a SHM key.  It also introduces logging
    facilities to SHM objects.
    """

    SHM_PREFIX = rtapi_bindings._SHM_PREFIX
    instance = 0
    requested_size = None
    magic = None
    log = logging.getLogger(__name__)

    @classmethod
    def posix_name_prefix(cls, instance = None):
        if instance is None:  instance = cls.instance
        return "%s" % (cls.SHM_PREFIX)

    @classmethod
    def init_shm(cls, instance=0, prefix=None):
        cls.instance = instance
        shmdrv_api.init()
        if prefix is None:
            # set prefix to default /linuxcnc-%(key)08x
            prefix = cls.posix_name_prefix()
        shmdrv_api.set_name_format(prefix)
        log.debug("Initialized shm: POSIX name prefix '%s', instance %d",
                  prefix, instance)

    def __init__(self, key = 0, size = 0, instance = None):
        # Set `instance` in object in case class instance is changed
        if instance is not None:
            self.instance = instance
        else:
            self.instance = self.__class__.instance
        # Set parent object key
        self.key = self.key_byname(self.instance, self)
        # If we're a subclass, we have size data
        if self.requested_size is not None:
            if size == 0:
                self.size = self.requested_size
            else:
                pass # self.size is set by SHMSegment.__cinit__

    @classmethod
    def key_byname(cls, instance, obj=None):
        if cls.magic is None: magic = obj.key
        else:  magic = cls.magic

        return ((magic & 0x00ffffff) |
                ((instance << 24) & 0xff000000))

    @property
    def name(self):
        '''Subclasses may override'''
        return "0x%08x" % self.key

    def new(self):
        super(MKSHMSegment, self).new()

        # be sure size is same as requested
        if self.requested_size and self.requested_size != self.size:
            raise RTAPISHMRuntimeError(
                "Segment '%s':  created size %d != requested size %d" %
                (self.name, self.size, self.requested_size))

        self.log.debug("Segment '%s': create new key=%08x, size=%d",
                       self.name, self.key, self.size)
        return self

    def attach(self):
        super(MKSHMSegment, self).attach()
        # be sure size is same as requested
        if self.requested_size and \
                self.requested_size != self.size:
            raise RTAPISHMRuntimeError(
                "Segment '%s':  attached size %d != requested size %d" %
                (self.name, self.size, self.attr['requested_size']))

        self.log.debug(
            "Segment '%s':  attached existing key=%08x, size=%d",
            self.name, self.key, self.size)
        return self

    def detach(self):
        super(MKSHMSegment, self).detach()

        self.log.debug("Segment '%s':  detached key=%08x",
                       self.name, self.key)
        return self

    def unlink(self):
        super(MKSHMSegment, self).unlink()

        self.log.debug("Segment '%s':  unlinked key=%08x",
                       self.name, self.key)
        return self


class GlobalSegment(MKSHMSegment):
    requested_size = rtapi_bindings.global_data_size()
    magic = rtapi_bindings._GLOBAL_KEY
    name = "global"

class RTAPISegment(MKSHMSegment):
    requested_size = rtapi_common.RTAPI_DATA_SIZE
    magic = rtapi_bindings._RTAPI_KEY
    name = "rtapi"

class HALSegment(MKSHMSegment):
    requested_size = HAL_SIZE
    magic = rtapi_bindings._HAL_KEY
    name = "hal"



class SHMOps(object):

    all_seg_classes = [
        GlobalSegment,
        RTAPISegment,
        HALSegment,
        ]

    def __init__(self, config=None):
        if config is None:
            raise RTAPISHMRuntimeError("SHM object needs rtapi.config")
        self.log = logging.getLogger(self.__module__)
        self.config = config
        self.util = Util(config=self.config)

        # initialize right here
        MKSHMSegment.init_shm(self.config.instance)

    @property
    def shmdrv_available(self, instance=0):
        return shmdrv_api.shmdrv_available()

    def init_shm(self, instance=0):
        MKSHMSegment.init_shm(instance=instance)

    def init_shmdrv(self, instance=None):
        if not self.config.use_shmdrv:
            return  # shmdrv not needed
        if self.shmdrv_available:
            return  # shmdrv already initialized

        # set up shmdrv
        self.util.insert_module("shmdrv", self.config.shmdrv_opts)
        self.init_shm(instance=instance)
        if not shmdrv_api.shmdrv_available():
            raise RTAPISHMRuntimeError(
                "Shmdrv module not detected; please report this bug")

    def any_segment_exists(self, instance=None):
        for cls in self.all_seg_classes:
            if cls(instance=instance).exists():
                return True
        return False

    def assert_segment_sanity(self, instance=None):
        """
        Sanity check: Be sure that any loaded shm segs are in a state
        they can be cleaned up.  This should only be run after
        Environment.assert_sanity() to avoid cleaning up SHM segments
        from under a running instance.
        """
        # If no shm segments exist, we're sane.
        if not self.any_segment_exists():
            return

        # Otherwise, clean up left-over shm segments.
        if self.config.use_shmdrv:
            self.cleanup_shmdrv()
        else:
            self.cleanup_shm_posix(barf=False)

        # If leftovers still exist after cleanup, raise exception.
        for cls in self.all_seg_classes:
            if cls(instance=instance).exists():
                raise RTAPISHMRuntimeError(
                    "Unable to cleanup conflicting %s segment, key=%s" %
                    (cls.__name__, cls(instance=instance).key))
            
    def assert_sanity(self, instance=None):
        """
        Sanity checks
        
        *** WARNING *** This should only be run after
        Environment.assert_sanity() to avoid cleaning up SHM segments
        from under a running instance.
        """
        self.assert_segment_sanity(instance=instance)

    def create_global_segment(self, instance=None):
        # Create the global segment.
        #
        # At this point, all environment sanity checks must have been
        # performed.

        global_seg = GlobalSegment(instance=instance)
        global_seg.new()
        return global_seg
        
    def global_segment_exists(self, instance=None):
        # boolean:  Does the shm segment still exist?
        return GlobalSegment(instance=instance).exists()
        
    def unlink_global_segment(self, instance=None):
        # Unlink the global segment.
        #
        # At this point, all global data shutdown activities must have
        # been performed.

        global_seg = GlobalSegment(instance=instance)
        global_seg.unlink()

    def cleanup_shmdrv(self):
        shmdrv_api.shmdrv_gc()

    def cleanup_shm_posix(self, instance=None, barf=True):
        """
        Clean up 'global', 'hal' and 'rtapi' shm segments.  When barf
        is True, an exception will be raised if any segments are found
        to be already unlinked.
        """
        # clean up keys in hal, rtapi, global order
        for cls in reversed(self.all_seg_classes):
            seg = cls(instance=instance)
            if not seg.exists():
                if not barf:  continue
                raise RTAPISHMRuntimeError(
                    "Unable to cleanup non-existent %s segment, key=%s" %
                    (cls.__name__, seg.key))

            self.log.warn("Removing unused %s shm segment %s",
                          cls.__name__, seg.posix_name)
            seg.unlink()


class RTAPISHMRuntimeError(RuntimeError):
    """
    Thrown for problems in RTAPI SHM
    """
    pass

class RTAPISHMPermissionError(RTAPISHMRuntimeError):
    """
    Thrown for permission errors in RTAPI SHM
    """
    pass
