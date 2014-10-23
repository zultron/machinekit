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

    @staticmethod
    def get_cmp_closure(flag_attr):
        def attr_cmp(x,y):
            """
            Sort objects by the attribute named in flag_attr
            """
            if getattr(x, flag_attr) > getattr(y, flag_attr):
                return 1
            if getattr(x, flag_attr) < getattr(y, flag_attr):
                return -1
            return 0
        return attr_cmp


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
        self.log.debug("      Inspecting module '%s'" % module_name)
        info = imp.find_module(module_name, [self.plugindir])
        module = imp.load_module(module_name, *info)
        return module

    def search_module_for_plugins(self, module):
        for attrname in dir(module):
            attr = getattr(module, attrname)
            if type(attr) == type:
                cls = attr  # attribute is a class
                if (issubclass(cls, self.pluginclass) and \
                        not getattr(cls,'disabled',False) and \
                        getattr(cls,self.flagattr,None) is not None):
                    # class is an enabled plugin class
                    self.plugin_classes.append(cls)

    def process_plugin_classes(self,*args,**kwargs):
        for cls in self.plugin_classes:
            if self.instantiate:
                obj = cls(*args,**kwargs)
                self.log.debug("        Loading object %s" % obj)
                self.plugins.append(obj)
            else:
                self.log.debug("        Loading class %s" % cls)
                self.plugins.append(cls)

    def __init__(self,*args,**kwargs):
        """
        Initialize plugin system, especially setting up the plugin
        directory
        """
        # set up basic attributes
        self.name = self.pluginclass.__name__
        self._plugindir = None
        self.plugins = []
        self.plugin_classes = []

        # logging
        self.log = logging.getLogger(self.__module__)
        self.log.debug("    Loading plugins for class '%s'" % self.name)

        # load modules and look for plugin classes
        for module_name in self.find_plugin_modules():

            module = self.load_module(module_name)
            self.search_module_for_plugins(module)

        # process classes and add to plugin list
        self.process_plugin_classes(*args,**kwargs)

        # sort plugins if applicable
        if not self.no_sort:
            if self.priocmp is None:
                self.priocmp = self.get_cmp_closure(self.flagattr)
            self.plugins.sort(self.priocmp)

        self.log.debug("    Loaded %d %s(s)" %
                       (len(self.plugins), self.name))

    def __iter__(self):
        return iter(self.plugins)
