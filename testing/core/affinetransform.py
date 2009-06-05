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

    def testApplication1(self):
        self._t.set_identity()
        self.assertEqual(self._t.transform_point(2,3), (2,3))

    def testApplication2(self):
        self._t.set_identity()
        self.assertEqual(self._t.transform_size(2,3), (2,3))

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

class AppendAndPrepend(unittest.TestCase):
    def setUp(self):
        self._t1 = AffineTransform()
        self._t2 = AffineTransform()
        self.failIfEqual(self._t1, None)
        self.assert_(self._t1.is_identity())
        self.failIfEqual(self._t2, None)
        self.assert_(self._t2.is_identity())

    def tearDown(self):
        self._t1 = None
        self._t2 = None

    def testAppendIdentity(self):
        self._t1.set_identity()
        self._t2.set_elements(1,2,3,4,5,6)
        self.assert_(self._t1.is_identity())
        self.assertEqual(self._t2.get_elements(), (1,2,3,4,5,6))
        self._t2.append_transform(self._t1)
        self.assertEqual(self._t2.get_elements(), (1,2,3,4,5,6))

    def testPrependIdentity(self):
        self._t1.set_identity()
        self._t2.set_elements(1,2,3,4,5,6)
        self.assert_(self._t1.is_identity())
        self.assertEqual(self._t2.get_elements(), (1,2,3,4,5,6))
        self._t2.prepend_transform(self._t1)
        self.assertEqual(self._t2.get_elements(), (1,2,3,4,5,6))

    def testAppend1(self):
        self._t1.set_elements(6,5,4,3,2,1)
        self._t2.set_elements(1,2,3,4,5,6)
        self.assertEqual(self._t1.get_elements(), (6,5,4,3,2,1))
        self.assertEqual(self._t2.get_elements(), (1,2,3,4,5,6))
        self._t2.append_transform(self._t1)
        self.assertEqual(self._t1.get_elements(), (6,5,4,3,2,1))
        self.assertEqual(self._t2.get_elements(), (21,32,13,20,62,39))

    def testPrepend1(self):
        self._t1.set_elements(6,5,4,3,2,1)
        self._t2.set_elements(1,2,3,4,5,6)
        self.assertEqual(self._t1.get_elements(), (6,5,4,3,2,1))
        self.assertEqual(self._t2.get_elements(), (1,2,3,4,5,6))
        self._t2.prepend_transform(self._t1)
        self.assertEqual(self._t1.get_elements(), (6,5,4,3,2,1))
        self.assertEqual(self._t2.get_elements(), (14,11,34,27,9,16))

    def testPrepend2(self):
        self._t1.set_elements(6,5,4,3,2,1)
        self._t2.set_elements(1,2,3,4,5,6)
        self.assertEqual(self._t1.get_elements(), (6,5,4,3,2,1))
        self.assertEqual(self._t2.get_elements(), (1,2,3,4,5,6))
        self._t1.prepend_transform(self._t2)
        self.assertEqual(self._t1.get_elements(), (21,32,13,20,62,39))
        self.assertEqual(self._t2.get_elements(), (1,2,3,4,5,6))

    def testAppend2(self):
        self._t1.set_elements(6,5,4,3,2,1)
        self._t2.set_elements(1,2,3,4,5,6)
        self.assertEqual(self._t1.get_elements(), (6,5,4,3,2,1))
        self.assertEqual(self._t2.get_elements(), (1,2,3,4,5,6))
        self._t1.append_transform(self._t2)
        self.assertEqual(self._t1.get_elements(), (14,11,34,27,9,16))
        self.assertEqual(self._t2.get_elements(), (1,2,3,4,5,6))

class Inverse(unittest.TestCase):
    def setUp(self):
        self._t1 = AffineTransform()
        self._t2 = AffineTransform()
        self.failIfEqual(self._t1, None)
        self.assert_(self._t1.is_identity())
        self.failIfEqual(self._t2, None)
        self.assert_(self._t2.is_identity())

    def tearDown(self):
        self._t1 = None
        self._t2 = None

    def testInvertLeft(self):
        self._t1.set_elements(1,2,3,4,5,6)
        self._t2.set_transform(self._t1)
        self.assertEqual(self._t1.get_elements(), (1,2,3,4,5,6))
        self.assertEqual(self._t2.get_elements(), (1,2,3,4,5,6))
        self.assert_(self._t1.invert())
        self._t2.append_transform(self._t1)
        self.assert_(self._t2.is_identity())

    def testInvertRight(self):
        self._t1.set_elements(1,2,3,4,5,6)
        self._t2.set_transform(self._t1)
        self.assertEqual(self._t1.get_elements(), (1,2,3,4,5,6))
        self.assertEqual(self._t2.get_elements(), (1,2,3,4,5,6))
        self.assert_(self._t1.invert())
        self._t2.prepend_transform(self._t1)
        self.assert_(self._t2.is_identity())

    def testSingular(self):
        self._t1.set_elements(1,0,3,0,5,6)
        self.assert_(not self._t1.invert())

class TestTransform(unittest.TestCase):
    def setUp(self):
        self._t = AffineTransform()
        self.failIfEqual(self._t, None)
        self.assert_(self._t.is_identity())

    def tearDown(self):
        self._t = None

    def testPoint1(self):
        self._t.set_identity()
        self.assertEqual(self._t.transform_point(4,5), (4,5))

    def testPoint2(self):
        self._t.set_elements(1,2,3,4,5,6)
        self.assertEqual(self._t.transform_point(4,5), (19,38))

    def testSize1(self):
        self._t.set_identity()
        self.assertEqual(self._t.transform_size(4,5), (4,5))

    def testSize2(self):
        self._t.set_elements(1,2,3,4,5,6)
        self.assertEqual(self._t.transform_size(4,5), (14,32))

# vim:sw=4:ts=4:et:autoindent

