from machinekit.rtapi.config.store import \
    ConfigStore, RTAPIConfigNotFoundException

class TestStore(ConfigStore):
    """
    This store is for testing, and allows variables to be set from
    Config() arguments.

    Config(enabled_stores = 'test',
           store_config = {'test' : {'attr1' : 'val'}})
    """

    name = "test"
    priority = 05                       # preempt most everything
    disabled = True                     # normally disabled

    def get(self, item):
        try:
            return self.plugin_config[item.name]
        except KeyError:
            raise RTAPIConfigNotFoundException(
                "item %s not found in file: %s" %
                (item.section, item.name))

    @property
    def config_dict(self):
        """
        Mnemonic method for passing config dict to test harness for
        live manipulation
        """
        return self.plugin_config
