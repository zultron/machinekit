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
    _instance = 0
    all_seg_names = ('global', 'rtapi', 'hal')

    attsdict = {
        "rtapi" : {
            "magic"         : rtapi_bindings._RTAPI_KEY,
            "requested_size" : rtapi_common.RTAPI_DATA_SIZE,
            },
        "hal" : {
            "magic"          : rtapi_bindings._HAL_KEY,
            "requested_size" : HAL_SIZE
            },
        "global" : {
            "magic"          : rtapi_bindings._GLOBAL_KEY,
            "requested_size" : rtapi_bindings.global_data_size(),
            },
        }

    @classmethod
    def posix_name_prefix(cls, instance = None):
        if instance is None:  instance = cls._instance
        return "%s" % (cls.SHM_PREFIX)

    @classmethod
    def init_shm(cls, instance=0, prefix=None):
        cls._instance = instance
        shmdrv_api.init()
        if prefix is None:
            # set prefix to default /linuxcnc-%(key)08x
            prefix = cls.posix_name_prefix()
        shmdrv_api.set_name_format(prefix)
        log.debug("Initialized shm: POSIX name prefix '%s', instance %d",
                  prefix, instance)

    @classmethod
    def all_key_names(cls):
        return self.atts.keys()

    class exists_descr(object):
        """
        A descriptor class for a unified exists() instance and class
        method
        """
        def __get__(self, obj, objtype=None):
            if obj is not None:
                return lambda: shmdrv_api.exists(
                    obj.key_byname(obj.name, obj._instance))
            else:
                return lambda name, instance=None: shmdrv_api.exists(
                    objtype.key_byname(
                        name, (instance,objtype._instance)[instance is None]))
    exists = exists_descr()

    class instance_descr(object):
        def __get__(self, obj, type=None):
            if obj is not None:
                return obj._instance
            return type._instance
        def __set__(self, obj, value):
            obj._instance = value
    instance = instance_descr()

    def __init__(self, name):
        self.name = name
        # Don't want obj instance to change if class instance is changed
        self.instance = self.__class__._instance
        self.log = logging.getLogger(self.__module__)

    @property
    def attr(self):
        return self.attsdict[self.name]

    @classmethod
    def key_byname(cls, name, instance):
        # Usable as either object or class method
        return ((cls.attsdict[name]['magic'] & 0x00ffffff) |
                ((instance << 24) & 0xff000000))

    @property
    def key(self):
        return self.key_byname(self.name, self._instance)

    @property
    def requested_size(self):
        return self.attr.get('requested_size',None)

    def new(self, requested_size=None):
        requested_size = self.attr.setdefault('requested_size',requested_size)
        if requested_size is None:
            raise RTAPISHMRuntimeError(
                "Unknown requested size creating shm seg %s" % self.name)
        super(MKSHMSegment, self).new(self.key, requested_size)
        # be sure size is same as requested
        if requested_size != self.size:
            raise RTAPISHMRuntimeError(
                "Segment %s:  created size %d != requested size %d" %
                (self.name,self.size,requested_size))

        self.log.debug("Created new %s shm segment: key=%08x, size=%d",
                       self.key, self.size)
        return self

    def attach(self):
        super(MKSHMSegment, self).attach(self.key)
        # be sure size is same as requested
        if self.attr['requested_size'] and \
                self.attr['requested_size'] != self.size:
            raise RTAPISHMRuntimeError(
                "Segment %s:  attached size %d != requested size %d" %
                (self.name,self.size,self.attr['requested_size']))

        self.log.debug(
            "Attached existing %s shm segment: key=%08x, size=%d",
            self.key, self.size)
        return self

    def detach(self):
        super(MKSHMSegment, self).detach()

        self.log.debug("Detached %s shm segment: key=%08x", self.key)
        return self

    def unlink(self):
        super(MKSHMSegment, self).unlink()

        self.log.debug("Unlinked %s shm segment: key=%08x", self.key)
        return self


class SHMOps(object):

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
        for name in MKSHMSegment.all_seg_names:
            if MKSHMSegment.exists(name, instance):
                return True
        return False

    def assert_segment_sanity(self):
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
        for name in MKSHMSegment.all_seg_names:
            if MKSHMSegment.exists(name):
                raise RTAPISHMRuntimeError(
                    "Unable to cleanup conflicting %s segment, key=%s" %
                    (name, MKSHMSegment(name).key))
            
    def assert_sanity(self):
        """
        Sanity checks
        
        *** WARNING *** This should only be run after
        Environment.assert_sanity() to avoid cleaning up SHM segments
        from under a running instance.
        """
        self.assert_segment_sanity()

    def create_global_segment(self):
        # Create the global segment.
        #
        # At this point, all environment sanity checks must have been
        # performed.

        global_seg = MKSHMSegment('global')
        global_seg.new()
        return global_seg
        

    def cleanup_shmdrv(self):
        shmdrv_api.shmdrv_gc()

    def cleanup_shm_posix(self, barf=True):
        """
        Clean up 'global', 'hal' and 'rtapi' shm segments.  When barf
        is True, an exception will be raised if any segments are found
        to be already unlinked.
        """
        # clean up keys in hal, rtapi, global order
        for name in reversed(MKSHMSegment.all_seg_names):
            seg = MKSHMSegment(name)
            if not seg.exists():
                if not barf:  continue
                raise RTAPISHMRuntimeError(
                    "Unable to cleanup non-existent %s segment, key=%s" %
                    (name, seg.key))

            seg.attach()
            self.log.warn("Removing unused %s shm segment %s",
                          name, seg.posix_name)
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
