import os, subprocess

class test_module():
    """
    This class's methods start and stop the realtime environment.

    Tests that need RT should depend on the "realtime_start" group to
    ensure RT is started before they run, and should also belong to
    the "all_rt_tests" group to ensure they run before RT is stopped.

    These two dependencies should be taken care of in "run_tests.py".
    """

    DEBUG = "5"

    def setup_package(self):
        """
        Start RT environment
        """
        e=os.environ.copy()
        e["DEBUG"] = self.DEBUG
        subprocess.call(["realtime","restart"],
                        stderr=subprocess.STDOUT, env=e)

    def teardown_package(self):
        """Stop realtime"""
        e=os.environ.copy()
        e["DEBUG"] = self.DEBUG
        subprocess.call(["realtime","stop"],
                        stderr=subprocess.STDOUT, env=e)


DEBUG = "5"

def setup_package():
    """
    Start RT environment
    """
    e=os.environ.copy()
    e["DEBUG"] = DEBUG
    subprocess.call(["realtime","restart"],
                    stderr=subprocess.STDOUT, env=e)

def teardown_package():
    """Stop realtime"""
    e=os.environ.copy()
    e["DEBUG"] = DEBUG
    subprocess.call(["realtime","stop"],
                    stderr=subprocess.STDOUT, env=e)
