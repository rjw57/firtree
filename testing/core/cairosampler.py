import unittest
import gobject
import cairo
from pyfirtree import *

from utils import FirtreeTestCase

width = 190
height = 120

class Creation(FirtreeTestCase):
    def setUp(self):
        self._s = CairoSurfaceSampler()
        self.failIfEqual(self._s, None)
        self.assertEqual(self._s.get_cairo_surface(), None)

        self._source_surface = cairo.ImageSurface(cairo.FORMAT_ARGB32, 320, 240)

        cr = cairo.Context(self._source_surface)
        cr.set_source_rgba(0,0,1,0.2)
        cr.paint()
        cr.set_source_rgba(1,0,0,1)
        cr.move_to(0,0)
        cr.line_to(320,240)
        cr.move_to(0,240)
        cr.line_to(320,0)
        cr.stroke()

    def tearDown(self):
        self._s = None

    def testDissociateCairoSurface(self):
        self._s.set_cairo_surface(None)
        self.assertEqual(self._s.get_cairo_surface(), None)

    def testSetCairoSurface(self):
        self._s.set_cairo_surface(None)
        self.assertEqual(self._s.get_cairo_surface(), None)
        self._s.set_cairo_surface(self._source_surface)
        self.assertNotEqual(self._s.get_cairo_surface(), None)
        self._s.set_cairo_surface(None)
        self.assertEqual(self._s.get_cairo_surface(), None)

    def clearSurface(self, cs):
        cr = cairo.Context(cs)
        cr.set_source_rgba(0,0,0,0)
        cr.paint()
        cr = None

    def testSimpleRender1(self):
        self._s.set_cairo_surface(None)
        self.assertEqual(self._s.get_cairo_surface(), None)
        self._s.set_cairo_surface(self._source_surface)
        self.assertNotEqual(self._s.get_cairo_surface(), None)

        cs = cairo.ImageSurface(cairo.FORMAT_RGB24, width, height)
        self.clearSurface(cs)

        engine = CpuEngine()
        engine.set_sampler(self._s)

        rv = engine.render_into_cairo_surface((-10, -10, 310, 230), cs)
        self.assert_(rv)
        self.assertCairoSurfaceMatches(cs, 'cpu-cairo-1')
        
    def testSimpleRender2(self):
        self._s.set_cairo_surface(None)
        self.assertEqual(self._s.get_cairo_surface(), None)
        self._s.set_cairo_surface(self._source_surface)
        self.assertNotEqual(self._s.get_cairo_surface(), None)

        cs = cairo.ImageSurface(cairo.FORMAT_ARGB32, width, height)
        self.clearSurface(cs)

        engine = CpuEngine()
        engine.set_sampler(self._s)

        rv = engine.render_into_cairo_surface((-10, -10, 310, 230), cs)
        self.assert_(rv)
        self.assertCairoSurfaceMatches(cs, 'cpu-cairo-2')
 
    def testSimpleRender3(self):
        self._s.set_cairo_surface(None)
        self.assertEqual(self._s.get_cairo_surface(), None)
        self._s.set_cairo_surface(self._source_surface)
        self.assertNotEqual(self._s.get_cairo_surface(), None)

        cs = cairo.ImageSurface(cairo.FORMAT_ARGB32, width, height)
        self.clearSurface(cs)

        engine = CpuEngine()
        engine.set_sampler(self._s)

        rv = engine.render_into_cairo_surface((0, 0, 30, 30), cs)
        self.assert_(rv)
        self.assertCairoSurfaceMatches(cs, 'cpu-cairo-3')
 
    def testSimpleRender4(self):
        self._s.set_cairo_surface(None)
        self.assertEqual(self._s.get_cairo_surface(), None)
        self._s.set_cairo_surface(self._source_surface)
        self.assertNotEqual(self._s.get_cairo_surface(), None)

        cs = cairo.ImageSurface(cairo.FORMAT_ARGB32, width, height)
        self.clearSurface(cs)
        self._s.set_do_interpolation(True)

        engine = CpuEngine()
        engine.set_sampler(self._s)

        rv = engine.render_into_cairo_surface((0, 0, 30, 30), cs)
        self.assert_(rv)
        self.assertCairoSurfaceMatches(cs, 'cpu-cairo-4')
        self._s.set_do_interpolation(False)
 
    def testSimpleRender5(self):
        self._s.set_cairo_surface(None)
        self.assertEqual(self._s.get_cairo_surface(), None)
        self._s.set_cairo_surface(self._source_surface)
        self.assertNotEqual(self._s.get_cairo_surface(), None)

        cs = cairo.ImageSurface(cairo.FORMAT_ARGB32, width, height)
        self.clearSurface(cs)
        self._s.set_do_interpolation(True)

        engine = CpuEngine()
        engine.set_sampler(self._s)

        rv = engine.render_into_cairo_surface((-5, -5, 25, 25), cs)
        self.assert_(rv)
        self.assertCairoSurfaceMatches(cs, 'cpu-cairo-5')
        self._s.set_do_interpolation(False)
 
    def testSimpleRender6(self):
        self._s.set_cairo_surface(None)
        self.assertEqual(self._s.get_cairo_surface(), None)
        self._s.set_cairo_surface(self._source_surface)
        self.assertNotEqual(self._s.get_cairo_surface(), None)

        cs = cairo.ImageSurface(cairo.FORMAT_ARGB32, width, height)
        self.clearSurface(cs)
        self._s.set_do_interpolation(True)

        engine = CpuEngine()
        engine.set_sampler(self._s)

        rv = engine.render_into_cairo_surface((0, 0, width, height), cs)
        self.assert_(rv)
        self.assertCairoSurfaceMatches(cs, 'cpu-cairo-6')
        self._s.set_do_interpolation(False)
 
    def testSimpleRender7(self):
        self._s.set_cairo_surface(None)
        self.assertEqual(self._s.get_cairo_surface(), None)
        self._s.set_cairo_surface(self._source_surface)
        self.assertNotEqual(self._s.get_cairo_surface(), None)

        cs = cairo.ImageSurface(cairo.FORMAT_ARGB32, width, height)
        self.clearSurface(cs)
        self._s.set_do_interpolation(False)

        engine = CpuEngine()
        engine.set_sampler(self._s)

        rv = engine.render_into_cairo_surface((0, 0, width, height), cs)
        self.assert_(rv)
        self.assertCairoSurfaceMatches(cs, 'cpu-cairo-7')

    def testDistorted(self):
        dist_k = Kernel()
        dist_k.compile_from_source('''
            kernel vec4 distKernel(sampler src) {
                vec2 sample_coord = destCoord() +
                    10 * sin(destCoord().yx * 0.05);
                return sample(src, samplerTransform(src, sample_coord));
            }''')
        self.assertKernelCompiled(dist_k)

        self._s.set_cairo_surface(self._source_surface)
        self.assertNotEqual(self._s.get_cairo_surface(), None)

        dist_k['src'] = self._s
        self.assert_(dist_k.is_valid())

        dks = KernelSampler()
        dks.set_kernel(dist_k)

        cs = cairo.ImageSurface(cairo.FORMAT_ARGB32, width, height)
        self.clearSurface(cs)

        engine = CpuEngine()
        engine.set_sampler(self._s)
        engine.set_sampler(dks)

        rv = engine.render_into_cairo_surface((0, 0, 320, 240), cs)
        self.assert_(rv)
        self.assertCairoSurfaceMatches(cs, 'cpu-cairo-distort-1')

    def testDistorted(self):
        dist_k = Kernel()
        dist_k.compile_from_source('''
            kernel vec4 distKernel(sampler src) {
                vec2 sample_coord = destCoord() +
                    10 * sin(destCoord().yx * 0.05);
                return sample(src, samplerTransform(src, sample_coord));
            }''')
        self.assertKernelCompiled(dist_k)

        self._s.set_cairo_surface(self._source_surface)
        self.assertNotEqual(self._s.get_cairo_surface(), None)

        dist_k['src'] = self._s
        self.assert_(dist_k.is_valid())

        dks = KernelSampler()
        dks.set_kernel(dist_k)

        cs = cairo.ImageSurface(cairo.FORMAT_ARGB32, width, height)
        self.clearSurface(cs)

        self._s.set_do_interpolation(True)

        engine = CpuEngine()
        engine.set_sampler(self._s)
        engine.set_sampler(dks)

        rv = engine.render_into_cairo_surface((0, 0, 320, 240), cs)
        self.assert_(rv)
        self.assertCairoSurfaceMatches(cs, 'cpu-cairo-distort-2')
        self._s.set_do_interpolation(False)


# vim:sw=4:ts=4:et:autoindent

