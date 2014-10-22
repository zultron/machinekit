from machinekit.rtapi.config.item import ConfigString, ConfigInt, ConfigBool
from machinekit.rtapi.config.store import ConfigStore
from machinekit.rtapi.exceptions import RTAPIConfigNotFoundException
import argparse, string

class ArgparseConfigItem(object):
    """
    Argparse config item mixin class

    Connects config item to argparse.parser.add_argument()
    """
    transtable = string.maketrans('_','-')  # translate '_' to '-' in cl args
    argparse_extra_kwargs = {}

    def __init__(self):
        super(ArgparseConfigItem, self).__init__()

        # Set up argparse option
        self.kwargs = {'help' : self.help}
        self.kwargs.update(self.argparse_extra_kwargs)

    def set_getter_store(self, store):
        super(ArgparseConfigItem, self).set_getter_store(store)

        # Call argparse.parser.add_argument()
        store.parser.add_argument(
            self.longopt_str, self.shortopt_str, **self.kwargs)

    def get(self):
        value = getattr(self.getter_store.opts, self.longopt)
        if value is None:
            raise RTAPIConfigNotFoundException(
                "item %s not found in command line args" % self.longopt)
        return getattr(self.getter_store.opts, self.longopt)

    @property
    def longopt_str(self):
        return '--%s' % string.translate(self.longopt, self.transtable)

    @property
    def shortopt_str(self):
        return '-%s' % self.shortopt


class ArgparseStore(ConfigStore):
    """
    Command line argument configuration storage class: a read-only
    source of configuration variables
    """

    name = "argv"
    priority = 10                       # command line args are top prio
    read_only = True                    # writing cl args is nonsensical

    def __init__(self):
        # Set up argument parser first
        self.parser = argparse.ArgumentParser(
            description='Machinekit messaging daemon')

        super(ArgparseStore, self).__init__()

    def finalize_init(self):
        self.log.debug("Parsing command line args")
        self.opts = self.parser.parse_args()
        super(ArgparseStore, self).finalize_init()

    def plugin_class_translator(self, cls):
        """Subclass config item class, adding ArgparseConfigItem mixin class"""
        if issubclass(cls, ConfigString):
            return type('ArgparseConfigString',
                        (ArgparseConfigItem, cls),{})
        if issubclass(cls, ConfigInt):
            return type('ArgparseConfigInt',
                        (ArgparseConfigItem, cls),
                        {'argparse_extra_kwargs' : { 'type' : int }})
        if issubclass(cls, ConfigBool):
            return type('ArgparseConfigBool',
                        (ArgparseConfigItem, cls),
                        {'argparse_extra_kwargs' : { 'action' : 'count' }})
        raise Exception ("shouldn't be here")

    def item_class_get_filter(self, cls):
        """Argparse can_get items with longopt attribute"""
        return getattr(cls,'longopt',None) is not None

    def item_class_set_filter(self, cls):
        """Argparse can_set no items"""
        return False
