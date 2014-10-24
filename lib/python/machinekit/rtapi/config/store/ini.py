from machinekit.rtapi.config.store import ConfigStore
from machinekit.rtapi.config.item import ConfigString, ConfigInt, ConfigBool, \
    IniFileConfig
from machinekit.rtapi.exceptions import \
    RTAPIConfigNotFoundException, RTAPIConfigException
from ConfigParser import SafeConfigParser, NoSectionError, NoOptionError


class IniStore(ConfigStore):
    """
    Abstract '.ini' configuration storage class (read-only)

    Subclasses should add 'name' and 'inifile_config' attributes
    """

    #name = 'myconfig.ini'              # name for storage backend
    inifile_config = 'inifile'          # config item naming .ini file path
    priority = 30                       # normally follows command
                                        # line and environment
    section_map = {}                    # dict mapping item.section to
                                        # config file section; this is
                                        # used for migration purposes
    read_only = True                    # .ini files are read-only
                                        # in practice
    item_class_filter = IniFileConfig   # only handle this item class

    @property
    def ini_filename(self):
        if self._ini_filename is None:
            # Get .ini filename either from config passed in, or from
            # a higher-level config store
            self._ini_filename = self.plugin_config.get(
                'inifile', self.config.get(self.inifile_config))
            if self._ini_filename is None:
                raise RTAPIConfigException(
                    "Unable to find configuration %s/%s file path for %s" % \
                        self.inifile_config + (self.name,))
        return self._ini_filename

    def __init__(self, config):
        super(IniStore, self).__init__(config)

        # init parser
        self.parser = SafeConfigParser()
        self._ini_filename = None

        try:
            files_read = self.parser.read(self.ini_filename)
            if len(files_read) == 0:
                raise RTAPIConfigException(
                    "Unable to read file %s" %
                    (self.ini_filename))
            self.log.debug("Read %d ini files: %s" % (len(files_read),
                                                      ' '.join(files_read)))
        except Exception as e:
            raise RTAPIConfigException("Unable to read and parse file %s: %s" %
                                       (self.ini_filename, e))

    def get_by_type(self, item, section=None):
        # Use item's usual section, or an alternate remapped section
        if section is None:
            section = item.section

        if isinstance(item, ConfigString):
            return self.parser.get(section, item.name)
        if isinstance(item, ConfigInt):
            return self.parser.getint(section, item.name)
        if isinstance(item, ConfigBool):
            return self.parser.getboolean(section, item.name)
        raise RTAPIConfigException(
            "Unhandled ConfigItem class for %s" % item.name)

    def get(self, item):
        # outer 'try:' catches usual exceptions
        try:
            # inner 'try:' helps with section remappings
            try:
                # run ConfigParser.parser.(get|getint|getboolean)
                value = self.get_by_type(item)
            except (NoSectionError, NoOptionError) as e:
                # not found; if there's a section mapping, try again
                if self.section_map.get(item.section,None):
                    value = self.get_by_type(
                        item, section=self.section_map[item.section])
                else:
                    raise e
        except NoSectionError as e:
            raise RTAPIConfigNotFoundException(
                "item %s/%s section not found in .ini file: %s" %
                (item.section, item.name, e))
        except NoOptionError as e:
            raise RTAPIConfigNotFoundException(
                "item %s/%s not found in .ini file: %s" %
                (item.section, item.name, e))
        return value
