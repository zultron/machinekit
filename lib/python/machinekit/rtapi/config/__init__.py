from item import ConfigItemLoader
from store import ConfigStoreLoader
from exceptions import *
import logging

class Config(object):
    """
    The Machinekit configuration management class

    The Config object loads and indexes ConfigItem plugins at init
    time.  When the Config object is given a get/set for a key, it
    retrieves the ConfigItem associated with that key from the index
    and passes the get/set operation to it.

    The Config object also loads ConfigStore plugins at init time,
    which register themselves with the ConfigItems they know how to
    handle.  When a ConfigItem is given a get/set operation, it goes
    through its register of ConfigStore plugins in priority order
    until it finds one able to perform the operation.
    """

    def __init__(self,
                 store_config={}, enabled_stores=None, disabled_stores=None,
                 item_config={}):
        self.log = logging.getLogger(self.__module__)

        # Configuration to be passed to plugins
        self.store_config = store_config
        self.item_config = item_config

        # ConfigItem object index
        self.index = {}

        self.log.debug("Finding and loading config item plugins")
        # ConfigItems add themselves to our index
        self.items = ConfigItemLoader(self)

        self.log.debug("Finding and loading config store backend plugins")
        # ConfigStores register themselves with items
        self.stores = ConfigStoreLoader(self,
                                        enabled_plugins=enabled_stores,
                                        disabled_plugins=disabled_stores)

        self.log.debug("Machinekit configuration initialized and ready")

    def index_add(self,item):
        """Index ConfigItem object"""
        if self.index.has_key(item.name):
            raise RTAPIConfigException(
                "ConfigItem plugin name duplicate:  %s" % item.name)
        self.index[item.name] = item

    def index_lookup(self, name):
        """Retrieve ConfigItem from index"""
        try:
            return self.index[name]
        except KeyError:
            raise RTAPIConfigNotFoundException(
                "No such config item key '%s'" % name)

    def __iter__(self):
        """Iterate through each ConfigItem in index"""
        return iter(self.index.values())

    def get(self, name):
        """
        Get item value
        """
        return self.index_lookup(name).get()

    def set(self, name, value):
        """
        Set item value
        """
        return self.index_lookup(name).set(value)

    def dump(self):
        self.log.info("Dumping configuration")
        # Print header
        print "%-30s = %-30s  %s" % \
            ('%s/%s' % ('section', 'name'), 'value', 'store')
        print "%-30s   %-30s  %s" % \
            ('-'*30, '-'*30, '-'*30)
        # Print lines
        for item in self:
            val = str(item.get())
            if len(val) > 30:
                val = '...%s' % val[-27:]
            print "%-30s = %-30s  %s" % \
                ('%s/%s' % (item.section, item.name), val,
                 item.store(default="(default)"))
