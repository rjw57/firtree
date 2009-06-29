import unittest
import gobject
import Image
import cairo
from pyfirtree import *

from utils import FirtreeTestCase

width = 190
height = 120

class Creation(FirtreeTestCase):
    def setUp(self):
        self._s = BufferSampler()
        self.assertEqual(self._s.get_extent()[2], 0)
        self.assertEqual(self._s.get_extent()[3], 0)
        self.failIfEqual(self._s, None)

    def tearDown(self):
        self._s = None

    def clearSurface(self, cs):
        cr = cairo.Context(cs)
        cr.set_source_rgba(0,0,0,0)
        cr.paint()
        cr = None

    def testSimpleRender1(self):
        im = self.loadImage('painting.jpg')
        
        w = im.size[0]
        h = im.size[1]
        s = w * 3
        d = im.tostring()

        self._s.set_buffer(d, w, h, s, FORMAT_RGB24)

        cs = cairo.ImageSurface(cairo.FORMAT_RGB24, width, height)
        self.clearSurface(cs)

        engine = CpuEngine()
        engine.set_sampler(self._s)

        rv = engine.render_into_cairo_surface((-10, -10, 630, 470), cs)
        self.assert_(rv)
        self.assertCairoSurfaceMatches(cs, 'cpu-buffer-1')

# vim:sw=4:ts=4:et:autoindent

