import unittest
import gobject
from pyfirtree import *

class SimpleGood(unittest.TestCase):
    def setUp(self):
        self._k = Kernel()
        self._k.connect('module-changed', self.modChange)
        self._mod_changed_called = False

        self.assertNotEqual(self._k, None)
        self.assert_(not self._mod_changed_called)
        src = """
            kernel vec4 simpleKernel() {
                return vec4(1,0,0,1);
            }
        """
        self._k.compile_from_source(src)
        self.assert_(self._mod_changed_called)

    def tearDown(self):
        self._k = None

    def modChange(self, kernel):
        self.assertEqual(kernel, self._k)
        self._mod_changed_called = True

    def testArgList(self):
        self.assertEqual(self._k.list_arguments(), ())

    def testValidity(self):
        self.assertEqual(self._k.is_valid(), True)

    def testCompileStatusMethod(self):
        self.assertEqual(self._k.get_compile_status(), True)

    def testCompileStatusProperty(self):
        self.assertEqual(self._k.get_property('compile-status'), True)

    def testCompileLog(self):
        log = self._k.get_compile_log()
        self.assertNotEqual(log, None)
        self.assertEqual(len(log), 0)

class SimpleBad(unittest.TestCase):
    def setUp(self):
        self._k = Kernel()
        self._k.connect('module-changed', self.modChange)
        self._mod_changed_called = False

        self.assertNotEqual(self._k, None)
        self.assert_(not self._mod_changed_called)
        src = """
            kernel vec4 simpleKernel() {
                int a = functionDoesNotExist();
                return vec4(1,0,0,1);
            }
        """
        self._k.compile_from_source(src)
        self.assert_(self._mod_changed_called)

    def tearDown(self):
        self._k = None

    def modChange(self, kernel):
        self.assertEqual(kernel, self._k)
        self._mod_changed_called = True

    def testValidity(self):
        self.assertEqual(self._k.is_valid(), False)

    def testCompileStatusMethod(self):
        self.assertEqual(self._k.get_compile_status(), False)

    def testCompileStatusProperty(self):
        self.assertEqual(self._k.get_property('compile-status'), False)

    def testCompileLog(self):
        log = self._k.get_compile_log()
        self.assertNotEqual(log, None)
        self.assertEqual(len(log), 1)

class SimpleLines(unittest.TestCase):
    def setUp(self):
        self._k = Kernel()
        self.assertNotEqual(self._k, None)
        src = ( "kernel vec4 simpleKernel() {",
                "    return vec4(1,0,0,1);",
                "}", )
        self._k.compile_from_source(src)

    def tearDown(self):
        self._k = None

    def testCompileStatusMethod(self):
        self.assertEqual(self._k.get_compile_status(), True)

    def testCompileStatusProperty(self):
        self.assertEqual(self._k.get_property('compile-status'), True)

    def testCompileLog(self):
        log = self._k.get_compile_log()
        self.assertNotEqual(log, None)
        self.assertEqual(len(log), 0)

