#!/usr/bin/env python

import sys, os, unittest

# Set up the path so that the Python bindings are
# loaded correctly.
root_path = os.path.dirname(os.path.dirname(os.path.realpath(__file__)))
bindings_path = os.path.join(root_path, 'bindings', 'python')
sys.path.append(bindings_path)

# Now import the tests
import core.affinetransform
import core.kernel
import core.sampler

suite = unittest.TestSuite()
suite.addTest(unittest.defaultTestLoader.loadTestsFromModule(core.affinetransform))
suite.addTest(unittest.defaultTestLoader.loadTestsFromModule(core.kernel))
suite.addTest(unittest.defaultTestLoader.loadTestsFromModule(core.sampler))

if __name__ == '__main__':
    runner = unittest.TextTestRunner()
    runner.run(suite)

# vim:sw=4:ts=4:et:autoindent

