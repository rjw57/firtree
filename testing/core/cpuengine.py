import unittest
import gobject
import gtk.gdk
import cairo
from pyfirtree import *

from utils import FirtreeTestCase

width = 190
height = 120

class Creation(unittest.TestCase):
    def setUp(self):
        self._e = CpuRenderer()
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

class CairoARGBSurface(FirtreeTestCase):
    def setUp(self):
        self._e = CpuRenderer()
        self.failIfEqual(self._e, None)
        self.assertEqual(self._e.get_sampler(), None)
        self._s = cairo.ImageSurface(cairo.FORMAT_ARGB32, width, height)

    def tearDown(self):
        self._e = None
        self._s = None

    def testSimpleKernel(self):
        k = Kernel()
        k.compile_from_source('kernel vec4 red() { return vec4(1,0,0,1); }')
        self.assertKernelCompiled(k)
        self.assert_(k.is_valid())

        ks = KernelSampler()
        ks.set_kernel(k)
        self._e.set_sampler(ks)

        rv = self._e.render_into_cairo_surface((0, 0, width, height), self._s)
        self.assertCairoSurfaceMatches(self._s, 'cairo-argb-simple')

    def testMultipleEngine(self):
        k = Kernel()
        k.compile_from_source('kernel vec4 red() { return vec4(1,0,0,1); }')
        self.assertKernelCompiled(k)
        self.assert_(k.is_valid())

        ks = KernelSampler()
        ks.set_kernel(k)
        self._e.set_sampler(ks)

        engine_1 = CpuRenderer()
        engine_1.set_sampler(ks)

        k2 = Kernel()
        k2.compile_from_source('kernel vec4 green() { return vec4(0,1,0,1); }')
        self.assertKernelCompiled(k2)
        self.assert_(k2.is_valid())

        ks2 = KernelSampler()
        ks2.set_kernel(k2)

        engine_2 = CpuRenderer()
        engine_2.set_sampler(ks2)

        for run in xrange(2):
            rv = self._e.render_into_cairo_surface((0, 0, width, height), self._s)
            self.assertCairoSurfaceMatches(self._s, 'cairo-multiple-engine-1')

            rv = engine_1.render_into_cairo_surface((0, 0, width, height), self._s)
            self.assertCairoSurfaceMatches(self._s, 'cairo-multiple-engine-2')

            rv = engine_2.render_into_cairo_surface((0, 0, width, height), self._s)
            self.assertCairoSurfaceMatches(self._s, 'cairo-multiple-engine-3')

    def testSimpleAlphaKernel(self):
        k = Kernel()
        k.compile_from_source('kernel vec4 red() { return 0.5*vec4(1,0,0,1); }')
        self.assertKernelCompiled(k)
        self.assert_(k.is_valid())

        ks = KernelSampler()
        ks.set_kernel(k)
        self._e.set_sampler(ks)

        cr = cairo.Context(self._s)
        cr.set_source_rgba(0,1,0,0.75)
        cr.paint()
        cr = None

        rv = self._e.render_into_cairo_surface((0, 0, width, height), self._s)
        self.assertCairoSurfaceMatches(self._s, 'cairo-argb-simple-alpha')

class CairoRGBSurface(FirtreeTestCase):
    def setUp(self):
        self._e = CpuRenderer()
        self.failIfEqual(self._e, None)
        self.assertEqual(self._e.get_sampler(), None)
        self._s = cairo.ImageSurface(cairo.FORMAT_RGB24, width, height)

    def tearDown(self):
        self._e = None
        self._s = None

    def testSimpleKernel(self):
        k = Kernel()
        k.compile_from_source('kernel vec4 red() { return vec4(1,0,0,1); }')
        self.assertKernelCompiled(k)
        self.assert_(k.is_valid())

        ks = KernelSampler()
        ks.set_kernel(k)
        self._e.set_sampler(ks)

        cr = cairo.Context(self._s)
        cr.set_source_rgba(0,0,0,0)
        cr.paint()
        cr = None

        rv = self._e.render_into_cairo_surface((0, 0, width, height), self._s)
        self.assertCairoSurfaceMatches(self._s, 'cairo-rgb-simple')

    def testSimpleAlphaKernel(self):
        k = Kernel()
        k.compile_from_source('kernel vec4 red() { return 0.5*vec4(1,0,0,1); }')
        self.assertKernelCompiled(k)
        self.assert_(k.is_valid())

        ks = KernelSampler()
        ks.set_kernel(k)
        self._e.set_sampler(ks)

        cr = cairo.Context(self._s)
        cr.set_source_rgba(0,1,0,0.75)
        cr.paint()
        cr = None

        rv = self._e.render_into_cairo_surface((0, 0, width, height), self._s)
        self.assertCairoSurfaceMatches(self._s, 'cairo-rgb-simple-alpha')

