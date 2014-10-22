from machinekit.rtapi.config.item import ConfigString, ConfigInt, ConfigBool
from machinekit.rtapi.config.store import ConfigStore
from machinekit.rtapi.exceptions import \
    RTAPIConfigNotFoundException, RTAPIConfigException
from ConfigParser import SafeConfigParser, NoSectionError, NoOptionError


class IniConfigItem(object):
    """
    A '.ini' file config item mixin class

    Connects config item to ConfigParser object pointing at a '.ini' file.
    """
    def __init__(self):
        super(IniConfigItem, self).__init__()

    @property
    def ini_getter(self):
        return self.getter.parser.get

    def get(self):
        ini_get_method = getattr(self.getter_store.parser, self.ini_get_method)
        try:
            try:
                # run ConfigParser.parser.(get|getint|getboolean)
                value = ini_get_method(self.section, self.name)
            except NoSectionError as e:
                # if there's a section mapping, try again
                if self.getter_store.section_map.get(self.section,None):
                    value = ini_get_method(
                        self.getter_store.section_map[self.section],
                        self.name)
                else:
                    raise e
        except NoSectionError as e:
            raise RTAPIConfigNotFoundException(
                "item %s/%s section not found in .ini file: %s" %
                (self.section, self.name, e))
        except NoOptionError as e:
            raise RTAPIConfigNotFoundException(
                "item %s/%s not found in .ini file: %s" %
                (self.section, self.name, e))
        return value


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
    section_maps = {}                   # dict mapping item.section to
                                        # config file section; this is
                                        # used for migration purposes
    read_only = True                    # .ini files are read-only
                                        # in practice

    def __init__(self):
        # Set up ConfigParser first
        self.parser = SafeConfigParser()
        self._ini_filename = None

        super(IniStore, self).__init__()

    def finalize_init(self):
        self.log.debug("Parsing .ini file: %s" % self.ini_filename)
        try:
            self.parser.read(self.ini_filename)
        except Exception as e:
            raise RTAPIConfigException("Unable to read and parse file %s: %s" %
                                       (self.ini_filename, e))
        super(IniStore, self).finalize_init()

    @property
    def ini_filename(self):
        if self._ini_filename is None:
            self._ini_filename = self.config.get(*self.inifile_config)
            if self._ini_filename is None:
                raise RTAPIConfigException(
                    "Unable to find configuration %s/%s file path for %s" % \
                        self.inifile_config + (self.name,))
        return self._ini_filename

    def plugin_class_translator(self, cls):
        """Subclass config item class, adding ArgparseConfigItem mixin class"""
        if issubclass(cls, ConfigString):
            return type('IniConfigString',
                        (IniConfigItem, cls),
                        {'ini_get_method':'get'})
        if issubclass(cls, ConfigInt):
            return type('IniConfigInt',
                        (IniConfigItem, cls),
                        { 'ini_get_method' : 'getint' })
        if issubclass(cls, ConfigBool):
            return type('IniConfigBool',
                        (IniConfigItem, cls),
                        { 'ini_get_method' : 'getboolean' })
        raise Exception ("shouldn't be here")

    def item_class_get_filter(self, cls):
        """can_get items with section attribute from .ini files"""
        return getattr(cls,'section',None) is not None

    def item_class_set_filter(self, cls):
        """can_set nothing in .ini files"""
        return False
