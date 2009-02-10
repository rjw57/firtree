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

kernel1_source = '''
kernel vec4 testKernel() {
    const float radius = 90; // < Size of blob.
    const float sigma = 30; // < Size of blob.

    // Find delta from centre.
    vec2 delta = destCoord() - vec2(320, 240);

    // Find distance from the centre.
    float r = length(delta);

    // Find angle from centre.
    float a = atan(delta.x, delta.y);

    r *= 1.0 + 0.3 * sin(7 * a);
    r -= radius;
    r /= sigma;

    // Calculate the alpha value of the output.
    float alpha = exp(-r*r);

    // Set the output colour.
    vec4 outputCol = vec4(0.5, 0.75, 0, 0.5*alpha);

    // Return the output appropriately alpha pre-multiplied.
    return premultiply(outputCol);
}
'''

kernel2_source = '''
kernel vec4 testKernel(sampler src) {
    vec2 delta = destCoord() - vec2(320, 240);
    vec2 s = sin(delta / 30.0);

    vec2 pos = destCoord();
    pos.yx += 20 * s;

    return sample(src, samplerTransform(src, pos));
}
'''

bigbenim = Image.CreateFromFile(os.path.join(imageDir, 'bigben.jpg'))
firim = Image.CreateFromFile(os.path.join(imageDir, 'firtree-192x192.png'))

kernel1 = Kernel.CreateFromSource(kernel1_source)
if(not kernel1.GetStatus()):
    print "KERNEL1:"
    print kernel1.GetCompileLog()
    sys.exit(1)
im1 = Image.CreateFromKernel(kernel1)

kernel2 = Kernel.CreateFromSource(kernel2_source)
if(not kernel2.GetStatus()):
    print "KERNEL2:"
    print kernel2.GetCompileLog()
    sys.exit(2)

im2 = Image.CreateFromKernel(kernel2)

vp = Rect2D(0,0,640,480)

# Create a CPU-based renderer to create an image.
renderer = CPURenderer.Create(vp)

# Render the image into the renderer's viewport.
def render(renderer, viewport):
    # Set the background colour.
    renderer.Clear(0.25,0.25,0.25,1.0)

    kernel2.SetValueForKey(bigbenim, 'src')
    renderer.RenderInRect(im2, viewport, viewport)
    kernel2.SetValueForKey(im1, 'src')
    renderer.RenderInRect(im2, viewport, viewport)
    renderer.RenderInRect(im1, viewport, viewport)
    for x in range(50, 600, 200):
        renderer.RenderWithOrigin(firim, Point2D(x,50))

# Write the output.
render(renderer, renderer.GetViewport())
renderer.WriteToFile('output.png')

# vim:sw=4:ts=4:et:autoindent