class Arguments(unittest.TestCase):
    def setUp(self):
        self._k = Kernel()
        self.assertNotEqual(self._k, None)
        src = """
            kernel vec4 simpleKernel(int arg1, static float arg2) {
                return vec4(1,0,0,1);
            }
        """
        self._k.compile_from_source(src)
        self.assertEqual(self._k.get_compile_status(), True)
        self.assertEqual(self._k['arg1'], None)
        self.assertEqual(self._k['arg2'], None)

        self._mirror = { }
        self._k.connect('argument-changed', self.argChange)

    def tearDown(self):
        self._k = None

    def argChange(self, kernel, arg):
        self.assertEqual(kernel, self._k)
        self._mirror[arg] = kernel[arg]

    def testValidity(self):
        self._k['arg1'] = None
        self._k['arg2'] = None
        self.assertEqual(self._k.is_valid(), False)
        self._k['arg1'] = 1.0
        self.assertEqual(self._k.is_valid(), False)
        self._k['arg2'] = 2.0
        self.assertEqual(self._k.is_valid(), True)
        self._k['arg1'] = None
        self.assertEqual(self._k.is_valid(), False)
        self._k['arg2'] = None
        self.assertEqual(self._k.is_valid(), False)

    def testArgList(self):
        self.assertEqual(self._k.list_arguments(), ('arg1', 'arg2'))

    def testArgExistence(self):
        self.assert_(self._k.has_argument_named('arg1'))
        self.assert_(self._k.has_argument_named('arg2'))
        self.assert_(not self._k.has_argument_named('does_not_exist'))

    def testBogusSpec(self):
        def bogusSpec():
            bogusspec = self._k.get_argument_spec('bogus')
        self.assertRaises(KeyError, bogusSpec)

    def testArgSpec(self):
        arg1spec = self._k.get_argument_spec('arg1')
        arg2spec = self._k.get_argument_spec('arg2')
        self.assertEqual(arg1spec, ('arg1', gobject.TYPE_INT, False))
        self.assertEqual(arg2spec, ('arg2', gobject.TYPE_FLOAT, True))

    def testArg1IntSet(self):
        self._k.set_argument_value('arg1', None)
        self.assertEqual(self._k.get_argument_value('arg1'), None)
        self.assertEqual(self._mirror['arg1'], None)
        self._k.set_argument_value('arg1', 1)
        self.assertEqual(self._k.get_argument_value('arg1'), 1)
        self.assertEqual(self._mirror['arg1'], 1)

    def testArg1FloatSet(self):
        self._k.set_argument_value('arg1', None)
        self.assertEqual(self._k.get_argument_value('arg1'), None)
        self.assertEqual(self._mirror['arg1'], None)
        self._k.set_argument_value('arg1', 3.5)
        self.assertEqual(self._k.get_argument_value('arg1'), 3)
        self.assertEqual(self._mirror['arg1'], 3)

    def testArg1BadSet(self):
        self._k.set_argument_value('arg1', None)
        self.assertEqual(self._k.get_argument_value('arg1'), None)
        def badSet():
            self._k.set_argument_value('arg1', 'some string')
        self.assertRaises(TypeError, badSet)

    def testArg2IntSet(self):
        self._k.set_argument_value('arg2', None)
        self.assertEqual(self._k.get_argument_value('arg2'), None)
        self.assertEqual(self._mirror['arg2'], None)
        self._k.set_argument_value('arg2', 1)
        self.assertEqual(self._k.get_argument_value('arg2'), 1.0)
        self.assertEqual(self._mirror['arg2'], 1.0)

    def testArg2FloatSet(self):
        self._k.set_argument_value('arg2', None)
        self.assertEqual(self._k.get_argument_value('arg2'), None)
        self.assertEqual(self._mirror['arg2'], None)
        self._k.set_argument_value('arg2', 3.5)
        self.assertEqual(self._k.get_argument_value('arg2'), 3.5)
        self.assertEqual(self._mirror['arg2'], 3.5)

    def testArg2BadSet(self):
        self._k.set_argument_value('arg2', None)
        self.assertEqual(self._k.get_argument_value('arg2'), None)
        def badSet():
            self._k.set_argument_value('arg2', 'some string')
        self.assertRaises(TypeError, badSet)

    def testMap1IntSet(self):
        self._k['arg1'] = None
        self.assertEqual(self._k['arg1'], None)
        self._k['arg1'] = 1
        self.assertEqual(self._k['arg1'], 1)

    def testMap1FloatSet(self):
        self._k['arg1'] = None
        self.assertEqual(self._k['arg1'], None)
        self._k['arg1'] = 3.5
        self.assertEqual(self._k['arg1'], 3)

    def testMap1BadSet(self):
        self._k['arg1'] = None
        self.assertEqual(self._k['arg1'], None)
        def badSet():
            self._k['arg1'] = 'some string'
        self.assertRaises(TypeError, badSet)

    def testMap2IntSet(self):
        self._k['arg2'] = None
        self.assertEqual(self._k['arg2'], None)
        self._k['arg2'] = 1
        self.assertEqual(self._k['arg2'], 1.0)

    def testMap2FloatSet(self):
        self._k['arg2'] = None
        self.assertEqual(self._k['arg2'], None)
        self._k['arg2'] = 3.5
        self.assertEqual(self._k['arg2'], 3.5)

    def testMap2BadSet(self):
        self._k['arg2'] = None
        self.assertEqual(self._k['arg2'], None)
        def badSet():
            self._k['arg2'] = 'some string'
        self.assertRaises(TypeError, badSet)

    def testNoArgBadGet(self):
        def badGet():
            tmp = self._k.get_argument_value('noarg1')
        self.assertRaises(KeyError, badGet)

    def testNoMapArgBadGet(self):
        def badGet():
            tmp = self._k['noarg2']
        self.assertRaises(KeyError, badGet)

    def testNoArgBadSet(self):
        def badSet():
            tmp = self._k.set_argument_value('noarg1', 1)
        self.assertRaises(KeyError, badSet)

    def testNoMapArgBadSet(self):
        def badSet():
            self._k['noarg2'] = None
        self.assertRaises(KeyError, badSet)

