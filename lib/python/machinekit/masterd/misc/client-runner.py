import time
import sys
from masterd.service.control_interface import ControlInterfaceService
from masterd.service.config import ConfigService

import logging
logging.basicConfig(level=logging.DEBUG)


def config_client():
    client = ConfigService.client()
    lookups = 2
    print "  %s, trying %d times" % (client.transport, lookups)
    for r in range(lookups):
        # send a couple of things
        print "get(name='foo')..."
        reply = client.get(name='foo')
        print "...%s" % reply
        time.sleep(1)
        print "set(name='foo', value_string='bar')..."
        client.set(name='foo', value_string='bar')
        print "...(ok)"
        time.sleep(1)
    

def control_client():
    client = ControlInterfaceService.client()
    pings = 4
    print "  %s pinging %d times" % (client.transport, pings)
    # serve a few requests and die
    for r in range(pings):
        print "ping..."
        client.ping()
        print "...ack"
        time.sleep(1)
    print "shutting down server..."
    client.shutdown()
    print "...done"
        
        
if __name__ == "__main__":
    try:
        print "running config_client again"
        config_client()
        print
        print "running control_client"
        control_client()
        print
        # print "running config_client; this will hang"
        # config_client()
    except KeyboardInterrupt:
        print "Received keyboard interrupt; exiting"

    sys.exit(0)
