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
    const float sigma = 150; // < Size of blob.

    // Find delta from centre.
    vec2 delta = destCoord() - vec2(320, 240);

    // Find distance from the centre.
    float r = length(delta);

    // Find angle from centre.
    float a = atan(delta.x, delta.y);

    r *= 1.0 + 0.1 * sin(10 * a);

    // Calculate the alpha value of the output.
    float alpha = step(0.5, exp(-(r*r) / (sigma*sigma)));

    // Set the output colour.
    vec4 outputCol = vec4(0, 1, 0, alpha);

    // Return the output appropriately alpha pre-multiplied.
    return premultiply(outputCol);
}
'''

# Create a CPU-based renderer to create an image.
renderer = CPURenderer.Create(640, 480)

# Set the background colour.
renderer.Clear(0.5,0.5,0.5,1)

# Create an image from the kernel above.
im = Image.CreateFromKernel(Kernel.CreateFromSource(kernel_source))

# Render the image into the renderer's viewport.
renderer.RenderInRect(im, renderer.GetViewport(), renderer.GetViewport())

# Write the output.
renderer.WriteToFile('output.png')

# vim:sw=4:ts=4:et:autoindent
