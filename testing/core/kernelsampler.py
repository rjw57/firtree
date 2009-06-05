import unittest
import gobject
from pyfirtree import *

class Creation(unittest.TestCase):
    def setUp(self):
        self._s = KernelSampler()
        self.failIfEqual(self._s, None)

    def tearDown(self):
        self._s = None

# vim:sw=4:ts=4:et:autoindent

