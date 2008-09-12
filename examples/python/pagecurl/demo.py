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
sys.path.append(importDir)

from Firtree import *
from PageCurlTransition import *

class FirtreeScene:

    def init (self):
        # Set background colour & disable depth testing
        glClearColor(0, 0, 0, 1)
        glDisable(GL_DEPTH_TEST)

        self._image_idx = -1
        self._image_files = [ 'bricks.jpg', 'tariffa.jpg', 'statues.jpg', 
            'painting.jpg', 'bigben.jpg' ]

        front = self.next_image()
        back = self.next_image()

        # Create a rendering context.
        self._context = OpenGLContext.CreateNullContext()
        self._renderer = GLRenderer.Create(self._context)

        self._context.Begin()

        self._transition = PageCurlTransition()
        self._transition.set_front_image(front)
        self._transition.set_back_image(back)
        self._transition.set_progress(0.0)
        self._transition.set_radius(100.0)

        self._context.End()

        self._trans_start_time = None

    def next_image(self):
        self._image_idx += 1
        if(self._image_idx >= len(self._image_files)):
            self._image_idx = 0

        return Image.CreateFromFile(os.path.join(imageDir, 
            self._image_files[self._image_idx]))

    def display (self, width, height):
        glClear(GL_COLOR_BUFFER_BIT)

        self._context.Begin()

        if(self._trans_start_time == None):
            self._trans_start_time = float(glutGet(GLUT_ELAPSED_TIME)) / 1000.0

        now = float(glutGet(GLUT_ELAPSED_TIME)) / 1000.0
        delta = now - self._trans_start_time
        pos = 0.5 * delta

        self._transition.set_progress(max(0.0, min(1.0, pos)))
        
        # Render the composited image into the framebuffer.
        self._renderer.RenderWithOrigin(self._transition.get_output(), Point2D(0,0))

        if(pos > 1.0):
            self._trans_start_time = now + 1.0
            old_back_image = self._transition.back_image()
            self._transition.set_back_image(self.next_image())
            self._transition.set_front_image(old_back_image)

        self._context.End()

    def clear_up (self):
        print('Clearing up...')

        self._transition = None
        self._renderer = None
        self._context = None
        
        # Sanity check to make sure that there are no objects left
        # dangling.
        gc.collect()
        globalObCount = ReferenceCounted.GetGlobalObjectCount()
        print('Number of objects still allocated (should be zero): %i' 
            % globalObCount)

    def reshape (self, width, height):
        # Resize the OpenGL viewport
        glViewport(0, 0, width, height)
        
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
    frameDelay = 1000/75

    glutInit(sys.argv)
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB)
    glutInitWindowSize(600, 449)
    window = glutCreateWindow("Firtree Demo")

    glutDisplayFunc(display)
    glutReshapeFunc(reshape)
    glutKeyboardFunc(keypress)
    
    glutTimerFunc(frameDelay, timer, 0)

    glutMainLoop()

# vim:sw=4:ts=4:et:autoindent
