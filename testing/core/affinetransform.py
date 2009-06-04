import unittest
import gobject
from pyfirtree import *

class Creation(unittest.TestCase):
    def setUp(self):
        self._t = AffineTransform()
        self.failIfEqual(self._t, None)

    def tearDown(self):
        self._t = None

    def testCreated(self):
        self.failIfEqual(self._t, None)

# vim:sw=4:ts=4:et:autoindent

