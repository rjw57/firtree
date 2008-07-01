#!/usr/bin/env python

'''
demo.py

This module provides an example of using the Firtree
Python bindings to rendering via GTK.

Rich Wareham, <richwareham@gmail.com>
'''

# Import the Python OpenGL stuff
from OpenGL.GLUT import *
from OpenGL.GLU import *
from OpenGL.GL import *

#==============================================================================
# Use sys to find the script path and hence construct a path in which to 
# search for the Firtree bindings.

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
imageDir = os.path.join(rootDir, 'examples')

# Append this path to our search path & import the firtree bindings.
sys.path.append(importDir)

import Firtree

class FirtreeScene:
    ''' Implements the GLScene interface for the demo '''

    def init (self):
        # Set background colour & disable depth testing
        glClearColor(0, 0, 0, 1)
        glDisable(GL_DEPTH_TEST)

        # Setup a simple alpha over kernel.
        overKernel = Firtree.Kernel.CreateFromSource('''
        kernel vec4 overKernel(sampler over, sampler under)
        {
            vec4 overVal = sample(over, samplerCoord(over));
            vec4 underVal = sample(under, samplerCoord(under));
            return overVal + underVal * (1.0-overVal.a);
        }
        ''')
        compositeImage = Firtree.Image.CreateFromKernel(overKernel)

        # Load the lena and fog images.
        lenaImage = Firtree.Image.CreateFromFile(
            os.path.join(imageDir, 'lena.png'))
        fogImage = Firtree.Image.CreateFromFile(
            os.path.join(imageDir, 'fog.png'))

        # Form a rotated and translated lena image.
        lenaTransform = Firtree.AffineTransform.Translation(-256, -256)
        lenaTransform.RotateByDegrees(20)
        #lenaTransform.TranslateBy(400,300)
        lenaTransImage = Firtree.Image.CreateFromImageWithTransform(
            lenaImage, lenaTransform) 

        # Form a rotated, translated and scaled fog image.
        fogTransform = Firtree.AffineTransform.Translation(-160, -120)
        fogTransform.RotateByDegrees(-30)
        fogTransform.ScaleBy(1,1.5)
        #fogTransform.TranslateBy(400,300)
        fogTransImage = Firtree.Image.CreateFromImageWithTransform(
            fogImage, fogTransform)

        # Wire the lena and fog images into the kernel.
        overKernel.SetValueForKey(lenaTransImage, 'under')
        overKernel.SetValueForKey(fogTransImage, 'over')

        self.image = compositeImage

        # Create a rendering context.
        self.context = Firtree.GLRenderer.Create()

    def display (self, width, height):
        glClear(GL_COLOR_BUFFER_BIT)

        # Render the composited image into the framebuffer.
        self.context.RenderWithOrigin(self.image, 
            Firtree.Point2D(0.5*width, 0.5*height))

    def clear_up (self):
        print('Clearing up...')

        self.image = None
        self.context = None

        # Sanity check to make sure that there are no objects left
        # dangling.
        gc.collect()
        globalObCount = Firtree.ReferenceCounted.GetGlobalObjectCount()
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
    
    # glutTimerFunc(frameDelay, timer, 0)

    glutMainLoop()

# vim:sw=4:ts=4:et:autoindent
