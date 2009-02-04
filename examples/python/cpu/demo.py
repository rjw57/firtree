#!/usr/bin/env python

#==============================================================================
# Use sys to find the script path and hence construct a path in which to 
# search for the Firtree bindings.

import math
import sys
import os
import gc

# Find the script path.
scriptDir = os.path.abspath(os.path.dirname(sys.argv[0]))

# Walk down three subdirs...
rootDir = os.path.dirname(os.path.dirname(os.path.dirname(scriptDir)))

# Add path to the python bindings
importDir = os.path.join(rootDir, 'bindings', 'python')

# Create path to the example images
imageDir = os.path.join(rootDir, 'artwork')

# Append this path to our search path & import the firtree bindings.
sys.path.insert(0, importDir)

from Firtree import *

kernel_source = '''
kernel vec4 testKernel() {
    vec4 outputCol = vec4(destCoord() / 700,1-(destCoord().y / 480),1);
    float a = 0.5*(destCoord().x/640) + 0.5;
    outputCol *= a;
    return outputCol;
}
'''

if __name__ == '__main__':
    renderer = CPURenderer.Create(640, 480)

    renderer.Clear(0.5,0.5,0.5,1)

    im = Image.CreateFromKernel(Kernel.CreateFromSource(kernel_source))

    renderer.RenderInRect(im, renderer.GetViewport(), renderer.GetViewport())

    renderer.WriteToFile('foo.png')

# vim:sw=4:ts=4:et:autoindent
