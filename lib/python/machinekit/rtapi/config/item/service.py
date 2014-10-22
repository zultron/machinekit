from machinekit.rtapi.config.item import ConfigString, ConfigInt, ConfigBool

class service_uuid(ConfigString):
    name = 'mkuuid'
    section = 'service'
    longopt = 'service_uuid'
    shortopt = 'R'
    help = "RTAPI instance service UUID"
    description = """
All network-accessible services of a running Machinekit instance are
identified by a unique id; see 'man uuidgen' Clients browsing zeroconf
for services belonging to a particular instance use this MKUUID value
as a unique key.

All MKUUID's must be different, so if there are several Machinekit
instances running on a LAN, there might be collisions hence, change
this UUID by using the output of 'uuidgen':
"""

class service_remote(ConfigBool):
    name = 'remote'
    section = 'service'
    default = False
    help = "Enable remote operation"
    description = """
Enable remote service access.  Default is local access only; set to true
for enabling remote operation regardless of 'interfaces' setting.

When enabled, zeroMQ sockets will use TCP on the preferred interface
listed in the 'interfaces' setting.

When disabled, zeroMQ will use IPC sockets in
RUNDIR/<rtapi_instance>.<service>.<uuid>, and zeroconf announcement
will be disabled.
"""

class service_interfaces(ConfigString):
    name = 'interfaces'
    section = 'service'
    default = 'eth wlan usb'
    help = "Network interface list"
    description = """
By default, the 'remote' setting is disabled, and services are bound
to Unix IPC sockets, so the service cannot be reached from the
network.

If services should be remotely accessible, the 'remote' setting should
be enabled, and a primary interface can be chosen by giving a list of
preferred interfaces or interface prefixes:  the first IPv4 address of
the first matching interface is used to bind(2).

The default setting is 'eth wlan usb':  when binding to a network
interface, prefer to bind to ethX; then wlanX if ethX is not present;
then usbX.
"""

