import sys
from masterd.worker import Worker
from masterd.service.control_interface import ControlInterfaceService
from masterd.service.config import ConfigService
from masterd.service.signal_fd import SignalFDService
from masterd.coprocess.rtapi_app import RTAPIApp
from masterd.daemon import Daemon

import logging
logging.basicConfig(level=logging.DEBUG)


class MasterD(Daemon):
    log = logging.getLogger(__name__)

    def __init__(self):
        self.log.info("Initializing master daemon")

        # Init services
        control_interface = ControlInterfaceService.server()
        signal_handler = SignalFDService.server()
        config_interface = ConfigService.server()

        # Init coprocesses
        rtapi_app = RTAPIApp()

        # Services run in Worker IOLoop
        self.worker = Worker(
            servers=[
                control_interface,
                signal_handler,
                config_interface,
                ],
            coprocesses=[
                rtapi_app,
                ],
            )

    def run(self):
        self.log.info("Daemonizing")
        self.daemonize()

        self.log.info("Starting worker loop")
        self.worker.run()

        self.log.info("Worker loop finished; exiting")


if __name__ == "__main__":
    masterd = MasterD()

    masterd.run()

    sys.exit(0)
