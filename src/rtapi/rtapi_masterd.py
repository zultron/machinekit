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


    def prepare_environment(self):
        # Run sanity checks:
        #
        # Environment:  setuid, rlimits, running daemons
        self.env.assert_sanity()
        # SHM:  clean up leftover segments
        #   *** WARNING ***:  Do not run this before Environment sanity checks
        self.shm.assert_sanity()

        # set up shm
        self.shm.init_shmdrv()
        self.global_segment = self.shm.create_global_segment()

        # init global data
        self.global_data = GlobalData(self.global_segment, self.config)
        self.global_data.init_global_data()

        # FIXME  Do something with logs that they don't come out on stdout


    def daemonize(self):
        # fork into a background daemon, unless asked not to
        if not self.config.foreground:
            self.util.daemonize()

        # print some runtime data; rtapi_msgd.cc:935
        #  FIXME  This needs build data

        # if another masterd is registered in global data, bail
        self.global_data.assert_no_other_rtapi_masterd()

        # setup signals 959
        
        # setup zmq log publisher socket 974

        # set global data 'ready' 1049

    def fork_rtapi_app(self):
        # fire off rtapi_app; happens in realtime script after daemonize()

        # spin....

        pass

    def shutdown(self, exit_code=0):
        # stop rtapi_app

        # close down global data
        self.global_data.exit()

        # detach and unlink global shm segment
        try:
            self.shm.unlink_global_segment()
        except Exception as e:
            self.log.error(
                "rtapi_masterd instance %d:  Failed to shut down "
                "global shm segment:  %s" % (self.config.instance, e))
        else:
            self.log.info(
                "rtapi_masterd instance %d:  Global segment detached" %
                self.config.instance)

        # shutdown logging

        # cleanup

        # close log

        # exit
        sys.exit(exit_code)

    def sigaction_handler(self, sig):
        # this is only for bad problems?
        # Michael's:

        # log an error
        self.log.error(
            "rtapi_masterd instance %d: received signal %d; killing rtapi",
            self.config.instance, sig)

        # shut down global segment and kill rtapi_app
        if self.global_data.exists():
            self.global_data.exit()
            self.util.kill_rtapi_app(self.global_data)

        # print backtrace

        # close logs

        # reset handler

        # dump core

    def signalfd_handler(self, zloop, zpoller, *args):
        # read signalfd_siginfo
        # FIXME
        sig = zpoller.sig

        # shut down global segment and kill rtapi_app
        if self.global_data.exists():
            self.global_data.exit()
            self.util.kill_rtapi_app(self.global_data)

        if self.handler.is_user_termination_signal(sig):
            self.log.info(
                "rtapi_masterd instance %d:  terminated by user",
                self.config.instance)

            # Print stats about error ring buffer

            # Exit reactor normally
            return -1

        else:
            # This should have been handled either above or in
            # sigaction_handler
            self.log.error(
                "rtapi_masterd instance %d:  Unhandled signal %d", sig)
            # Continue reactor
            return 0


def trap_errors(function, error_message, args=()):
    """
    Run function (with optional args), returning result.

    If a known exception is raised, log the error_message and
    exception message and exit.
    """
    try:
        return function(*args)
    except rtapi_exceptions as e:
        logging.error(message)
        for line in str(e).split('\n'):
            logging.error("    ", line)
        sys.exit(1)


if __name__ == "__main__":
    # Initialization:
    #
    # Everything here is setup; nothing is touched
    #
    # - Set up config
    # - Initalize objects
    rtapi = trap_errors(
        RTAPIEnvironment,
        "Failed to start RTAPI environment")

    # Prepare RTAPI environment:
    #
    # Set up the pre-daemonization environment; any failure here can
    # be cleaned up by the next run
    #
    # - Run environment sanity checks
    # - Run shm sanity checks; clean up any messes
    # - Initialize shm
    # - Initialize global data segment
    trap_errors(
        rtapi.prepare_environment,
        "Failed to prepare RTAPI environment")

    # Daemonize:
    #
    # Become a daemon; prepare for rtapi_app launch; any failure here
    # can be cleaned up by the next run
    #
    # - Informational log messages
    # - Last safety checks
    # - Set up signal handlers
    # - Set up zmq services
    # - Mark global data as 'ready'
    trap_errors(
        rtapi.daemonize,
        "Failed to daemonize")

    # Run rtapi_app:
    #
    # Fork an rtapi_app and spin; anything that goes wrong here may
    # need manual cleanup
    trap_errors(
        rtapi.fork_rtapi_app,
        "Failed to run rtapi_app")

    # Shutdown:
    #
    # - Stop rtapi_app
    # - Close global data
    # - Unlink global shm segment
    # - Shut down zmq services
    # - Close logs
    # - Exit
    trap_errors(
        rtapi.shutdown,
        "Failed to shut down")
