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

    def filter_module_plugin_classes(self, module):
        """Filter out disabled, abstract, etc. module plugin classes"""
        count = 0; loaded_count = 0
        for attrname in dir(module):
            cls = getattr(module, attrname)
            if not isinstance(cls, type) or \
                    not issubclass(cls, self.pluginclass) or \
                    getattr(cls,'name',None) is None:
                continue  # not a plugin or abstract plugin class
            count += 1

            # filter plugins
            if getattr(cls,'disabled',False) and self.enabled_plugins is None:
                continue  # 'disabled' attribute set; overridden by
                          # enabled_plugins
            if self.enabled_plugins is not None and \
                    cls.name not in self.enabled_plugins:
                continue  # not in 'enabled_plugins' list
            if self.disabled_plugins is not None and \
                    cls.name in self.enabled_plugins:
                continue  # in 'disabled_plugins' list

            # plugin looks good; add to list
            self.plugin_classes.append(cls)
            loaded_count += 1
        self.log.debug("      Module %s:  loaded %d of %d plugin classes" %
                       (module.__name__, loaded_count, count))

    def process_plugin_classes(self,*args,**kwargs):
        # sort plugins if applicable
        if not self.no_sort:
            self.plugin_classes.sort(self.cmp_closure)

        for cls in self.plugin_classes:
            if self.instantiate:
                obj = cls(*args,**kwargs)
                self.plugins.append(obj)
                self.log.debug("      Added object %s" % obj)
            else:
                self.log.debug("      Added class %s" % cls)
                self.plugins.append(cls)

    @property
    def cmp_closure(self):
        self.log.debug("Sorting plugins by attribute '%s'" % self.flagattr)
        def attr_cmp(x,y):
            """
            Sort objects by the attribute named in flag_attr
            """
            if getattr(x, self.flagattr) > getattr(y, self.flagattr):
                return 1
            if getattr(x, self.flagattr) < getattr(y, self.flagattr):
                return -1
            return 0
        return attr_cmp


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
        self.plugins = []
        self.plugin_classes = []
        self.enabled_plugins = kwargs.pop('enabled_plugins',None)
        self.disabled_plugins = kwargs.pop('disabled_plugins',None)

        # load modules and look for plugin classes
        self.log.debug("    Importing modules and searching for plugins")
        for module_name in self.find_plugin_modules():
            module = self.load_module(module_name)
            self.filter_module_plugin_classes(module)

        # process classes and add to plugin list
        self.log.debug("    Build plugin list")
        self.process_plugin_classes(*args,**kwargs)

        self.log.debug("    Loaded %d %s(s)" %
                       (len(self.plugins), self.name))

    def __iter__(self):
        return iter(self.plugins)
