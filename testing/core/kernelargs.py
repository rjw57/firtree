import unittest
import gobject
import gtk.gdk
import cairo
from pyfirtree import *

from utils import FirtreeTestCase

width = 190
height = 120

class StaticArgs(FirtreeTestCase):
    def setUp(self):
        self._e = CpuRenderer()
        self.failIfEqual(self._e, None)
        self.assertEqual(self._e.get_sampler(), None)
        self._s = cairo.ImageSurface(cairo.FORMAT_ARGB32, width, height)

    def tearDown(self):
        self._e = None
        self._s = None

    def testFloatArg(self):
        k = Kernel()
        k.compile_from_source('kernel vec4 sinewave(static float factor) { return vec4(0.5+0.5*sin(destCoord().x * factor),0,0,1); }')
        self.assertKernelCompiled(k)

        k['factor'] = 0.1
        self.assert_(k.is_valid())

        ks = KernelSampler()
        ks.set_kernel(k)
        self._e.set_sampler(ks)

        rv = self._e.render_into_cairo_surface((0, 0, width, height), self._s)
        self.assertCairoSurfaceMatches(self._s, 'arg-test-static-float')

    def testIntArg(self):
        k = Kernel()
        k.compile_from_source('''kernel vec4 cosinewave(static int period) { 
            float factor = 2.0 * 3.14159 / period;
            return vec4(0.5+0.5*cos(destCoord().x * factor),0,0,1); 
        }''')
        self.assertKernelCompiled(k)

        k['period'] = width
        self.assert_(k.is_valid())

        ks = KernelSampler()
        ks.set_kernel(k)
        self._e.set_sampler(ks)

        rv = self._e.render_into_cairo_surface((0, 0, width, height), self._s)
        self.assertCairoSurfaceMatches(self._s, 'arg-test-static-int')

    def testBoolArg(self):
        k = Kernel()
        k.compile_from_source('''kernel vec4 cosinewave(static bool green) { 
            float factor = 2.0 * 3.14159 / 20;
            if(green) {
                return vec4(0,0.5+0.5*cos(destCoord().x * factor),0,1); 
            } else {
                return vec4(0.5+0.5*cos(destCoord().x * factor),0,0,1); 
            }
        }''')
        self.assertKernelCompiled(k)

        k['green'] = True
        self.assert_(k.is_valid())

        ks = KernelSampler()
        ks.set_kernel(k)
        self._e.set_sampler(ks)

        rv = self._e.render_into_cairo_surface((0, 0, width, height), self._s)
        self.assertCairoSurfaceMatches(self._s, 'arg-test-static-bool')

    def testVec2Arg(self):
        k = Kernel()
        k.compile_from_source('''kernel vec4 cosinewave(static vec2 periods) { 
            vec2 factor = 2.0 * 3.14159 / periods;
            return vec4(0.5+0.5*cos(destCoord() * factor),0,1); 
        }''')
        self.assertKernelCompiled(k)

        k['periods'] = (width, height)
        self.assert_(k.is_valid())

        ks = KernelSampler()
        ks.set_kernel(k)
        self._e.set_sampler(ks)

        rv = self._e.render_into_cairo_surface((0, 0, width, height), self._s)
        self.assertCairoSurfaceMatches(self._s, 'arg-test-static-vec2')

    def testVec3Arg(self):
        k = Kernel()
        k.compile_from_source('''kernel vec4 colour(static vec3 col) { 
            return vec4(col,1);
        }''')
        self.assertKernelCompiled(k)

        k['col'] = (0.75,1,1)
        self.assert_(k.is_valid())

        ks = KernelSampler()
        ks.set_kernel(k)
        self._e.set_sampler(ks)

        rv = self._e.render_into_cairo_surface((0, 0, width, height), self._s)
        self.assertCairoSurfaceMatches(self._s, 'arg-test-static-vec3')

    def testVec4Arg(self):
        k = Kernel()
        k.compile_from_source('''kernel vec4 colour(static vec4 col) { 
            return premultiply(col);
        }''')
        self.assertKernelCompiled(k)

        k['col'] = (0.75,1,1,0.5)
        self.assert_(k.is_valid())

        ks = KernelSampler()
        ks.set_kernel(k)
        self._e.set_sampler(ks)

        rv = self._e.render_into_cairo_surface((0, 0, width, height), self._s)
        self.assertCairoSurfaceMatches(self._s, 'arg-test-static-vec4')

    def testChangeStatic(self):
        k = Kernel()
        k.compile_from_source('''kernel vec4 colour(static vec4 col) { 
            return premultiply(col);
        }''')
        self.assertKernelCompiled(k)

        ks = KernelSampler()
        ks.set_kernel(k)
        self._e.set_sampler(ks)

        k['col'] = (1,0,0,1)
        rv = self._e.render_into_cairo_surface((0, 0, width, height), self._s)
        self.assertCairoSurfaceMatches(self._s, 'arg-test-static-red')

        k['col'] = (0,1,0,1)
        rv = self._e.render_into_cairo_surface((0, 0, width, height), self._s)
        self.assertCairoSurfaceMatches(self._s, 'arg-test-static-green')

        k['col'] = (0,0,1,1)
        rv = self._e.render_into_cairo_surface((0, 0, width, height), self._s)
        self.assertCairoSurfaceMatches(self._s, 'arg-test-static-blue')

        k2 = Kernel()
        k2.compile_from_source('''kernel vec4 yellow() {
            return vec4(1,1,0,1);
        }''')
        self.assertKernelCompiled(k2)
        ks.set_kernel(k2)

        rv = self._e.render_into_cairo_surface((0, 0, width, height), self._s)
        self.assertCairoSurfaceMatches(self._s, 'arg-test-static-yellow')


# vim:sw=4:ts=4:et:autoindent

