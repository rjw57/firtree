#!/usr/bin/env python

# Import the Python OpenGL stuff
from OpenGL.GLUT import *
from OpenGL.GLU import *
from OpenGL.GL import *

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

class FirtreeScene:

    def init (self):
        sigma = 20.0

        # Create a rendering context.
        self.context = GLRenderer.Create()

        # Load the firtree image.
        firtreeImage = Image.CreateFromFile(
            os.path.join(imageDir, 'firtree-192x192.png'))
        firtreeExtent = firtreeImage.GetExtent()
        firtreeExtent = Rect2D.Inset(firtreeExtent, -2.0*sigma, -2.0*sigma)
        
        # Create an accumulation image
        self.accumulator = ImageAccumulator.Create(firtreeExtent)

        blurKernelSource = '''
            kernel vec4 blurKernel(sampler src, vec2 direction, float sigma)
            {
                vec4 outputColour = vec4(0,0,0,0);
                float weightSum = 0.0;
                for(float delta = -2.0; delta <= 2.0; delta += 1.0/sigma)
                {
                    float weight = exp(-delta*delta);
                    weightSum += weight;
                    outputColour += weight * sample(src, 
                        samplerTransform(src, destCoord() + sigma * delta * direction));
                }
                
                outputColour /= weightSum;

                return outputColour;
            }
        '''

        # Create a blur kernel.
        xBlurKernel = Kernel.CreateFromSource(blurKernelSource)
        yBlurKernel = Kernel.CreateFromSource(blurKernelSource)

        xBlurKernel.SetValueForKey(firtreeImage, 'src')
        xBlurKernel.SetValueForKey(1.0, 0.0, 'direction')
        xBlurKernel.SetValueForKey(sigma, 'sigma')

        self.ximage = Image.CreateFromKernel(xBlurKernel)

        yBlurKernel.SetValueForKey(self.accumulator.GetImage(), 'src')
        yBlurKernel.SetValueForKey(0.0, 1.0, 'direction')
        yBlurKernel.SetValueForKey(sigma, 'sigma')

        self.xk = xBlurKernel
        self.yk = yBlurKernel

        blurImage = Image.CreateFromKernel(yBlurKernel)
        scaleBlurImage = Image.CreateFromImageWithTransform(blurImage,
            AffineTransform.Scaling(2.0, 2.0))

        self.fadeKernel = Kernel.CreateFromSource('''
            kernel vec4 fadeKernel(sampler src, float a)
            {
                return sample(src, samplerCoord(src)) * a;
            }
        ''')
        self.fadeKernel.SetValueForKey(scaleBlurImage, 'src')
        self.fadeKernel.SetValueForKey(1.0, 'a')

        self.image = Image.CreateFromKernel(self.fadeKernel)


    def display (self, width, height):
        glClearColor(0,0,0,1)
        glClear(GL_COLOR_BUFFER_BIT)

        t = float(glutGet(GLUT_ELAPSED_TIME)) / 1000.0
        self.xk.SetValueForKey(
            10.0 * (1.1 + math.sin(2.0 * t)), 'sigma')
        self.yk.SetValueForKey(
            10.0 * (1.1 + math.sin(2.0 * t)), 'sigma')
        self.fadeKernel.SetValueForKey(
            0.5 * (1.0 - math.sin(2.0 * t)), 'a')

        self.accumulator.Clear()
        self.accumulator.RenderImage(self.ximage)

        # Render the composited image into the framebuffer.
        self.context.RenderWithOrigin(self.image, 
            Point2D(0,0))

    def clear_up (self):
        print('Clearing up...')

        self.image = None
        self.ximage = None
        self.context = None
        self.accumulator = None
        self.xk = None
        self.yk = None
        self.fadeKernel = None
        
        # Sanity check to make sure that there are no objects left
        # dangling.
        gc.collect()
        globalObCount = ReferenceCounted.GetGlobalObjectCount()
        print('Number of objects still allocated (should be zero): %i' 
            % globalObCount)

    def reshape (self, width, height):
        # Resize the OpenGL viewport
        glViewport(0, 0, width, height)
        glMatrixMode(GL_PROJECTION)
        glLoadIdentity()
        glOrtho(0.0, width, 0.0, height, -1.0, 1.0)
        glMatrixMode(GL_MODELVIEW)
        glLoadIdentity()
        
    def keypress(self, key):
        if(key == 'q'):
            print('Quitting...')
            quit()

def reshape(w, h):
    global winWidth, winHeight

    winWidth = w
    winHeight = h
    glscene.reshape(winWidth, winHeight)

def quit():
    glscene.clear_up()
    sys.exit()

def display():
    global shouldInit, shouldQuit

    if(shouldInit):
        glscene.init()
        shouldInit = False

    glscene.display(winWidth, winHeight)
    glutSwapBuffers()

def keypress(k, x, y):
    glscene.keypress(k)

def timer(val):
    glutSetWindow(window)
    glutPostRedisplay()
    glutTimerFunc(frameDelay, timer, val)

if __name__ == '__main__':
    glscene = FirtreeScene()

    shouldInit = True
    winWidth = winHeight = 0
    frameDelay = 1000/60

    glutInit(sys.argv)
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB)
    glutInitWindowSize(800, 600)
    window = glutCreateWindow("Firtree Demo")

    glutDisplayFunc(display)
    glutReshapeFunc(reshape)
    glutKeyboardFunc(keypress)
    
    glutTimerFunc(frameDelay, timer, 0)

    glutMainLoop()

# vim:sw=4:ts=4:et:autoindent
