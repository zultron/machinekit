from . import FixtureTestCase
from unittest import skip, SkipTest
from nose.tools import assert_equal, assert_almost_equal, assert_not_equal, \
    assert_in, assert_greater, assert_less, assert_false, assert_true, \
    assert_is_none, assert_is_not_none, assert_is_instance, \
    assert_raises
import mock

from machinekit.rtapi.rtapi_heap_bindings import \
    rtapi_heap, RTAPIHeapRuntimeError
from machinekit.rtapi import shmdrv_api
import os

class test_026_rtapi_heap(FixtureTestCase):

    ########################################
    # Functions for debugging the allocator
    def print_free_blocks(self):
        print "total_avail = %s, fragments = %s, largest = %s" % \
            self.heap.status()
        for s, o in self.heap:
            print "  free block: %s bytes @ %s..%s" % (s, o, o+s)

    def realsize(self,s):
        padded_size = ((s - 1) / self.heap.blocksize + 1) * self.heap.blocksize
        return padded_size + self.heap.blocksize

    def arenarealsize(self,s):
        return s / self.heap.blocksize * self.heap.blocksize

    alloc_hash = {}
    def alloc_info(self, name, pop=False):
        p, size = self.alloc_hash[name]
        o = self.heap.offset(p)
        real_o = o - self.heap.blocksize

        if pop:
            print "      popping %s (p=%x, o=%d, s=%d)" % (name, p, o, size)
            self.alloc_hash.pop(name)

        return "'%s':  %d bytes @ %d (real %d bytes @ %d..%d)" % \
            (name, size, o,
             self.realsize(size), real_o, real_o + self.realsize(size))

    def expected_free(self):
        free = self.total  # set in a test
        for name, (p, size) in self.alloc_hash.items():
            free -= self.realsize(size)
        return free

    def alloc(self, name, size):
        self.alloc_hash[name] = (self.heap.malloc(size), size)
        self.print_alloc_info('alloc', name)

    def free(self, name):
        self.heap.free(self.alloc_hash[name][0])
        self.print_alloc_info('free', name, pop=True)

    def print_alloc_info(self, hdr, name, pop=False):
        print "%s %s" % (hdr, self.alloc_info(name, pop))
        self.print_free_blocks()
        print

    def malloc(self, name, size):
        self.alloc_hash[name] = (self.heap.malloc(size), size)
        self.print_alloc_info('alloc', name)

    def free(self, name):
        self.heap.free(self.alloc_hash[name][0])
        self.print_alloc_info('free', name, pop=True)

    #######################################

    def test_02610_rtapi_heap_setup(self):
        """02610 rtapi_heap:  rtapi_heap setup and init"""

        self.fix(
            key1 = 0x00badbad,
            size1 = 4096,
            heap_header_size = 24,
            malloc_header_size = 8,
            )

        # Init shm
        shmdrv_api.init()

        # Unlink any existing seg
        try:
            shmdrv_api.SHMSegment().attach(self.key1).unlink()
            print "(detached existing seg)"
        except:
            pass

        # Create heap shm segment
        self.fix(
            seg1 = shmdrv_api.SHMSegment().new(self.key1, self.size1),
            )
        print "created shm seg %s, addr %x, key %s, size %s" % \
            (self.seg1.posix_name, self.seg1.ptr, self.seg1.key, self.seg1.size)


    def test_02620_init_heap(self):
        """02620 rtapi_heap:  init heap and add memory"""

        # Setup:  init heap; add memory
        self.fix(
            heap = rtapi_heap(self.seg1.ptr),
            )
        # Use space in shm seg after the heap header
        arenaptr = self.seg1.ptr + self.heap.headersize
        arenasize = self.size1 - self.heap.headersize
        self.fix(total = self.arenarealsize(arenasize))
        self.heap.addmem(arenaptr, arenasize)

        # Setup:  get stats
        total_avail, fragments, largest = self.heap.status()

        # Debug
        print "list of free blocks:"
        for size, offset in self.heap:
            print "  size = %s, offset = %s" % (size, offset)

        # Test:  stats as expected
        assert_equal(fragments, 1)
        assert_equal(total_avail, self.total)
        assert_equal(largest, self.total)

    def test_02630_malloc1(self):
        """02630 rtapi_heap:  malloc()"""

        # Setup:  malloc(200); get stats
        self.malloc(1, 200)
        total_avail, fragments, largest = self.heap.status()

        # Debug
        self.print_alloc_info('malloc(1,200)', 1)

        # Test:  stats as expected
        assert_equal(fragments, 1)
        assert_equal(total_avail, self.expected_free())
        # 1 frag, so expected_free() == largest
        assert_equal(largest, self.expected_free())

    def test_02640_malloc_free(self):
        """02640 rtapi_heap:  malloc() and free()"""

        # Setup:  malloc(100) and malloc(300); get stats
        self.malloc(2, 100)
        self.malloc(3, 300)
        total_avail, fragments, largest = self.heap.status()

        # Test:  stats as expected
        assert_equal(fragments, 1)
        assert_equal(total_avail, self.expected_free())
        # 1 frag, so expected_free() == largest
        assert_equal(largest, self.expected_free())

        # Setup:  free the last allocated chunk
        self.free(3)
        total_avail, fragments, largest = self.heap.status()

        # Test:  stats as expected
        assert_equal(fragments, 1)
        assert_equal(total_avail, self.expected_free())
        # 1 frag, so expected_free() == largest
        assert_equal(largest, self.expected_free())

        # Setup:  free all chunks
        self.free(2)
        self.free(1)
        total_avail, fragments, largest = self.heap.status()

        # Test:  stats as expected
        assert_equal(fragments, 1)
        assert_equal(total_avail, self.total)
        assert_equal(largest, self.total)

    def test_02650_fragmentation(self):
        """02650 rtapi_heap:  fragmentation"""

        # Setup:  malloc 100, 200, 300
        self.malloc(1, 100)
        self.malloc(2, 200)
        self.malloc(3, 300)
        total_avail, fragments, largest = self.heap.status()
        # save largest
        largest_saved = largest

        # Test:  stats as expected
        assert_equal(fragments, 1)
        assert_equal(total_avail, self.expected_free())
        assert_equal(largest, self.expected_free())

        # Setup:  free #2
        self.free(2)
        total_avail, fragments, largest = self.heap.status()

        # Test:  stats as expected
        assert_equal(fragments, 2)
        assert_equal(total_avail, self.expected_free())
        # largest should be the same
        assert_equal(largest, largest_saved)

        # Setup:  malloc 100 and 50
        self.malloc(4, 100)
        self.malloc(5, 50)
        total_avail, fragments, largest = self.heap.status()

        # Test:  frags & sizes
        assert_equal(fragments, 2)
        assert_equal(total_avail, self.expected_free())
        # largest should be the same
        assert_equal(largest, largest_saved)

        # Test: 4 & 5 allocated in the hole left by 2, so
        # order is 3, 5, 4, 1
        assert_greater(self.alloc_hash[5][0], self.alloc_hash[3][0])
        assert_greater(self.alloc_hash[4][0], self.alloc_hash[5][0])
        assert_greater(self.alloc_hash[1][0], self.alloc_hash[4][0])

        # Setup:  malloc 100, which won't fit in hole
        self.malloc(6, 100)
        total_avail, fragments, largest = self.heap.status()

        # Test:  largest should have decreased, and new chuck is below 3
        assert_equal(fragments, 2)
        assert_equal(total_avail, self.expected_free())
        assert_less(largest, largest_saved)
        assert_greater(self.alloc_hash[3][0], self.alloc_hash[6][0])

        # Test:  allocating one byte larger than available raises exception
        largest_saved = largest
        assert_raises(RTAPIHeapRuntimeError, self.malloc, 7,
                      largest_saved - self.heap.blocksize + 1)

        # Setup:  allocate exactly largest available
        self.malloc(8, largest_saved - self.heap.blocksize)
        total_avail, fragments, largest = self.heap.status()

        # Test:  1 less fragment; largest block decreased
        assert_equal(fragments, 1)
        assert_equal(total_avail, self.expected_free())
        assert_less(largest, largest_saved)

        # Setup:  cleanup
        for m in (1, 3, 4, 5, 6, 8):
            self.free(m)
        total_avail, fragments, largest = self.heap.status()

        # Test:  everything free
        assert_equal(fragments, 1)
        assert_equal(total_avail, self.total)
        assert_equal(largest, self.total)