class GdkPixbufARGBSurface(FirtreeTestCase):
    def setUp(self):
        self._e = CpuRenderer()
        self.failIfEqual(self._e, None)
        self.assertEqual(self._e.get_sampler(), None)
        self._pb = gtk.gdk.Pixbuf(gtk.gdk.COLORSPACE_RGB, True, 8, width, height)

    def tearDown(self):
        self._e = None
        self._pb = None

    def testSimpleKernel(self):
        k = Kernel()
        k.compile_from_source('kernel vec4 red() { return vec4(1,0,0,1); }')
        self.assertKernelCompiled(k)
        self.assert_(k.is_valid())

        ks = KernelSampler()
        ks.set_kernel(k)
        self._e.set_sampler(ks)

        self._pb.fill(0x0)
        rv = self._e.render_into_pixbuf((0, 0, width, height), self._pb)
        self.assertPixbufMatches(self._pb, 'pixbuf-argb-simple')

    def testSimpleAlphaKernel(self):
        k = Kernel()
        k.compile_from_source('kernel vec4 red() { return 0.5*vec4(1,0,0,1); }')
        self.assertKernelCompiled(k)
        self.assert_(k.is_valid())

        ks = KernelSampler()
        ks.set_kernel(k)
        self._e.set_sampler(ks)

        self._pb.fill(0x00ff00c0) # 75% green
        rv = self._e.render_into_pixbuf((0, 0, width, height), self._pb)
        self.assertPixbufMatches(self._pb, 'pixbuf-argb-simple-alpha')

class GdkPixbufRGBSurface(FirtreeTestCase):
    def setUp(self):
        self._e = CpuRenderer()
        self.failIfEqual(self._e, None)
        self.assertEqual(self._e.get_sampler(), None)
        self._pb = gtk.gdk.Pixbuf(gtk.gdk.COLORSPACE_RGB, False, 8, width, height)

    def tearDown(self):
        self._e = None
        self._pb = None

    def testSimpleKernel(self):
        k = Kernel()
        k.compile_from_source('kernel vec4 red() { return vec4(1,0,0,1); }')
        self.assertKernelCompiled(k)
        self.assert_(k.is_valid())

        ks = KernelSampler()
        ks.set_kernel(k)
        self._e.set_sampler(ks)

        self._pb.fill(0x0)
        rv = self._e.render_into_pixbuf((0, 0, width, height), self._pb)
        self.assertPixbufMatches(self._pb, 'pixbuf-rgb-simple')

    def testSimpleAlphaKernel(self):
        k = Kernel()
        k.compile_from_source('kernel vec4 red() { return 0.5*vec4(1,0,0,1); }')
        self.assertKernelCompiled(k)
        self.assert_(k.is_valid())

        ks = KernelSampler()
        ks.set_kernel(k)
        self._e.set_sampler(ks)

        self._pb.fill(0x00c00000) # 75% green 
        rv = self._e.render_into_pixbuf((0, 0, width, height), self._pb)
        self.assertPixbufMatches(self._pb, 'pixbuf-rgb-simple-alpha')

