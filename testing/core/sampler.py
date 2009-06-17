import unittest
import gobject
from pyfirtree import *

class Creation(unittest.TestCase):
    def setUp(self):
        self._s = Sampler()
        self.failIfEqual(self._s, None)

    def tearDown(self):
        self._s = None

    def testDefaultFunction(self):
        self.assertEqual(debug_dump_sampler_function(self._s), None)

    def testDefaultExtent(self):
        extent = self._s.get_extent()
        self.assertEqual(len(extent), 4)
        # Don't know in python the exact min/max floats so test roughly
        self.assert_(extent[0] < -1e10)
        self.assert_(extent[1] < -1e10)
        self.assert_(extent[0] + extent[2] > 1e10)
        self.assert_(extent[1] + extent[3] > 1e10)

    def testDefaultTransform(self):
        trans = self._s.get_transform()
        self.assert_(trans.is_identity())

# vim:sw=4:ts=4:et:autoindent

