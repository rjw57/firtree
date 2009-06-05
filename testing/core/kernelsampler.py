import unittest
import gobject
from pyfirtree import *

class Creation(unittest.TestCase):
    def setUp(self):
        self._s = KernelSampler()
        self.failIfEqual(self._s, None)
        self.assertEqual(self._s.get_kernel(), None)

    def tearDown(self):
        self._s = None

    def testDissociateKernel(self):
        self._s.set_kernel(None)
        self.assertEqual(self._s.get_kernel(), None)

    def testSetKernel(self):
        self._s.set_kernel(None)
        self.assertEqual(self._s.get_kernel(), None)
        k = Kernel()
        self._s.set_kernel(k)
        self.assertEqual(self._s.get_kernel(), k)
        self._s.set_kernel(None)
        self.assertEqual(self._s.get_kernel(), None)

# vim:sw=4:ts=4:et:autoindent

