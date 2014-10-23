from . import FixtureTestCase
from nose.plugins.attrib import attr
from nose.tools import assert_equal, assert_almost_equal, assert_in, \
    assert_greater, assert_false, assert_true, \
    assert_is_none, assert_is_not_none, assert_is_instance, \
    assert_raises

from machinekit.rtapi.config import Config, RTAPIConfigException
import os

@attr('config')
class test_010_rtapi_config(FixtureTestCase):

    test_ini = 'test_010_rtapi_config.ini'
    environment = [  # config items from environment to test
        [ 'rtapi_app', 'flavor', 'FLAVOR', 'posix' ],
        [ 'rtapi', 'debug', 'DEBUG', 5 ],
        ]

    @property
    def test_ini_path(self):
        return os.path.join(os.path.dirname(__file__), self.test_ini)

    @property
    def test_ini_config(self):
        return { 'inifile' : self.test_ini_path }

    # Test basic Config object init
    def test_01010_import_config(self):
        """01010 rtapi config:  Initialize 'Config' object"""
        config = Config(enabled_stores=[])
        assert_is_not_none(config)

    # Test basic plugin loading
    def test_01020_load_plugin(self):
        """01020 rtapi config:  Load a config store plugin"""
        # Load 'env' and 'argv' plugins
        config = Config(enabled_stores=['env','argv'],
                        store_config = {'argv' : {'argv' : []}})
        store_list = [ str(s) for s in config.stores ]
        assert_equal(len(store_list),2)
        assert_in('env', store_list)
        assert_in('argv', store_list)

    # Test INI file plugin
    def test_01030_ini_file_missing_raises_exception(self):
        """01030 rtapi config:  A missing .ini file raises an exception"""
        assert_raises(RTAPIConfigException,Config,enabled_stores=['test.ini'])

    def test_01031_test_store_plugin_loading(self):
        """01031 rtapi config:  Load 'test.ini' plugin"""
        # Point inifile at fake 'test.ini'
        self.f.config = \
            Config(enabled_stores=['test.ini'],
                   store_config = {'test.ini' : self.test_ini_config })

    def test_01032_read_ini_data_types(self):
        """01032 rtapi config:  Read .ini file data types"""
        c = self.f.config
        # strings
        assert_equal(c.get('service','mkuuid'),
                     '7ebbbaff-6d33-4ecb-a4cd-0ae59d34d8f8')
        # integers
        assert_equal(c.get('rtapi','debug'),5)
        # booleans
        assert_false(c.get('rtapi','use_shmdrv'))

    def test_01033_read_ini_section_remapping(self):
        """01033 rtapi config:  Read keys in remapped ini section"""
        c = self.f.config
        # global -> rtapi
        assert_equal(c.get('rtapi','hal_size'),768000)
        assert_equal(c.get('rtapi','shmdrv_opts'),'opt1 opt2')
        # MACHINEKIT -> service
        assert_equal(c.get('service','interfaces'),'eth wlan usb test')

    def test_01040_read_env(self):
        """01040 rtapi config:  Read environment variables"""
        # Set environment variables, saving the old ones
        for i in self.environment:
            i.append(os.environ.pop(i[2], None))
            os.environ[i[2]] = str(i[3])
        # Init config
        c = Config(enabled_stores=['env'])
        # Test
        for i in self.environment:
            assert_equal(c.get(i[0],i[1]),i[3])
        # Restore environment
        for i in self.environment:
            os.environ[i[2]] = str(i.pop(-1))

    def test_01045_read_argv(self):
        """01045 rtapi config:  Read command-line args"""
        # Set up argv pointing to .ini file
        argv_config = {'argv' : {'argv' : ["-M",self.test_ini_path]}}
        
        # Load 'argv' plugin
        c = Config(enabled_stores=['argv'],
                   store_config = argv_config)

        # Check .ini file path
        assert_equal(c.get('rtapi_config','inifile'), self.test_ini_path)

    def test_01080_lower_store_reads_higher_store(self):
        """01080 rtapi config: Lower- to higher-level store reads"""
        # Set up argv pointing to .ini file
        argv_config = {'argv' : {'argv' : ["-M",self.test_ini_path]}}

        # Load 'argv' and 'test.ini' plugins
        c = Config(enabled_stores=['argv','test.ini'],
                   store_config = argv_config)

        # Check .ini file path
        assert_equal(c.get('rtapi_config','inifile'), self.test_ini_path)
        # Check .ini values
        assert_equal(c.get('rtapi','debug'),5)
