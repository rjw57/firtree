import unittest
import gobject
from pyfirtree import *

class ElementAccess(unittest.TestCase):
    def setUp(self):
        self._t = AffineTransform()
        self.failIfEqual(self._t, None)

    def tearDown(self):
        self._t = None

    def assertAlmostEqualTuples(self, a, b):
        self.assertEqual(len(a), len(b))
        for i in range(0, len(a)):
            self.assertAlmostEqual(a[i], b[i], 4)

    def testElement1(self):
        self._t.set_elements(1,2,3,4,5,6)
        self.assertEqual(self._t.get_elements(), (1,2,3,4,5,6))

    def testElement2(self):
        self._t.set_elements(1,2.2,3.3,4.4,5,6)
        self.assertAlmostEqualTuples(self._t.get_elements(), (1,2.2,3.3,4.4,5,6))

class Creation(unittest.TestCase):
    def setUp(self):
        self._t = AffineTransform()
        self.failIfEqual(self._t, None)

    def tearDown(self):
        self._t = None

    def testCreated(self):
        self.failIfEqual(self._t, None)

# vim:sw=4:ts=4:et:autoindent

