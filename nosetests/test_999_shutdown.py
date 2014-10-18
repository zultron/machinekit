from . import RTAPITestCase

import os, subprocess

class test_rtapi(RTAPITestCase):

    env = {
        "DEBUG" : "5",
        "MSGD_OPTS" : "-s",
    }

    def test_99999_stop_rt(self):
        """99999 rtapi:  Stop realtime environment"""
        e=os.environ.copy()
        e.update(self.env)
        subprocess.call(["realtime","stop"],
                        stderr=subprocess.STDOUT, env=e)
