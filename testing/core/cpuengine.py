import unittest
import gobject
import gtk.gdk
from pyfirtree import *

class Creation(unittest.TestCase):
    def setUp(self):
        self._e = CpuEngine()
        self.failIfEqual(self._e, None)
        self.assertEqual(self._e.get_sampler(), None)

    def tearDown(self):
        self._e = None

    def testDissociateKernel(self):
        self._e.set_sampler(None)
        self.assertEqual(self._e.get_sampler(), None)

    def testSetKernel(self):
        self._e.set_sampler(None)
        self.assertEqual(self._e.get_sampler(), None)
        s = Sampler()
        self._e.set_sampler(s)
        self.assertEqual(self._e.get_sampler(), s)
        self._e.set_sampler(None)
        self.assertEqual(self._e.get_sampler(), None)

class RenderKernelSampler(unittest.TestCase):
    def setUp(self):
        self._e = CpuEngine()
        self.failIfEqual(self._e, None)
        self.assertEqual(self._e.get_sampler(), None)

    def tearDown(self):
        self._e = None

    def testKernelSampler(self):
        ks = KernelSampler()
        self._e.set_sampler(ks)
        self.assertEqual(self._e.get_sampler(), ks)

        k = Kernel()
        k.compile_from_source("""
            kernel vec4 simple() { 
                return vec4(destCoord().yx, 0.5, 1);
            }""")
        self.assert_(k.get_compile_status())
        ks.set_kernel(k)

        # fixme. no way of testing this...

        # print(debug_dump_sampler_function(ks))

        pb = gtk.gdk.Pixbuf(gtk.gdk.COLORSPACE_RGB, True, 8, 512, 512)
        pb.fill(0x0000ff)
        pb.save('foo1.png', 'png')
        rv = self._e.render_into_pixbuf((0, 0, 1, 1), pb)
        self.assert_(rv)
        pb.save('foo2.png', 'png')

# vim:sw=4:ts=4:et:autoindent

