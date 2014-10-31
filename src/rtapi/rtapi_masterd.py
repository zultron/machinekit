#  RTAPI environment deamon
# 
#  This daemon is responsible for setting up the real-time
#  environment, independent of thread flavor.
#
#  Functions, in approximate order of execution:
#
#  - Import config (this does a whole stack of stuff)
#  - Run sanity checks
#  - Load shmdrv, if applicable
#  - Initialize logging
#  - Load global segment
#  - Start ZMQ log channels
#  
#  This daemon should start up rtapi_app, giving it a log channel
#
#  Then get rid of syslog_async
#
#  polls the rtapi message ring in the global data segment and
#  eventually logs them this is the single place for RTAPI and any
#  ULAPI processes where log messages pass through, regardless of
#  origin or thread style (kernel, rtapi_app, ULAPI) * doubles as
#  zeroMQ PUBLISH server making messages available to any interested
#  subscribers the PUBLISH/SUBSCRIBE pattern will also fix the current
#  situation where an error message consumed by an entity is not seen
#  by any other entities
# 
# 
#  Copyright (C) 2014 John Morris <john@zultron.com>
#  Based on work:
#  Copyright (C) 2012, 2013  Michael Haberler <license AT mah DOT priv DOT at>
# 
#  This program is free software; you can redistribute it and/or
#  modify it under the terms of the GNU Lesser General Public License
#  as published by the Free Software Foundation; either version 2.1 of
#  the License, or (at your option) any later version.
# 
#  This program is distributed in the hope that it will be useful, but
#  WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
#  Lesser General Public License for more details.
# 
#  You should have received a copy of the GNU Lesser General Public
#  License along with this library; if not, write to the Free Software
#  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
#  02110-1301 USA


import sys, logging

from machinekit.rtapi import *

logging.basicConfig(level=logging.DEBUG)


class RTAPIEnvironment(object):


    def __init__(self):
        self.log = logging.getLogger(self.__module__)

        # Set up objects
        self.config = Config()
        self.compat = Compat(config=self.config)
        self.util = Util(config=self.config)
        self.env = Environment(config=self.config,
                               compat=self.compat,
                               util=self.util)
        self.shm = SHMOps(config=self.config)

        # Run sanity checks:
        #
        # Environment:  setuid, rlimits, running daemons
        self.env.assert_sanity()
        # SHM:  clean up leftover segments
        #   *** WARNING ***:  Do not run this before Environment sanity checks
        self.shm.assert_sanity()

    def run(self):
        # set up shm
        self.shm.init_shmdrv()
        self.global_segment = self.shm.create_global_segment()

        # fork
        if not self.config.foreground:
            self.util.daemonize()

        # init global data
        self.global_data = GlobalData(self.global_segment, self.config)
        self.global_data.init_global_data()

        # print some runtime data; rtapi_msgd.cc:935

        # check global seg for another msgd pid

        # setup signals 959

        # setup zmq log publisher socket 974

        # fix up global data

        # [ fire off rtapi_app ]

        # spin....

        # shutdown logging

        # cleanup

        # close log

        # exit

if __name__ == "__main__":
    try:
        rtapi = RTAPIEnvironment()
    except RTAPIEnvironmentInitError as e:
        logging.error("Failed to start RTAPI environment:")
        for line in str(e).split('\n'):
            logging.error("    %s" % line)
        sys.exit(1)

    rtapi.run()
