#!/usr/bin/env python

import os, sys

# if we were passed the --valgrind option, set
# some environement variables appropriatelt for
# running GLib programs in valgrind.
if('--valgrind' in sys.argv):
    print(">>>> Running tests in valgrind mode <<<<")
    os.environ['G_SLICE'] = 'always-malloc'
    os.environ['G_DEBUG'] = 'gc-friendly'

import unittest

# Set up the path so that the Python bindings are
# loaded correctly.
root_path = os.path.dirname(os.path.dirname(os.path.realpath(__file__)))
bindings_path = os.path.join(root_path, 'bindings', 'python')
sys.path.append(bindings_path)

# Now import the tests
import core.affinetransform
import core.cpuengine
import core.kernel
import core.sampler
import core.kernelsampler
import core.pixbufsampler

suite = unittest.TestSuite()
suite.addTest(unittest.defaultTestLoader.loadTestsFromModule(core.affinetransform))
suite.addTest(unittest.defaultTestLoader.loadTestsFromModule(core.cpuengine))
suite.addTest(unittest.defaultTestLoader.loadTestsFromModule(core.kernel))
suite.addTest(unittest.defaultTestLoader.loadTestsFromModule(core.sampler))
suite.addTest(unittest.defaultTestLoader.loadTestsFromModule(core.kernelsampler))
suite.addTest(unittest.defaultTestLoader.loadTestsFromModule(core.pixbufsampler))

if __name__ == '__main__':
    runner = unittest.TextTestRunner()
    runner.run(suite)

# vim:sw=4:ts=4:et:autoindent

