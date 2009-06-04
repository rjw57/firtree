import unittest
import gobject
from pyfirtree import *

class Creation(unittest.TestCase):
    def setUp(self):
        self._s = Sampler()
        self.failIfEqual(self._s, None)

    def tearDown(self):
        self._s = None

    def testDefaultExtent(self):
        extent = self._s.get_extent()
        self.assertEqual(len(extent), 4)
        self.assertEqual(extent, (0,0,0,0))

# vim:sw=4:ts=4:et:autoindent

