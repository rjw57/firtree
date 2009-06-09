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

    def testComplexKernelSampler(self):
        ks = KernelSampler()
        self._e.set_sampler(ks)
        self.assertEqual(self._e.get_sampler(), ks)

        k = Kernel()
        k.compile_from_source("""
            kernel vec4 simple() { 
                vec4 out_vec = 0.5 + 0.5 * 
                    vec4(sincos(destCoord().x), cossin(destCoord().y));
                out_vec.a = 1;
                return out_vec;
            }""")
        self.assert_(k.get_compile_status())
        ks.set_kernel(k)

        # fixme. no way of testing this...

        # print(debug_dump_sampler_function(ks))

        pb = gtk.gdk.Pixbuf(gtk.gdk.COLORSPACE_RGB, True, 8, 512, 512)
        pb.fill(0x0000ff)
        pb.save('foo3.png', 'png')
        rv = self._e.render_into_pixbuf((0, 0, 51, 51), pb)
        self.assert_(rv)
        pb.save('foo4.png', 'png')

    def testMultipleKernelSampler(self):
        ks = KernelSampler()
        self._e.set_sampler(ks)
        self.assertEqual(self._e.get_sampler(), ks)

        k = Kernel()
        k.compile_from_source("""
            kernel vec4 simple(sampler src) { 
                vec4 src_val = sample(src, 
                    samplerTransform(src,
                        destCoord() + cos(0.2 * destCoord()*2)));
                return src_val;
            }""")
        if(not k.get_compile_status()):
            print(k.get_compile_log())
        self.assert_(k.get_compile_status())
        ks.set_kernel(k)

        k2 = Kernel()
        k2.compile_from_source("""
            kernel vec4 simple() { 
                return 0.75 * vec4(0.5 + 0.5 * sin(destCoord()), 0, 1);
            }""")
        if(not k2.get_compile_status()):
            print(k.get_compile_log())
        self.assert_(k2.get_compile_status())
        ks2 = KernelSampler()
        ks2.set_kernel(k2)

        k['src'] = ks2

        # print(debug_dump_sampler_function(ks))

        # fixme. no way of testing this...

        # print(debug_dump_sampler_function(ks))

        pb = gtk.gdk.Pixbuf(gtk.gdk.COLORSPACE_RGB, False, 8, 512, 512)
        pb.fill(0xff000000)
        pb.save('foo5.png', 'png')
        rv = self._e.render_into_pixbuf((0, 0, 51, 51), pb)
        self.assert_(rv)
        pb.save('foo6.png', 'png')

# vim:sw=4:ts=4:et:autoindent

