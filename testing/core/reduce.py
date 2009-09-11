import unittest
import gobject
import cairo
from pyfirtree import *

class SimpleReduce(unittest.TestCase):
    def setUp(self):
        self._k = Kernel()
        self.assertNotEqual(self._k, None)
        src = """
            kernel __reduce void simpleKernel(static sampler src) {
                vec4 incol = sample(src, samplerCoord(src));
                if(incol.r > 0.5) {
                    emit(vec4(destCoord(), 0, 0));
                }
            }
        """
        self._k.compile_from_source(src)

        log = self._k.get_compile_log()
        if len(log) != 0:
            print('\n'.join(log))

        source_surface = cairo.ImageSurface(cairo.FORMAT_ARGB32, 320, 240)
        cr = cairo.Context(source_surface)
        cr.set_source_rgba(0,0,1,0.2)
        cr.paint()
        cr.set_source_rgba(1,0,0,1)
        cr.move_to(0,0)
        cr.line_to(320,240)
        cr.move_to(0,240)
        cr.line_to(320,0)
        cr.stroke()

        s = CairoSurfaceSampler()
        s.set_cairo_surface(source_surface)
        self._k['src'] = s

    def tearDown(self):
        self._k = None

    def testDebug(self):
        #print(debug_dump_kernel_function(self._k))
        self.assertNotEqual(debug_dump_kernel_function(self._k), None)

    def testValidity(self):
        self.assertEqual(self._k.is_valid(), True)

    def testCompileStatusMethod(self):
        self.assertEqual(self._k.get_compile_status(), True)

    def testCompileStatusProperty(self):
        self.assertEqual(self._k.get_property('compile-status'), True)

    def testCompileLog(self):
        log = self._k.get_compile_log()
        if len(log) != 0:
            print('\n'.join(log))
        self.assertNotEqual(log, None)
        self.assertEqual(len(log), 0)

# vim:sw=4:ts=4:et:autoindent

