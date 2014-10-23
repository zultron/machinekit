from machinekit.rtapi.config.item import ConfigInt, ConfigBool
from machinekit.rtapi.config.store import ConfigStore
from machinekit.rtapi.exceptions import RTAPIConfigNotFoundException
import argparse, string

class ArgparseStore(ConfigStore):
    """
    Command line argument configuration storage class: a read-only
    source of configuration variables
    """

    name = "argv"
    priority = 10                       # command line args are top prio
    read_only = True                    # writing cl args is nonsensical
    transtable = \
        string.maketrans('_','-')       # translate '_' to '-' in cl args

    def handles(self, cls):
        """Argparse can handle items with longopt attribute"""
        return getattr(cls,'longopt',None) is not None

    def longopt_str(self, item):
        return '--%s' % string.translate(item.longopt, self.transtable)

    def shortopt_str(self, item):
        return '-%s' % item.shortopt

    def arg_kwargs(self, item):
        # Generate kwargs for argparse.parser.add_argument
        kwargs = { 'help' : item.help }
        if isinstance(item, ConfigInt):
            kwargs.update({ 'type' : int })
        if isinstance(item, ConfigBool):
            kwargs.update({ 'action' : 'count' })
        return kwargs

    def add_argument(self, item):
        # Call argparse.parser.add_argument()
        self.parser.add_argument(
            self.longopt_str(item), self.shortopt_str(item),
            **self.arg_kwargs(item))

    def __init__(self, config):
        super(ArgparseStore, self).__init__(config)

        # Set up argument parser
        self.parser = argparse.ArgumentParser(
            description='Machinekit messaging daemon')

       # Do argparse.parser.add_argument for applicable items
        for item in self.config.items:
            if self.handles(item):
                self.add_argument(item)

        self.log.debug("Parsing command line args")
        self.opts = self.parser.parse_args()

    def get(self, item):
        value = getattr(self.opts, item.longopt)
        if value is None:
            raise RTAPIConfigNotFoundException(
                "item %s not found in command line args" % item.longopt)
        return item.valtype(getattr(self.opts, item.longopt))
