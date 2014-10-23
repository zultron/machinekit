from machinekit.rtapi.config.store import ConfigStore
from machinekit.rtapi.config.item import ConfigString, ConfigInt, ConfigBool
from machinekit.rtapi.exceptions import \
    RTAPIConfigNotFoundException, RTAPIConfigException
from ConfigParser import SafeConfigParser, NoSectionError, NoOptionError


class IniStore(ConfigStore):
    """
    Abstract '.ini' configuration storage class (read-only)

    Subclasses should add 'name' and 'inifile_config' attributes
    """

    name = 'myconfig.ini'               # name for storage backend
    inifile_config = ('global','inifile')
                                        # config item naming .ini file path
    #priority = 30                      # normally follows command
                                        # line and environment
    section_map = {}                   # dict mapping item.section to
                                        # config file section; this is
                                        # used for migration purposes
    read_only = True                    # .ini files are read-only
                                        # in practice

    def handles(self, obj):
        """Handle items with section attribute from .ini files"""
        return getattr(obj,'section',None) is not None

    @property
    def ini_filename(self):
        if self._ini_filename is None:
            self._ini_filename = self.config.get(*self.inifile_config)
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
            self.parser.read(self.ini_filename)
        except Exception as e:
            raise RTAPIConfigException("Unable to read and parse file %s: %s" %
                                       (self.ini_filename, e))

    def get_by_type(self, item, section=None):
        if section is None:
            section = item.section
        if isinstance(item, ConfigString):
            return self.parser.get(section, item.name)
        if isinstance(item, ConfigInt):
            return self.parser.getint(section, item.name)
        if isinstance(item, ConfigBool):
            return self.parser.getboolean(section, item.name)
        raise Exception("shouldn't be here")

    def get(self, item):
        # outer 'try:' catches usual exceptions
        try:
            # inner 'try:' helps with section remappings
            try:
                # run ConfigParser.parser.(get|getint|getboolean)
                value = self.get_by_type(item)
            except NoSectionError as e:
                # if there's a section mapping, try again
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