class KernelTests(FirtreeTestCase):
    def setUp(self):
        self._e = CpuRenderer()
        self.failIfEqual(self._e, None)
        self.assertEqual(self._e.get_sampler(), None)
        self._s = cairo.ImageSurface(cairo.FORMAT_ARGB32, width, height)

    def tearDown(self):
        self._e = None
        self._s = None

    def clearSurface(self):
        cr = cairo.Context(self._s)
        cr.set_source_rgba(0,0,0,1)
        cr.paint()
        cr = None

    def clearSurfaceRGBA(self,r,g,b,a):
        cr = cairo.Context(self._s)
        cr.set_source_rgba(r,g,b,a)
        cr.paint()
        cr = None

    def testSin(self):
        k = Kernel()
        k.compile_from_source('''
            kernel vec4 sinKernel() {
                return vec4(0.5 + 0.5 * sin(destCoord() * 0.1), 0, 1);
            }
            ''')
        self.assertKernelCompiled(k)
        self.assert_(k.is_valid())

        ks = KernelSampler()
        ks.set_kernel(k)
        self._e.set_sampler(ks)

        self.clearSurface()

        rv = self._e.render_into_cairo_surface((0, 0, width, height), self._s)
        self.assertCairoSurfaceMatches(self._s, 'cpu-sin-1')

    def testDistorted(self):
        dist_k = Kernel()
        dist_k.compile_from_source('''
            kernel vec4 distKernel(sampler src) {
                vec2 sample_coord = destCoord() +
                    10 * sin(destCoord().yx * 0.05);
                return sample(src, samplerTransform(src, sample_coord));
            }''')
        self.assertKernelCompiled(dist_k)

        k = Kernel()
        k.compile_from_source('''
            kernel vec4 sinKernel() {
                return vec4(0.5 + 0.5 * sin(destCoord() * 0.2), 0, 1);
            }
            ''')
        self.assertKernelCompiled(k)
        self.assert_(k.is_valid())

        ks = KernelSampler()
        ks.set_kernel(k)

        dist_k['src'] = ks
        self.assert_(dist_k.is_valid())

        dks = KernelSampler()
        dks.set_kernel(dist_k)
        self._e.set_sampler(dks)

        self.clearSurface()

        rv = self._e.render_into_cairo_surface((0, 0, width, height), self._s)
        self.assertCairoSurfaceMatches(self._s, 'cpu-sin-2')

    def testDistortedAndInverted(self):
        inv_k = Kernel()
        inv_k.compile_from_source('''
            kernel vec4 invKernel(sampler src) {
                vec4 incol = sample(src, samplerCoord(src));
                incol = unpremultiply(incol);
                incol.rgb = 1 - incol.rgb;
                return premultiply(incol);
            }
        ''')
        self.assertKernelCompiled(inv_k)

        dist_k = Kernel()
        dist_k.compile_from_source('''
            kernel vec4 distKernel(sampler src) {
                vec2 sample_coord = destCoord() +
                    10 * sin(destCoord().yx * 0.05);
                return sample(src, samplerTransform(src, sample_coord));
            }''')
        self.assertKernelCompiled(dist_k)

        k = Kernel()
        k.compile_from_source('''
            kernel vec4 sinKernel() {
                float alpha = 0.5 + 0.5*cos(destCoord().y * 0.05);
                return premultiply(
                    vec4(0.5 + 0.5 * sin(destCoord() * 0.2), 0, alpha));
            }
            ''')
        self.assertKernelCompiled(k)
        self.assert_(k.is_valid())

        ks = KernelSampler()
        ks.set_kernel(k)

        dist_k['src'] = ks
        self.assert_(dist_k.is_valid())

        dks = KernelSampler()
        dks.set_kernel(dist_k)

        inv_k['src'] = dks
        self.assert_(inv_k.is_valid())

        iks = KernelSampler()
        iks.set_kernel(inv_k)
        self._e.set_sampler(iks)

        self.clearSurfaceRGBA(1,0,0,1)

        rv = self._e.render_into_cairo_surface((0, 0, width, height), self._s)
        self.assertCairoSurfaceMatches(self._s, 'cpu-sin-3')

# vim:sw=4:ts=4:et:autoindent

