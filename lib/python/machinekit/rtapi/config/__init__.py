from item import ConfigItemLoader
from store import ConfigStoreLoader
from exceptions import *
import logging

class Config(object):
    """
    The Machinekit configuration management class

    The Config object loads and indexes ConfigItem plugins at init
    time.  When the Config object is given a get/set for a
    (section,name) key, it retrieves the ConfigItem associated with
    that key from the index and passes the get/set operation to it.

    The Config object also loads ConfigStore plugins at init time,
    which register themselves with the ConfigItems they can handle.
    When a ConfigItem is given a get/set operation, it goes through
    its register of ConfigStore plugins in priority order until it
    finds one able to perform the operation.
    """

    def __init__(self):
        self.log = logging.getLogger(self.__module__)

        # Index for storing (section,name) config items
        self.index = {}

        self.log.debug("Finding and loading config item plugins")
        # ConfigItems add themselves to our index
        self.items = ConfigItemLoader(self)

        self.log.debug("Finding and loading config store backend plugins")
        # ConfigStores register themselves with items
        self.stores = ConfigStoreLoader(self)

        self.log.debug("Machinekit configuration initialized and ready")

    def index_add(self,item):
        """Add ConfigItem item index by (section,name) key"""
        self.index.setdefault(item.section,{})[item.name] = item

    def index_lookup(self, section, name):
        """Retrieve ConfigItem by (section,name) key"""
        try:
            return self.index[section][name]
        except KeyError:
            raise KeyError ("No such config item, section '%s' name '%s'" %
                            (section, name))

    def __iter__(self):
        """Iterate through each ConfigItem in index"""
        return iter(reduce(
                lambda a, b: a+b,
                [[self.index[s][n] for n in self.index[s]] \
                     for s in self.index]))

    def get(self, section, name):
        return self.index_lookup(section, name).get()

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
