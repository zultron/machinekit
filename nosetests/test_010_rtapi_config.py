from . import FixtureTestCase
from nose.plugins.attrib import attr
from nose.tools import assert_equal, assert_almost_equal, assert_in, \
    assert_greater, assert_false, assert_true, \
    assert_is_none, assert_is_not_none, assert_is_instance, \
    assert_raises

from machinekit.rtapi.config import Config, \
    RTAPIConfigException, RTAPIConfigNotFoundException
import os, resource

@attr('config')
class test_010_rtapi_config(FixtureTestCase):

    test_ini = 'test_010_rtapi_config.ini'
    environment = [  # config items from environment to test
        [ 'flavor', 'FLAVOR', 'posix' ],
        [ 'debug', 'DEBUG', 5 ],
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
        self.f.config = Config(enabled_stores=['env','argv'],
                               store_config = {'argv' : {'argv' : []}})
        store_list = [ str(s) for s in self.f.config.stores ]
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
        assert_equal(c.get('mkuuid'),
                     '7ebbbaff-6d33-4ecb-a4cd-0ae59d34d8f8')
        # integers
        assert_equal(c.get('debug'),5)
        # booleans
        assert_false(c.get('use_shmdrv'))

    def test_01033_get_and_descriptor_match(self):
        """01033 rtapi config:  get() method and descriptor values match"""
        c = self.f.config
        # strings
        assert_equal(c.get('mkuuid'), c.mkuuid)
        # integers
        assert_equal(c.get('debug'), c.debug)
        # booleans
        assert_false(c.get('use_shmdrv'), c.use_shmdrv)

    def test_01034_read_ini_section_remapping(self):
        """01034 rtapi config:  Read keys in remapped ini section"""
        c = self.f.config
        # global -> rtapi
        assert_equal(c.get('hal_size'),768000)
        assert_equal(c.get('shmdrv_opts'),'opt1 opt2')
        # MACHINEKIT -> service
        assert_equal(c.get('interfaces'),'eth wlan usb test')

    def test_01040_read_env(self):
        """01040 rtapi config:  Read environment variables"""
        # Set environment variables, saving the old ones
        for i in self.environment:
            i.append(os.environ.pop(i[1], None))
            os.environ[i[1]] = str(i[2])
        # Init config
        c = Config(enabled_stores=['env'])
        # Test
        for i in self.environment:
            assert_equal(c.get(i[0]),i[2])
        # Restore environment
        for i in self.environment:
            old = i.pop(-1)
            if old == None:
                os.environ.pop(i[1])
            else:
                os.environ[i[1]] = str(old)

    def test_01045_read_argv(self):
        """01045 rtapi config:  Read command-line args"""
        # Set up argv pointing to .ini file
        argv_config = {'argv' : {'argv' : ["-M",self.test_ini_path]}}
        
        # Load 'argv' plugin
        c = Config(enabled_stores=['argv'],
                   store_config = argv_config)

        # Check .ini file path
        assert_equal(c.get('inifile'), self.test_ini_path)

    def get_limits(self,lim,soft=True):
        # helper function
        if soft: index = 0; name = 'soft'
        else: index = 1; name = 'hard'
        return (
            self.f.c.get('system_rlimit_%s_%s' % (lim.lower(), name)),
            resource.getrlimit(getattr(resource,'RLIMIT_%s' % lim))[index],
            )

    def test_01050_read_rlimits(self):
        """01050 rtapi config:  Read rlimits"""
        # Load 'system_rlimit' plugin
        self.f.c = Config(enabled_stores=['system_rlimit'])

        # Read a few limits
        for lim in 'MEMLOCK', 'CPU', 'CORE':
            # check type
            assert_is_instance(self.get_limits(lim,True)[0], int)
            # soft limits
            assert_equal(*self.get_limits(lim,True))
            # hard limits
            assert_equal(*self.get_limits(lim,False))

    def test_01051_write_rlimits(self):
        """01051 rtapi config:  Write rlimits"""

        # Write a limit   FIXME:  more comprehensive tests
        for lim in ('MEMLOCK',):
            # lower soft limit
            (old_plug_val, sys_val) = self.get_limits(lim,True)
            self.f.c.set('system_rlimit_%s_soft' % lim.lower(), old_plug_val-1)
            new_plug_val, sys_val = self.get_limits(lim,True)
            assert_equal(old_plug_val-1, new_plug_val)

            # exceed soft limit:  exception
            assert_raises(RTAPIConfigException,
                          self.f.c.set,
                          'system_rlimit_%s_soft' % \
                              lim.lower(), old_plug_val+1)

            # lower hard limit
            (old_plug_val, sys_val) = self.get_limits(lim,False)
            self.f.c.set('system_rlimit_%s_hard' % lim.lower(), old_plug_val-1)
            new_plug_val, sys_val = self.get_limits(lim,False)
            assert_equal(old_plug_val-1, new_plug_val)

            # raise hard limit: exception
            assert_raises(RTAPIConfigException,
                          self.f.c.set,
                          'system_rlimit_%s_hard' % \
                              lim.lower(), old_plug_val+1)

    def test_01070_lower_store_reads_higher_store(self):
        """01070 rtapi config:  Lower- to higher-level store reads"""
        # Set up argv pointing to .ini file
        argv_config = {'argv' : {'argv' : ["-M",self.test_ini_path]}}

        # Load 'argv' and 'test.ini' plugins
        c = Config(enabled_stores=['argv','test.ini'],
                   store_config = argv_config)

        # Check .ini file path
        assert_equal(c.get('inifile'), self.test_ini_path)
        # Check .ini values
        assert_equal(c.get('debug'),5)

    flavors = ('posix', 'rt-preempt', 'xenomai', 'xenomai-kernel',
               'rtai-kernel')

    def test_01080_flavor_store_knows_all_flavors(self):
        """01080 rtapi config:  Flavor store knows all flavors"""
        c = Config(enabled_stores=['flavor'])

        for f in self.flavors:
            # f(g(x)) == x
            assert_equal(
                c.get('flavor_%s_name' % c.get('flavor_var_%s' % f)),
                f)

    flavor_attributes = {'name' : 'posix',
                         'mod_ext' : '.so',
                         'so_ext' : '.so',
                         'build_sys' : 'user-dso',
                         'id' : 0,
                         'flags': 0,
                         }


    def test_01081_flavor_store_knows_all_attributes(self):
        """01081 rtapi config:  Flavor store knows all attributes"""
        flavor = self.flavor_attributes['name']
        c = Config(enabled_stores=['flavor'])

        for a in self.flavor_attributes:
            assert_equal(c.get('flavor_%s_%s' % (flavor, a)),
                         self.flavor_attributes[a])

    def test_01082_current_flavor_store_knows_all_attributes(self):
        """01081 rtapi config:  Current flavor store knows all attributes"""
        flavor = self.flavor_attributes['name']
        c = Config(enabled_stores=['flavor','current_flavor'],
                   store_config = {'current_flavor' : {'flavor' : flavor}})

        for a in self.flavor_attributes:
            assert_equal(c.get('flavor_%s' % (a)),
                         self.flavor_attributes[a])


    def test_01090_read_use_shmdrv_store(self):
        """01090 rtapi config:  read use_shmdrv store"""
        for f in self.flavors:
            # set up config for each flavor and check if shmdrv is correct
            c = Config(
                enabled_stores=['flavor','current_flavor', 'use_shmdrv'],
                store_config = {'current_flavor' : {'flavor' : f}})
            if c.get('flavor_mod_ext') == '.so':
                use_shmdrv = False
            else:
                use_shmdrv = True
            print "Flavor %s use_shmdrv = %s" % (f, use_shmdrv)
            assert_equal(c.get('use_shmdrv'), use_shmdrv)
