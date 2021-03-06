import unittest
import gobject
import gtk.gdk
import cairo
from pyfirtree import *

from utils import FirtreeTestCase

width = 190
height = 120

class Creation(FirtreeTestCase):
    def setUp(self):
        self._s = PixbufSampler()
        self.assertEqual(self._s.get_extent()[2], 0)
        self.assertEqual(self._s.get_extent()[3], 0)
        self.failIfEqual(self._s, None)
        self.assertEqual(self._s.get_pixbuf(), None)

    def tearDown(self):
        self._s = None

    def testDissociatePixbuf(self):
        self._s.set_pixbuf(None)
        self.assertEqual(self._s.get_pixbuf(), None)

    def testSetPixbuf(self):
        self._s.set_pixbuf(None)
        self.assertEqual(self._s.get_pixbuf(), None)
        pb = gtk.gdk.Pixbuf(gtk.gdk.COLORSPACE_RGB, True, 8, width, height)
        self._s.set_pixbuf(pb)
        self.assertEqual(self._s.get_extent()[2], width)
        self.assertEqual(self._s.get_extent()[3], height)
        self.assertEqual(self._s.get_pixbuf(), pb)
        self._s.set_pixbuf(None)
        self.assertEqual(self._s.get_pixbuf(), None)

    def clearSurface(self, cs):
        cr = cairo.Context(cs)
        cr.set_source_rgba(0,0,0,0)
        cr.paint()
        cr = None

    def testSimpleRender1(self):
        self._s.set_pixbuf(None)
        self.assertEqual(self._s.get_pixbuf(), None)
        pb = self.loadPixbuf('painting.jpg')
        self._s.set_pixbuf(pb)
        self.assertEqual(self._s.get_pixbuf(), pb)

        cs = cairo.ImageSurface(cairo.FORMAT_RGB24, width, height)
        self.clearSurface(cs)

        engine = CpuRenderer()
        engine.set_sampler(self._s)

        rv = engine.render_into_cairo_surface((-10, -10, 630, 470), cs)
        self.assert_(rv)
        self.assertCairoSurfaceMatches(cs, 'cpu-pixbuf-1')
        
    def testSimpleRender2(self):
        self._s.set_pixbuf(None)
        self.assertEqual(self._s.get_pixbuf(), None)
        pb = self.loadPixbuf('painting.jpg')
        self._s.set_pixbuf(pb)
        self.assertEqual(self._s.get_pixbuf(), pb)

        cs = cairo.ImageSurface(cairo.FORMAT_ARGB32, width, height)
        self.clearSurface(cs)

        engine = CpuRenderer()
        engine.set_sampler(self._s)

        rv = engine.render_into_cairo_surface((-10, -10, 630, 470), cs)
        self.assert_(rv)
        self.assertCairoSurfaceMatches(cs, 'cpu-pixbuf-2')
 
    def testSimpleRender3(self):
        self._s.set_pixbuf(None)
        self.assertEqual(self._s.get_pixbuf(), None)
        pb = self.loadPixbuf('bricks.jpg')
        self._s.set_pixbuf(pb)
        self.assertEqual(self._s.get_pixbuf(), pb)

        cs = cairo.ImageSurface(cairo.FORMAT_ARGB32, width, height)
        self.clearSurface(cs)

        engine = CpuRenderer()
        engine.set_sampler(self._s)

        rv = engine.render_into_cairo_surface((0, 0, 30, 30), cs)
        self.assert_(rv)
        self.assertCairoSurfaceMatches(cs, 'cpu-pixbuf-3')
 
    def testSimpleRender4(self):
        self._s.set_pixbuf(None)
        self.assertEqual(self._s.get_pixbuf(), None)
        pb = self.loadPixbuf('bricks.jpg')
        self._s.set_pixbuf(pb)
        self.assertEqual(self._s.get_pixbuf(), pb)

        cs = cairo.ImageSurface(cairo.FORMAT_ARGB32, width, height)
        self.clearSurface(cs)
        self._s.set_do_interpolation(True)

        engine = CpuRenderer()
        engine.set_sampler(self._s)

        rv = engine.render_into_cairo_surface((0, 0, 30, 30), cs)
        self.assert_(rv)
        self.assertCairoSurfaceMatches(cs, 'cpu-pixbuf-4')
        self._s.set_do_interpolation(False)
 
    def testSimpleRender5(self):
        self._s.set_pixbuf(None)
        self.assertEqual(self._s.get_pixbuf(), None)
        pb = self.loadPixbuf('bricks.jpg')
        self._s.set_pixbuf(pb)
        self.assertEqual(self._s.get_pixbuf(), pb)

        cs = cairo.ImageSurface(cairo.FORMAT_ARGB32, width, height)
        self.clearSurface(cs)
        self._s.set_do_interpolation(True)

        engine = CpuRenderer()
        engine.set_sampler(self._s)

        rv = engine.render_into_cairo_surface((-5, -5, 25, 25), cs)
        self.assert_(rv)
        self.assertCairoSurfaceMatches(cs, 'cpu-pixbuf-5')
        self._s.set_do_interpolation(False)
 
    def testSimpleRender6(self):
        self._s.set_pixbuf(None)
        self.assertEqual(self._s.get_pixbuf(), None)
        pb = self.loadPixbuf('bricks.jpg')
        self._s.set_pixbuf(pb)
        self.assertEqual(self._s.get_pixbuf(), pb)

        cs = cairo.ImageSurface(cairo.FORMAT_ARGB32, width, height)
        self.clearSurface(cs)
        self._s.set_do_interpolation(True)

        engine = CpuRenderer()
        engine.set_sampler(self._s)

        rv = engine.render_into_cairo_surface((0, 0, width, height), cs)
        self.assert_(rv)
        self.assertCairoSurfaceMatches(cs, 'cpu-pixbuf-6')
        self._s.set_do_interpolation(False)
 
    def testSimpleRender7(self):
        self._s.set_pixbuf(None)
        self.assertEqual(self._s.get_pixbuf(), None)
        pb = self.loadPixbuf('bricks.jpg')
        self._s.set_pixbuf(pb)
        self.assertEqual(self._s.get_pixbuf(), pb)

        cs = cairo.ImageSurface(cairo.FORMAT_ARGB32, width, height)
        self.clearSurface(cs)
        self._s.set_do_interpolation(False)

        engine = CpuRenderer()
        engine.set_sampler(self._s)

        rv = engine.render_into_cairo_surface((0, 0, width, height), cs)
        self.assert_(rv)
        self.assertCairoSurfaceMatches(cs, 'cpu-pixbuf-7')

    def testDistorted(self):
        dist_k = Kernel()
        dist_k.compile_from_source('''
            kernel vec4 distKernel(sampler src) {
                vec2 sample_coord = destCoord() +
                    10 * sin(destCoord().yx * 0.05);
                return sample(src, samplerTransform(src, sample_coord));
            }''')
        self.assertKernelCompiled(dist_k)

        pb = self.loadPixbuf('painting.jpg')
        self._s.set_pixbuf(pb)
        self.assertEqual(self._s.get_pixbuf(), pb)

        dist_k['src'] = self._s
        self.assert_(dist_k.is_valid())

        dks = KernelSampler()
        dks.set_kernel(dist_k)

        cs = cairo.ImageSurface(cairo.FORMAT_ARGB32, width, height)
        self.clearSurface(cs)

        engine = CpuRenderer()
        engine.set_sampler(self._s)
        engine.set_sampler(dks)

        rv = engine.render_into_cairo_surface((0, 0, 640, 480), cs)
        self.assert_(rv)
        self.assertCairoSurfaceMatches(cs, 'cpu-pixbuf-distort-1')


# vim:sw=4:ts=4:et:autoindent

