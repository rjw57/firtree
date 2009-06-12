import unittest
import gobject
import cairo
import clutter
from pyfirtree import *

from utils import FirtreeTestCase

width = 190
height = 120

class Creation(FirtreeTestCase):
    def setUp(self):
        self._s = CoglTextureSampler()
        self.failIfEqual(self._s, None)
        self.assertEqual(self._s.get_clutter_texture(), None)

        self._source_texture = clutter.texture_new_from_file('../artwork/bricks.jpg')

    def tearDown(self):
        self._s = None
        self._source_texture = None

    def testDissociateCoglTexture(self):
        self._s.set_clutter_texture(None)
        self.assertEqual(self._s.get_clutter_texture(), None)

    def testSetCoglTexture(self):
        self._s.set_clutter_texture(None)
        self.assertEqual(self._s.get_clutter_texture(), None)
        self._s.set_clutter_texture(self._source_texture)
        self.assertNotEqual(self._s.get_clutter_texture(), None)
        self._s.set_clutter_texture(None)
        self.assertEqual(self._s.get_clutter_texture(), None)

    def clearSurface(self, cs):
        cr = cairo.Context(cs)
        cr.set_source_rgba(0,0,0,0)
        cr.paint()
        cr = None

    def testSimpleRender1(self):
        self._s.set_clutter_texture(None)
        self.assertEqual(self._s.get_clutter_texture(), None)
        self._s.set_clutter_texture(self._source_texture)
        self.assertNotEqual(self._s.get_clutter_texture(), None)

        cs = cairo.ImageSurface(cairo.FORMAT_RGB24, width, height)
        self.clearSurface(cs)

        engine = CpuEngine()
        engine.set_sampler(self._s)

        rv = engine.render_into_cairo_surface((-10, -10, 310, 230), cs)
        self.assert_(rv)
        self.assertCairoSurfaceMatches(cs, 'cpu-clutter-1')
        
    def testSimpleRender2(self):
        self._s.set_clutter_texture(None)
        self.assertEqual(self._s.get_clutter_texture(), None)
        self._s.set_clutter_texture(self._source_texture)
        self.assertNotEqual(self._s.get_clutter_texture(), None)

        cs = cairo.ImageSurface(cairo.FORMAT_ARGB32, width, height)
        self.clearSurface(cs)

        engine = CpuEngine()
        engine.set_sampler(self._s)

        rv = engine.render_into_cairo_surface((-10, -10, 310, 230), cs)
        self.assert_(rv)
        self.assertCairoSurfaceMatches(cs, 'cpu-clutter-2')
 
    def testSimpleRender3(self):
        self._s.set_clutter_texture(None)
        self.assertEqual(self._s.get_clutter_texture(), None)
        self._s.set_clutter_texture(self._source_texture)
        self.assertNotEqual(self._s.get_clutter_texture(), None)

        cs = cairo.ImageSurface(cairo.FORMAT_ARGB32, width, height)
        self.clearSurface(cs)

        engine = CpuEngine()
        engine.set_sampler(self._s)

        rv = engine.render_into_cairo_surface((0, 0, 30, 30), cs)
        self.assert_(rv)
        self.assertCairoSurfaceMatches(cs, 'cpu-clutter-3')
 
    def testSimpleRender4(self):
        self._s.set_clutter_texture(None)
        self.assertEqual(self._s.get_clutter_texture(), None)
        self._s.set_clutter_texture(self._source_texture)
        self.assertNotEqual(self._s.get_clutter_texture(), None)

        cs = cairo.ImageSurface(cairo.FORMAT_ARGB32, width, height)
        self.clearSurface(cs)

        engine = CpuEngine()
        engine.set_sampler(self._s)

        rv = engine.render_into_cairo_surface((0, 0, 30, 30), cs)
        self.assert_(rv)
        self.assertCairoSurfaceMatches(cs, 'cpu-clutter-4')
 
    def testSimpleRender5(self):
        self._s.set_clutter_texture(None)
        self.assertEqual(self._s.get_clutter_texture(), None)
        self._s.set_clutter_texture(self._source_texture)
        self.assertNotEqual(self._s.get_clutter_texture(), None)

        cs = cairo.ImageSurface(cairo.FORMAT_ARGB32, width, height)
        self.clearSurface(cs)

        engine = CpuEngine()
        engine.set_sampler(self._s)

        rv = engine.render_into_cairo_surface((-5, -5, 25, 25), cs)
        self.assert_(rv)
        self.assertCairoSurfaceMatches(cs, 'cpu-clutter-5')
 
    def testSimpleRender6(self):
        self._s.set_clutter_texture(None)
        self.assertEqual(self._s.get_clutter_texture(), None)
        self._s.set_clutter_texture(self._source_texture)
        self.assertNotEqual(self._s.get_clutter_texture(), None)

        cs = cairo.ImageSurface(cairo.FORMAT_ARGB32, width, height)
        self.clearSurface(cs)

        engine = CpuEngine()
        engine.set_sampler(self._s)

        rv = engine.render_into_cairo_surface((0, 0, width, height), cs)
        self.assert_(rv)
        self.assertCairoSurfaceMatches(cs, 'cpu-clutter-6')
 
    def testSimpleRender7(self):
        self._s.set_clutter_texture(None)
        self.assertEqual(self._s.get_clutter_texture(), None)
        self._s.set_clutter_texture(self._source_texture)
        self.assertNotEqual(self._s.get_clutter_texture(), None)

        cs = cairo.ImageSurface(cairo.FORMAT_ARGB32, width, height)
        self.clearSurface(cs)

        engine = CpuEngine()
        engine.set_sampler(self._s)

        rv = engine.render_into_cairo_surface((0, 0, width, height), cs)
        self.assert_(rv)
        self.assertCairoSurfaceMatches(cs, 'cpu-clutter-7')

    def testDistorted(self):
        dist_k = Kernel()
        dist_k.compile_from_source('''
            kernel vec4 distKernel(sampler src) {
                vec2 sample_coord = destCoord() +
                    10 * sin(destCoord().yx * 0.05);
                return sample(src, samplerTransform(src, sample_coord));
            }''')
        self.assertKernelCompiled(dist_k)

        self._s.set_clutter_texture(self._source_texture)
        self.assertNotEqual(self._s.get_clutter_texture(), None)

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
        self.assertCairoSurfaceMatches(cs, 'cpu-clutter-distort-1')

    def testDistorted(self):
        dist_k = Kernel()
        dist_k.compile_from_source('''
            kernel vec4 distKernel(sampler src) {
                vec2 sample_coord = destCoord() +
                    10 * sin(destCoord().yx * 0.05);
                return sample(src, samplerTransform(src, sample_coord));
            }''')
        self.assertKernelCompiled(dist_k)

        self._s.set_clutter_texture(self._source_texture)
        self.assertNotEqual(self._s.get_clutter_texture(), None)

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
        self.assertCairoSurfaceMatches(cs, 'cpu-clutter-distort-2')


# vim:sw=4:ts=4:et:autoindent

