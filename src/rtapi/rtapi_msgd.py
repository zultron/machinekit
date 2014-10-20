import sys, os
import argparse, ConfigParser

from machinekit import compat, shmcommon

class CLI(object):

    def __init__(self):
        self.getenv()
        self.init_args()
        self.read_ini()
        # Additional processing for individual args
        # These might be merged into argparse with some cleverness.
        self.get_uuid()
        self.get_remote()
        self.get_interfaces()
        self.get_flavor()
        # Sanity checks
        self.getuid()
        self.flavor_checks()
        self.get_shmdrv()

    def error(self, msg, exitval=1):
        sys.stderr.write("%s:  %s\n" % (sys.argv[0], msg))
        if exitval != 0:
            sys.exit(exitval)

    def getenv(self):
        self.env = os.environ.copy()

    def init_args(self):
        p = argparse.ArgumentParser(
            description='Machinekit messaging daemon')

        p.add_argument('--foregound', '-F', action="store_true",
                       help="Run in foreground")
        p.add_argument('--instance', '-I', type=int, default=0,
                       help="RTAPI instance number")
        p.add_argument('--hal-thread-stack-size', '-T', type=int,
                       help="Hal thread stack size")
        p.add_argument('--instance-name', '-i',
                       help="RTAPI instance name")
        p.add_argument('--interfaces', '-n',
                       help="Network interfaces to bind to")
        p.add_argument('--inifile', '-M',
                       default=self.env.get("MACHINEKIT_INI",
                                            "/etc/linuxcnc/machinekit.ini"),
                       help="Path to Machinekit .ini file")
        p.add_argument('--flavor', '-f',
                       help="RTAPI flavor, e.g. posix, xenomai, rt-preempt")
        p.add_argument('--user-msglevel', '-u', type=int,
                       help="ULAPI debug message level")
        p.add_argument('--rt-msglevel', '-r', type=int,
                       help="RTAPI debug message level")
        p.add_argument('--hal-size', '-H', type=int,
                       help="HAL size")
        p.add_argument('--use-shmdrv', '-S', action="store_true",
                       help="Use shmdrv (default for kernel thread flavors)")
        p.add_argument('--shmdrv-opts', '-o',
                       default=self.env.get("SHMDRV_OPTS", ""),
                       help="Options to pass to shmdrv module")
        p.add_argument('--log-stderr', '-s', action="store_true",
                       help="Log to stderr in addition to syslog")
        p.add_argument('--logpub-uri', '-U',
                       help="Logpub URI")
        p.add_argument('--service-uuid', '-R',
                       default=self.env.get("MKUUID",None),
                       help="RTAPI instance service UUID")

        self.opts = p.parse_args()

    def read_ini(self):
        self.config = ConfigParser.RawConfigParser()
        self.config.read(self.opts.inifile)
    
    def get_uuid(self):
        """Get service UUID from .ini file if not already set, and
        error out if no UUID found anywhere"""
        if self.opts.service_uuid is None:
            try:
                self.opts.service_uuid = self.config.get("MACHINEKIT","MKUUID")
            except ConfigParser.NoOptionError:
                pass  # next step will error out
        if self.opts.service_uuid is None:
            self.error(
                "Error:  No service UUID.  Please specify '-R SERVICE_UUID' "
                "or define in machinekit.ini file")

    def get_remote(self):
        """Get remote service access parameter from .ini file"""
        try:
            self.opts.remote = self.config.getint("MACHINEKIT","REMOTE")
        except ConfigParser.NoOptionError:
            self.opts.remote = 0

    def get_interfaces(self):
        """Get network interfaces parameter from .ini file and split
        into list"""
        if self.opts.interfaces is None:
            try:
                self.opts.interfaces = \
                    self.config.get("MACHINEKIT","INTERFACES")
            except ConfigParser.NoOptionError:
                self.opts.interfaces = "eth wlan usb"
        self.opts.interfaces = self.opts.interfaces.split()

    def get_flavor(self):
        # create a list of flavors for convenience
        self.flavors = []
        id = 0
        while True:
            try:
                self.flavors.append(compat.flavor_byid(id))
                id += 1
            except RuntimeError:
                break
            
        # set flavor from compat default routines unless -f option set
        if self.opts.flavor == None:
            self.flavor = compat.default_flavor()
            self.opts.flavor = self.flavor.name
        else:
            try:
                self.flavor = compat.flavor_byname(self.opts.flavor)
            except RuntimeError:
                self.error("Error:  failed to set flavor '%s'; "
                           "valid flavors:" % self.opts.flavor, 0)
                self.error("    %s\n"
                           % ' '.join([f.name for f in self.flavors]))

    def getuid(self):
        if os.getuid() == 0:
            self.error("Error:  Refusing to run as root")
        if os.geteuid() == 0:
            self.error("Error:  Refusing to run as setuid root")

    def flavor_checks(self):
        # FIXME: flavors need their own encapsulated sanity checks:
        #
        # - kernel compatibility
        # - xenomai gid
        # - stuff in scripts/check-system-configuration.sh
        pass

    def get_shmdrv(self):
        # FIXME:  related to flavor_checks?
        # Need to check whether the flavor needs shmdrv or not
        pass

    def start_shmdrv(self):
        if self.opts.use_shmdrv and not shmcommon.shmdrv_available():
            try:
                compat.run_module_helper(
                    "insert shmdrv %s" % self.opts.shmdrv_opts)
            except RuntimeError as e:
                self.error("Error:  Failed to insert shmdrv module:\n    %s" % e)
            shmcommon.shm_common_init()
            if not shmcommon.shmdrv_available():
                self.error("Error:  shmdrv module not detected; please "
                           "report this bug")


            

    def run(self):
        from pprint import pprint
        pprint(self.opts.__dict__)

        self.start_shmdrv()



if __name__ == "__main__":
    cli = CLI()
    cli.run()
