from compat_bindings import *
import os, imp, platform

class RTAPIFlavorKernelException(RuntimeError):
    """
    Raised by flavor sanity checks when kernel doesn't match requested
    flavor
    """
    pass

class RTAPIFlavorPrivilegeException(RuntimeError):
    """
    Raised by flavor sanity checks when user has insufficient
    privileges to access real-time environment
    """
    pass

class RTAPIFlavorULimitException(RuntimeError):
    """
    Raised by flavor sanity checks when system resource limits are
    insufficient
    """
    pass

class RTAPIFlavor(object):
    """
    Abstract class representing an RTAPI flavor.
    """

    # Attributes to be customized for each flavor
    name = None                 # String:  flavor name
    id = None                   # uint
    mod_ext = None              # ".so" or ".ko"
    build_sys = None            # "user-dso" or "kbuild"
    so_ext = ".so"              # always ".so"
    prio = 99                   # 0-99, 0 is most preferred
    # flags
    does_io = None              # Bool:  userland: whether iopl() needs
                                # to be called
    rtapi_data_in_shm = None    # Bool: flavor keeps rtapi_data in a
                                # shm segment?

    def __init__(self):
        pass

    @property
    def assert_pre_runtime_environment_sanity(self):
        """
        Check the pre-runtime environment (before modules have been
        loaded, etc.) for problems specific to this thread flavor;
        flavor should raise appropriate exceptions (from this class or
        others) on fatal errors
        """
        pass

    @property
    def assert_runtime_environment_sanity(self):
        """
        Check the runtime environment (after modules have been loaded,
        etc.) for problems specific to this thread flavor; flavor
        should raise appropriate exceptions (from this class or
        others) on fatal errors
        """
        pass

class RTAPIFlavorUserland(RTAPIFlavor):
    """
    Abstract class representing a userland threads RTAPI flavor.
    """

    build_sys = "user-dso"
    does_io = True
    rtapi_data_in_shm = False

    
class RTAPIFlavorKernel(RTAPIFlavor):
    """
    Abstract class representing a kernel-space RTAPI flavor.
    """

    mod_ext = ".ko"
    build_sys = "kbuild"
    # flags
    does_io = True
    rtapi_data_in_shm = True

    # shmdrv device
    kthreads_shmdrv_dev = "/dev/shmdrv"

    @property
    def running_kernel_release(self):
        return platform.uname()[2]

    @property
    def kernel_source_dir(self):
        """Locate source directory for running kernel"""
        return "/lib/modules/%s/build" % self.running_kernel_release

    @property
    def assert_shmdrv_device_access(self):
        if not os.access(self.kthreads_shmdrv_dev, os.W_OK):
            raise RTAPIFlavorPrivilegeException(
                "Insufficient privileges to write to %s" %
                self.kthreads_shmdrv_dev)

    @property
    def assert_runtime_environment_sanity(self):
        """
        Assertions for kthreads:
        - User has write access to /dev/shmdrv

        Kthreads subclasses must include this assertion
        """
        self.assert_shmdrv_device_access


class Flavors(object):

    def __init__(self):
        self.load_flavors()

    def load_flavors(self):
        """
        A simple plugin system:

        Look in all *.py files in this directory and instantiate any
        subclass of RTAPIFlavor that is not abstract ('name' attribute
        is not None)

        File instances of those into data structures for easy
        retrieval by id/name/priority
        """
        self._byid = {}
        self._byname = {}
        self._list = []
        plugin_path = os.path.dirname(__file__)
        for i in os.listdir(plugin_path):
            if i == "__init__.py":
                continue # this module
            if not i.endswith(".py"):
                continue # skip shared binary objects & other stuff
            i = i.replace('.py','')
            info = imp.find_module(i, [plugin_path])
            m = imp.load_module(i, *info)
            for i in dir(m):
                attr = getattr(m,i)
                if type(attr) == type:
                    if (issubclass(attr, RTAPIFlavor) and \
                            attr.name is not None):
                        f = attr()
                        self._byid[f.id] = f
                        self._byname[f.name] = f
                        self._list.append(f)
        self.sort_flavors()

    def sort_flavors(self):
        """Sort list of flavors from hi to lo priority"""
        def prio_cmp(x,y):
            if x.prio > y.prio:
                return 1
            if y.prio > x.prio:
                return -1
            return 0

        self._list.sort(cmp=prio_cmp)

    @property
    def flavor_names(self):
        """Return list of flavor names"""
        return [ f.name for f in self._list ]

    def byid(self,id):
        """Return flavor by id"""
        return self._byid[id]

    def byname(self,name):
        """Return flavor by name"""
        return self._byname[name]

    def select_flavor(self, requested_flavor=os.getenv("FLAVOR")):
        """
        Select a flavor, in order of priority from the 'flavor'
        argument, the "FLAVOR" environment variable, or from the
        (prioritized) list of flavors.

        Run checks to be sure the flavor is valid.
        """

        if requested_flavor is not None:
            # "requested_flavor" argument or "FLAVOR" environment
            # variable set; check this flavor and set it, or bomb out
            # if any problems
            try:
                flavor = self.byname(requested_flavor)
            except KeyError:
                raise KeyError(
                    "Error:  Invalid requested flavor name '%s'; "
                    "valid flavors:  %s" %
                    (requested_flavor, ' '.join(self.flavor_names)))

            # Check that flavor matches runtime environment; this will
            # raise exceptions if not
            try:
                flavor.assert_pre_runtime_environment_sanity
            except Exception as e:
                raise type(e)("Error:  Requested flavor '%s' "
                              "fails sanity checks:  %s" %
                              (requested_flavor, e))

        # Choose the flavor with highest priority whose checks succeed
        saved_exception = None
        for flavor in self._list:
            # self._list already sorted, so just work down the list
            try:
                flavor.assert_pre_runtime_environment_sanity
                # If we're here, checks all passed
                return flavor
            except Exception as e:
                # In case the last-ditch flavor failed, capture a
                # useful error for the user
                saved_exception = e

        # No suitable flavor found; bomb out with the last failure
        raise type(saved_exception)(
            "Error:  No suitable flavor found; last flavor sanity check "
            "failure message:  %s" % e)
