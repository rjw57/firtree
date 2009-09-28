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
                    emit(vec4(destCoord(), 1, 0));
                }
                if(incol.g > 0.5) {
                    emit(vec4(destCoord(), 2, 0));
                }
            }
        """
        self._k.compile_from_source(src)

        log = self._k.get_compile_log()
        if len(log) != 0:
            print('\n'.join(log))

        source_surface = cairo.ImageSurface(cairo.FORMAT_ARGB32, 320, 240)
        cr = cairo.Context(source_surface)
        cr.set_source_rgba(0,0,1,1)
        cr.paint()

        cr.set_source_rgba(1,0,0,1)
        cr.move_to(0,0)
        cr.line_to(160,0)
        cr.line_to(160,240)
        cr.line_to(0,240)
        cr.line_to(0,0)
        cr.fill()

        # Source has 160x240 == 38400 red pixels

        cr.set_source_rgba(0,1,0,1)
        cr.move_to(180,10)
        cr.line_to(200,10)
        cr.line_to(200,20)
        cr.line_to(180,20)
        cr.line_to(180,10)
        cr.fill()

        # Source has 20x10 == 200 green pixels

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

    def testTarget(self):
        self.assertEqual(self._k.get_target(), KERNEL_TARGET_REDUCE)

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

    def testEngine(self):
        engine = CpuReduceEngine()
        self.assertEqual(engine.get_kernel(), None)
        engine.set_kernel(self._k)
        self.assertEqual(engine.get_kernel(), self._k)

    def testReduce(self):
        engine = CpuReduceEngine()
        self.assertEqual(engine.get_kernel(), None)
        engine.set_kernel(self._k)
        self.assertEqual(engine.get_kernel(), self._k)
        output = engine.run((0,0,320,240),320,240)
        self.assertEqual(len(output), 38600)
        self.assertEqual(len(filter(lambda v: v[2] == 1, output)), 38400)
        self.assertEqual(len(filter(lambda v: v[2] == 2, output)), 200)
        self.assertEqual(len(filter(lambda v: (v[2] == 1) and (v[0] <= 160), output)), 38400)
        self.assertEqual(len(filter(lambda v: (v[2] == 1) and (v[0] > 160), output)), 0)
        self.assertEqual(len(filter(lambda v: (v[2] == 2) and (v[0] <= 200), output)), 200)
        self.assertEqual(len(filter(lambda v: (v[2] == 2) and (v[0] > 200), output)), 0)
        self.assertEqual(len(filter(lambda v: (v[2] == 2) and (v[1] <= 20), output)), 200)
        self.assertEqual(len(filter(lambda v: (v[2] == 2) and (v[1] > 20), output)), 0)
        self.assertEqual(len(filter(lambda v: (v[2] == 2) and (v[1] < 10), output)), 0)

# vim:sw=4:ts=4:et:autoindent

