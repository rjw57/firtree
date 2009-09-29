import unittest
import gobject
import Image
import cairo
import array
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

        engine = CpuRenderer()
        engine.set_sampler(self._s)

        rv = engine.render_into_cairo_surface((-10, -10, 630, 470), cs)
        self.assert_(rv)
        self.assertCairoSurfaceMatches(cs, 'cpu-buffer-1')

class RenderIntoBuffer(FirtreeTestCase):
    def setUp(self):
        self._source = BufferSampler()
        self.failIfEqual(self._source, None)
        self.assertEqual(self._source.get_extent()[2], 0)
        self.assertEqual(self._source.get_extent()[3], 0)

        im = self.loadImage('painting.jpg')        
        w = im.size[0]
        h = im.size[1]
        s = w * 3
        self._source_buffer = im.tostring()
        self._source.set_buffer(self._source_buffer, w, h, s, FORMAT_RGB24)

    def tearDown(self):
        self._source = None

    def clearSurface(self, cs):
        cr = cairo.Context(cs)
        cr.set_source_rgba(0,0,0,0)
        cr.paint()
        cr = None

    def checkBufferContents(self, tag, buffer, w, h, s, f):
        bs = BufferSampler()
        bs.set_buffer(buffer, w, h, s, f)

        cs = cairo.ImageSurface(cairo.FORMAT_ARGB32, w, h)
        self.clearSurface(cs)

        e = CpuRenderer()
        e.set_sampler(bs)

        rv = e.render_into_cairo_surface((0,0,w,h),cs)
        self.assert_(rv)
        self.assertCairoSurfaceMatches(cs, tag)

    def performFormatTest(self, stride, format, format_tag):
        size = stride * height
        out_buffer = array.array('B', (0x80,) * size)

        engine = CpuRenderer()
        engine.set_sampler(self._source)

        rv = engine.render_into_buffer((-10, -10, 630, 470), out_buffer, 
            width, height, stride, format)
        self.assert_(rv)

        self.checkBufferContents('cpu-buffer-%s' % format_tag, 
            out_buffer, width, height, stride, format)

    def testRgb24(self):
        stride = width * 3
        format = FORMAT_RGB24
        self.performFormatTest(stride, format, 'rgb24')

    def testBgr24(self):
        stride = width * 3
        format = FORMAT_BGR24
        self.performFormatTest(stride, format, 'bgr24')

    def testRgbx32(self):
        stride = width * 4
        format = FORMAT_RGBX32
        self.performFormatTest(stride, format, 'rgbx32')

    def testXbgr32(self):
        stride = width * 4
        format = FORMAT_XBGR32
        self.performFormatTest(stride, format, 'xbgr32')

    def testXrgb32(self):
        stride = width * 4
        format = FORMAT_XRGB32
        self.performFormatTest(stride, format, 'xrgb32')

    def testBgrx32(self):
        stride = width * 4
        format = FORMAT_BGRX32
        self.performFormatTest(stride, format, 'bgrx32')

    def testArgb32(self):
        stride = width * 4
        format = FORMAT_ARGB32
        self.performFormatTest(stride, format, 'argb32')

    def testArgb32Premultiplied(self):
        stride = width * 4
        format = FORMAT_ARGB32_PREMULTIPLIED
        self.performFormatTest(stride, format, 'argb32-premul')

    def testRgba32(self):
        stride = width * 4
        format = FORMAT_RGBA32
        self.performFormatTest(stride, format, 'rgba32')

    def testRgba32Premultiplied(self):
        stride = width * 4
        format = FORMAT_RGBA32_PREMULTIPLIED
        self.performFormatTest(stride, format, 'rgba32-premul')

    def testAbgr32(self):
        stride = width * 4
        format = FORMAT_ABGR32
        self.performFormatTest(stride, format, 'abgr32')

    def testAbgr32Premultiplied(self):
        stride = width * 4
        format = FORMAT_ABGR32_PREMULTIPLIED
        self.performFormatTest(stride, format, 'abgr32-premul')

    def testBgra32(self):
        stride = width * 4
        format = FORMAT_BGRA32
        self.performFormatTest(stride, format, 'bgra32')

    def testBgra32Premultiplied(self):
        stride = width * 4
        format = FORMAT_BGRA32_PREMULTIPLIED
        self.performFormatTest(stride, format, 'bgra32-premul')

# vim:sw=4:ts=4:et:autoindent

