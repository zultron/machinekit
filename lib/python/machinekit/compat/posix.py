from machinekit.compat import \
    RTAPIFlavorUserland
import os

class RTAPIFlavorPOSIX(RTAPIFlavorUserland):
    """
    POSIX userland (non-real-time) RTAPI threads flavor
    """
    name = "posix"
    id = 0
    prio=90                     # lowest prio
    does_io = False
