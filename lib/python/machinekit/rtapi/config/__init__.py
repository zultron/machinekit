from machinekit.rtapi.plugin import PluginLoader
from machinekit.rtapi.exceptions import RTAPIConfigNotFoundException
from item import ConfigItem
from store import ConfigStore
import logging

class Items(PluginLoader):
    """
    Machinekit configuration item pool class

    This class loads a pool of config items (e.g. service/service_uuid
    or rtapi/rtapi_debug) implemented as plugin classes.  Each config
    item is a subclass of the abstract ConfigItem class.

    These config items are matched with config storage backend stacks
    to handle get/set operations.
    """

    pluginclass = ConfigItem
    flagattr = 'name'
    no_sort = True
    instantiate = False     # instantiated per storage backend

    def sort_config_items(self):
        self.bysection = {}
        for i in self.itemlist:
            # For lookup by section,key
            # { section : { key : item, ... }, ... }
            self.bysection.getdefault(i.section,{})[i.key] = i


class Stores(PluginLoader):
    """
    Machinekit configuration storage stack class

    A Config object is a stack of configuration storage backend
    plugins sorted by priority.  A storage backend knows how to filter
    config items for which its get/set operations are applicable.
    """

    pluginclass = ConfigStore

    def init_items(self, items):
        for s in self:
            s.init_items(items)
            

    def get(self, section, key):
        for store in self:
            if store.has_key(section, key):
                return store.get(section, key)
        else:
            raise KeyError("No config store with section '%s' key '%s'" %
                           (section, key))

    def set(self, section, key, value):
        for store in self:
            if store.can_store_key(section, key):
                store.set(section, key, value)
                return
        else:
            raise KeyError("No config store can store section '%s' key '%s'" %
                           (section, key))

    def finalize_init(self):
        pass

class Config(object):
    """
    The Machinekit configuration management class

    The config object matches get/set requests for config items in the
    Items object to config store backends in the Stores object.
    """

    def __init__(self):
        self.log = logging.getLogger(self.__module__)

        # Index for storing (section,name) config items
        self.index = {}

        self.log.debug("Finding and loading config item plugins")
        self.items = Items()
        self.items.load_plugins()

        self.log.debug("Finding and loading config store backend plugins")
        self.stores = Stores()
        self.stores.load_plugins()

        self.log.debug("Indexing and matching items with stores")
        for item in self.items:
            self.init_item(item)

        self.log.debug("Initializing stores")
        for store in self.stores:
            self.init_store(store)

        self.log.debug("Machinekit configuration initialized and ready")

    def init_item(self, item):
        self.index_item(item)
        # ... add other per-item operations here

    def index_item(self,item):
        self.index.setdefault(item.section,{}).setdefault(
            item.name, [[],[],getattr(item,'default',None)])
        for store in self.stores:
            obj = None
            if store.item_class_get_filter(item):
                obj = store.plugin_class_translator(item)()
                obj.set_getter_store(store)
                self.index[item.section][item.name][0].append(obj)
            if store.item_class_set_filter(item):
                if obj is None:
                    obj = store.plugin_class_translator(item)()
                obj.set_setter_store(store)
                self.index[item.section][item.name][1].append(obj)

    def init_store(self, store):
        # Add reference to Config object for stores that need
        # configuration, e.g. .ini file location
        store.config = self
        store.finalize_init()

    def default(self, section, name):
        try:
            default = self.index[section][name][2]
        except KeyError:
            raise KeyError ("No such config item, section '%s' name '%s'" %
                            (section, name))
        return default

    def getter_items(self, section, name):
        try:
            getter_items = self.index[section][name][0]
        except KeyError:
            raise KeyError ("No such config item, section '%s' name '%s'" %
                            (section, name))
        return getter_items
        
    def get(self, section, name):
        for getter_item in self.getter_items(section, name):
            if not getter_item.getter_store.initialized:
                # don't query unitialized stores
                continue
            try:
                value = getter_item.get()
            except RTAPIConfigNotFoundException:
                # config store had nothing set for this item
                continue
            return value
        else:
            return self.default(section, name)

    def setter_items(self, section, name):
        try:
            setter_items = self.index[section][name][1]
        except KeyError:
            raise KeyError ("No such config item, section '%s' name '%s'" %
                            (section, name))
        return setter_items
        

    def dump(self):
        self.log.info("Dumping configuration")

        print "%-30s = %-30s  %s/%s" % \
            ('%s/%s' % ('section', 'name'), 'value',
             'getter stores', 'setter stores')
        print "%-30s   %-30s  %s" % \
            ('-'*30, '-'*30, '-'*30)
        for section in self.index:
            for name in self.index[section]:
                val = str(self.get(section,name))
                if len(val) > 30:
                    val = '...%s' % val[-27:]
                print "%-30s = %-30s  %s/%s" % \
                    ('%s/%s' % (section, name), val,
                     ' '.join([g.getter_store.name \
                                   for g in self.getter_items(section, name)]),
                     ' '.join([s.setter_store.name \
                                   for s in self.setter_items(section, name)]))

