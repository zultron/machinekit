import logging, os, imp, inspect
from machinekit.rtapi.exceptions import RTAPIPluginException

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

    def __init__(self):
        """
        Initialize plugin system, especially setting up the plugin
        directory
        """
        self.name = self.pluginclass.__name__

        self.log = logging.getLogger(self.__module__)
        self.log.debug("  Initializing '%s' class plugin loader" % self.name)

        # cmp function for sorting plugins in priority order
        if self.priocmp is None:
            def priocmp(x,y):
                if getattr(x, self.flagattr) > getattr(y, self.flagattr):
                    return 1
                if getattr(x, self.flagattr) < getattr(y, self.flagattr):
                    return -1
                return 0
            self.priocmp = priocmp

        # Work out plugin directory path
        if self.path is None:
            # default:  look for plugins in the cls module's directory
            path = os.path.dirname(inspect.getfile(self.pluginclass))
        elif not os.path.isabs(path):
            # relative path: look for plugins in the given directory
            # relative to this module
            path = os.path.join(os.path.dirname(__file__),path)
        # normalize the path
        self.plugindir = os.path.realpath(path)
        # assert the patch is a directory
        if not os.path.isdir(self.plugindir):
            raise RTAPIPluginException(
                "Class %s plugin path is not a directory: %s" %
                (cls.__name__, self.plugindir))
        #self.log.debug("    Using plugin directory %s" % self.plugindir)

        self.plugins = []

    def load_plugins(self,*args,**kwargs):
        """Load plugins into list and sort into priority order"""
        self.log.debug("    Loading plugins for class '%s'" % self.name)
        for fname in os.listdir(self.plugindir):
            (base, ext) = os.path.splitext(fname)
            if ext != ".py" or base == "__init__":
                continue # weed out junk files
            self.log.debug("      Inspecting module '%s'" % fname)
            info = imp.find_module(base, [self.plugindir])
            module = imp.load_module(base, *info)
            for attrname in dir(module):
                attr = getattr(module, attrname)
                if type(attr) == type:
                    cls = attr
                    if (issubclass(cls, self.pluginclass) and \
                            not getattr(cls,'disabled',False) and \
                            getattr(cls,self.flagattr,None) is not None):
                        if self.instantiate:
                            obj = cls(*args,**kwargs)
                            self.log.debug("        Loading object %s" % obj)
                            self.plugins.append(attr(*args,**kwargs))
                        else:
                            self.log.debug("        Loading class %s" % cls)
                            self.plugins.append(cls)
        if not self.no_sort:
            self.plugins.sort(self.priocmp)
        self.log.debug("    Loaded %d %s(s)" %
                       (len(self.plugins), self.name))

    def plugin_class_translator(self,cls):
        """Override this to run a translation on the class before
        adding to list"""
        return cls

    def plugin_class_filter(self,cls):
        """Override this to select which items are added to store"""
        return True

    def __iter__(self):
        return iter(self.plugins)
