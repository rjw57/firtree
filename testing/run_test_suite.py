#!/usr/bin/env python

import sys, os, unittest

# Set up the path so that the Python bindings are
# loaded correctly.
root_path = os.path.dirname(os.path.dirname(os.path.realpath(__file__)))
bindings_path = os.path.join(root_path, 'bindings', 'python')
sys.path.append(bindings_path)

# Now import the tests
from core.kernel import *

if __name__ == '__main__':
    unittest.main()

# vim:sw=4:ts=4:et:autoindent

