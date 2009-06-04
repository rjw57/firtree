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

    def testDefaultValue(self):
        self.assert_(self._t.is_identity())

class Identity(unittest.TestCase):
    def setUp(self):
        self._t = AffineTransform()
        self.failIfEqual(self._t, None)
        self.assert_(self._t.is_identity())

    def tearDown(self):
        self._t = None

    def testElement1(self):
        self._t.set_elements(1,2,3,4,5,6)
        self.assertEqual(self._t.get_elements(), (1,2,3,4,5,6))
        self.assert_(not self._t.is_identity())
        self._t.set_elements(1,0,0,1,0,0)
        self.assertEqual(self._t.get_elements(), (1,0,0,1,0,0))
        self.assert_(self._t.is_identity())

    def testElement2(self):
        self._t.set_elements(1,2,3,4,5,6)
        self.assertEqual(self._t.get_elements(), (1,2,3,4,5,6))
        self.assert_(not self._t.is_identity())
        self._t.set_identity()
        self.assert_(self._t.is_identity())

class CopyAndClone(unittest.TestCase):
    def setUp(self):
        self._t = AffineTransform()
        self.failIfEqual(self._t, None)
        self.assert_(self._t.is_identity())

    def tearDown(self):
        self._t = None

    def testCopy(self):
        self._t.set_elements(1,2,3,4,5,6)
        self.assertEqual(self._t.get_elements(), (1,2,3,4,5,6))
        t = AffineTransform()
        self.assert_(t.is_identity())
        t.set_transform(self._t)
        self.assertEqual(t.get_elements(), (1,2,3,4,5,6))

    def testClone(self):
        self._t.set_elements(1,2,3,4,5,6)
        self.assertEqual(self._t.get_elements(), (1,2,3,4,5,6))
        t = self._t.clone()
        self._t.set_elements(6,5,4,3,2,1)
        self.assertEqual(self._t.get_elements(), (6,5,4,3,2,1))
        self.assertEqual(t.get_elements(), (1,2,3,4,5,6))

# vim:sw=4:ts=4:et:autoindent