class Creation(unittest.TestCase):
    def setUp(self):
        self._k = Kernel()

    def tearDown(self):
        self._k = None

    def testKernelCreation(self):
        self.failIfEqual(self._k, None)

    def testDefaultArgList(self):
        self.assertEqual(self._k.list_arguments(), None)

    def testDefaultValidity(self):
        self.assertEqual(self._k.is_valid(), False)

    def testDefaultStatus(self):
        self.assertEqual(self._k.get_compile_status(), False)

    def testDefaultLog(self):
        self.assertEqual(self._k.get_compile_log(), None)

    def testDefaultArgList(self):
        self.assertEqual(self._k.list_arguments(), None)

class VectorArguments(unittest.TestCase):
    def setUp(self):
        self._k = Kernel()
        self.assertNotEqual(self._k, None)
        src = """
            kernel vec4 simpleKernel(float arg1, vec2 arg2, vec3 arg3, vec4 arg4) {
                return vec4(1,0,0,1);
            }
        """
        self._k.compile_from_source(src)
        self.assertEqual(self._k.get_compile_status(), True)
        self.assertEqual(self._k['arg1'], None)
        self.assertEqual(self._k['arg2'], None)
        self.assertEqual(self._k['arg3'], None)
        self.assertEqual(self._k['arg4'], None)

    def tearDown(self):
        self._k = None

    def assertAlmostEqualTuples(self, a, b):
        self.assertEqual(len(a), len(b))
        for i in range(0, len(a)):
            self.assertAlmostEqual(a[i], b[i], 4)

    def testSet1Int(self):
        self._k['arg1'] = 1;
        self.assertEqual(self._k['arg1'], 1.0)

    def testSet1Float(self):
        self._k['arg1'] = 4.5;
        self.assertEqual(self._k['arg1'], 4.5)

    def testSet1Bad(self):
        def badSet():
            self._k['arg1'] = 'invalid';
        self.assertRaises(TypeError, badSet)

    def testSet2Int(self):
        self._k['arg2'] = (1, 2)
        self.assertAlmostEqualTuples(self._k['arg2'], (1, 2))

    def testSet2Float(self):
        self._k['arg2'] = (1.2, 2.3)
        self.assertAlmostEqualTuples(self._k['arg2'], (1.2, 2.3))
        
    def testSet2Mixed(self):
        self._k['arg2'] = (1.2, 2)
        self.assertAlmostEqualTuples(self._k['arg2'], (1.2, 2))

    def testSet2Bad1(self):
        def badSet():
            self._k['arg2'] = 1.0
        self.assertRaises(TypeError, badSet)
 
    def testSet2Bad2(self):
        def badSet():
            self._k['arg2'] = (1.0, 2.0, 3.0)
        self.assertRaises(TypeError, badSet)
 
    def testSet2Bad3(self):
        def badSet():
            self._k['arg2'] = (1.0, 'bad')
        self.assertRaises(TypeError, badSet)

    def testSet3Int(self):
        self._k['arg3'] = (1, 2, 3)
        self.assertAlmostEqualTuples(self._k['arg3'], (1, 2, 3))

    def testSet3Float(self):
        self._k['arg3'] = (1.2, 2.3, 3.4)
        self.assertAlmostEqualTuples(self._k['arg3'], (1.2, 2.3, 3.4))
        
    def testSet3Mixed(self):
        self._k['arg3'] = (1.2, 2, 3.4)
        self.assertAlmostEqualTuples(self._k['arg3'], (1.2, 2, 3.4))

    def testSet3Bad1(self):
        def badSet():
            self._k['arg3'] = 1.0
        self.assertRaises(TypeError, badSet)
  
    def testSet3Bad2a(self):
        def badSet():
            self._k['arg3'] = (1.0, 2.0)
        self.assertRaises(TypeError, badSet)

    def testSet3Bad2b(self):
        def badSet():
            self._k['arg3'] = (1.0, 2.0, 3.0, 4.0)
        self.assertRaises(TypeError, badSet)
 
    def testSet3Bad3(self):
        def badSet():
            self._k['arg3'] = (1.0, 'bad', 3.0)
        self.assertRaises(TypeError, badSet)

    def testSet4Int(self):
        self._k['arg4'] = (1, 2, 3, 4)
        self.assertAlmostEqualTuples(self._k['arg4'], (1, 2, 3, 4))

    def testSet4Float(self):
        self._k['arg4'] = (1.2, 2.3, 3.4, 4.5)
        self.assertAlmostEqualTuples(self._k['arg4'], (1.2, 2.3, 3.4, 4.5))
        
    def testSet4Mixed(self):
        self._k['arg4'] = (1.2, 2, 3.4, 4)
        self.assertAlmostEqualTuples(self._k['arg4'], (1.2, 2, 3.4, 4))

    def testSet4Bad1(self):
        def badSet():
            self._k['arg4'] = 1.0
        self.assertRaises(TypeError, badSet)
 
    def testSet4Bad2a(self):
        def badSet():
            self._k['arg4'] = (3.0, 4.0, 5.0)
        self.assertRaises(TypeError, badSet)
 
    def testSet4Bad2b(self):
        def badSet():
            self._k['arg4'] = (1.0, 2.0, 3.0, 4.0, 5.0)
        self.assertRaises(TypeError, badSet)

    def testSet4Bad3(self):
        def badSet():
            self._k['arg4'] = (1.0, 'bad', 3.0, 'worse')
        self.assertRaises(TypeError, badSet)

class SamplerArguments(unittest.TestCase):
    def setUp(self):
        self._k = Kernel()
        self.assertNotEqual(self._k, None)
        src = """
            kernel vec4 simpleKernel(sampler arg1) {
                return vec4(1,0,0,1);
            }
        """
        self._k.compile_from_source(src)
        self.assertEqual(self._k.get_compile_status(), True)
        self.assertEqual(self._k['arg1'], None)

    def tearDown(self):
        self._k = None

    def testSimpleConstruction(self):
        s = Sampler()
        self.assertNotEqual(s, None)

    def testSimpleAssignment(self):
        self._k['arg1'] = None
        self.assertEqual(self._k['arg1'], None)
        s = Sampler()
        self.assertNotEqual(s, None)
        self._k['arg1'] = s
        self.assertEqual(self._k['arg1'], s)


# vim:sw=4:ts=4:et:autoindent

