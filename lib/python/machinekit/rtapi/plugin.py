import logging, os, imp, inspect

class RTAPIPluginException(RuntimeError):
    """
    Raised by plugin system
    """
    pass

class PluginLoader(object):
    """
    A simple plugin system:

    Look in all *.py files in the plugin directory path.  If the path
    is None or is relative, use the module directory of pluginclass as
    the base.

    Build a list of any subclasses of pluginclass where flagattr is
    not None (indicates class is not abstract), and sort the list
    using the supplied sortfn, or by flagattr (which defaults to the
    'priority' attribute).

    This class is meant to be subclassed and fleshed out
    """

    pluginclass = None          # Superclass of plugin modules
    path = None                 # Path to plugin directory, raw
    instantiate = True          # True: put class instances in list;
                                # False: put classes in list

    # priority sorting:
    flagattr = "priority"       # sort attribute
    priocmp = None              # cmp function
    no_sort = False             # Set to True to disable sorting

    cache = {}                  # Cache for loaded classes

    @property
    def plugindir(self):
        if self._plugindir is None:
            # Work out plugin directory path
            if self.path is None:
                # default:  look for plugins in the cls module's directory
                path = os.path.dirname(inspect.getfile(self.pluginclass))
            elif not os.path.isabs(path):
                # relative path: look for plugins in the given directory
                # relative to this module
                path = os.path.join(os.path.dirname(__file__),path)
            # normalize the path
            self._plugindir = os.path.realpath(path)

            # assert the path is a directory
            if not os.path.isdir(self._plugindir):
                raise RTAPIPluginException(
                    "Class %s plugin path is not a directory: %s" %
                    (cls.__name__, self._plugindir))
            #self.log.debug("    Using plugin directory %s" % self._plugindir)
        return self._plugindir

    def find_plugin_modules(self):
        module_names = []
        for fname in os.listdir(self.plugindir):
            (module_name, ext) = os.path.splitext(fname)
            if ext != ".py" or module_name == "__init__":
                continue # weed out junk files
            module_names.append(module_name)
        return module_names

    def load_module(self, module_name):
        """Find and load module"""
        info = imp.find_module(module_name, [self.plugindir])
        module = imp.load_module(module_name, *info)
        return module

    def cache_plugin_classes(self, module):
        """Get plugin classes from modules"""
        for attrname in dir(module):
            cls = getattr(module, attrname)

            # select only bound plugin classes
            if not isinstance(cls, type) or \
                    not issubclass(cls, self.pluginclass):
                continue  # not a self.pluginclass
            if getattr(cls,'name',None) is None:
                continue  # abstract plugin class

            # add to plugin class list
            self.class_cache.append(cls)

    def sort_plugin_class_cache(self):
        # sort plugins if applicable
        if self.no_sort:
            return
        self.log.debug("Sorting plugins by attribute '%s'" % self.flagattr)
        self.class_cache.sort(
            lambda x,y: \
                cmp(getattr(x, self.flagattr),getattr(y, self.flagattr)))

    def find_and_load_plugin_module_classes(self):
        # Cache plugin classes.  Aside from being expensive to load
        # them twice, it also breaks a second PluginLoader.__init__()
        # when classes are added a second time
        self.log.debug("    Importing modules and searching for plugins")
        if len(self.class_cache) == 0:
            for module_name in self.find_plugin_modules():
                module = self.load_module(module_name)
                self.cache_plugin_classes(module)
            self.sort_plugin_class_cache()

    def filter_plugin_classes(self):
        """Filter out disabled plugin classes"""
        for cls in self.cache[self.name]['classes']:
            # 'enabled_plugins' list overrides everything else
            if self.enabled_plugins is not None:
                if cls.name in self.enabled_plugins:
                    self.plugin_classes.append(cls)
                continue
            # check for 'disabled' class attribute
            if getattr(cls,'disabled',False):
                continue
            # check in 'disabled_plugins' list
            if self.disabled_plugins is not None and \
                    cls.name in self.disabled_plugins:
                continue

            # add enabled plugin to list
            self.plugin_classes.append(cls)

    def build_plugin_list(self,*args,**kwargs):
        """Build final plugin list from plugin_classes"""
        for cls in self.plugin_classes:
            if self.instantiate:
                obj = cls(*args,**kwargs)
                self.plugins.append(obj)
                self.log.debug("      Added object %s" % obj)
            else:
                self.log.debug("      Added class %s" % cls)
                self.plugins.append(cls)


    def __init__(self,*args,**kwargs):
        """
        Initialize plugin system, especially setting up the plugin
        directory
        """
        # logging
        self.log = logging.getLogger(self.__module__)
        self.log.debug("  Loading plugins for class '%s'" %
                       self.pluginclass.__name__)

        # set up basic attributes
        self.name = self.pluginclass.__name__
        self._plugindir = None

        # module and plugin lists
        self.class_cache = \
            self.cache.setdefault(self.name,{}).setdefault('classes',[])
        self.plugin_classes = []
        self.plugins = []

        # enabled/disabled plugin lists
        self.enabled_plugins = kwargs.pop('enabled_plugins',None)
        self.disabled_plugins = kwargs.pop('disabled_plugins',None)
        self.log.debug("enabled plugins = %s; disabled_plugins = %s" %
                       (self.enabled_plugins, self.disabled_plugins))

        # load modules and look for plugin classes
        self.find_and_load_plugin_module_classes()

        # filter classes and add to plugin list
        self.log.debug("    Build plugin list")
        self.filter_plugin_classes()
        self.build_plugin_list(*args,**kwargs)

        self.log.debug("    Enabled %d of %d %s plugins" %
                       (len(self.plugins), len(self.class_cache), self.name))

    def __iter__(self):
        return iter(self.plugins)
