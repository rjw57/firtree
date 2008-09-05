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
import math

class PageCurlTransition:
    def __init__(self):
        self._output = None
        self._front_image = None
        self._back_image = None

        kernelSource = '''
          vec4 atop(vec4 a, vec4 b)
          {
            return a + (1.0-a.a)*b;
          }

          kernel vec4 transitionKernel(sampler front, sampler back,
                                       vec2 diagonal, float offset,
                                       float radius)
          {
            // Find the 'background' colour
            vec4 backColour = sample(back, samplerCoord(back));

            // Firstly we want to calculate the 'along diagonal' 
            // co-ordinate.
            float onDiagonalCoord = dot(destCoord(), diagonal);

            // We offset this co-ordinate to advance the animation.
            onDiagonalCoord -= offset;

            // Calculate masks for regions A and E. These masks are
            // 1.0 in the corresponding regions and 0.0 outside.
            float aMask = step(onDiagonalCoord, 0.0);
            float eMask = step(radius, onDiagonalCoord);

            // We calculate the region A colour first
            vec4 aColour = sample(front, samplerCoord(front)) * aMask;

            // Calculate the 'backside' colour, or region E
            float eOffset = - 2.0 * onDiagonalCoord + (3.14159 * radius);
            vec4 eColour = sample(front, samplerTransform(front,
                destCoord() + eOffset * diagonal));

            // Darken the back side.
            eColour *= vec4(0.7, 0.7, 0.7, 1.0) * aMask;

            // Calculate a combined colour for this region
            vec4 flatColour = atop(eColour, aColour);

            // Now we need to be a bit clever to calculate the curved areas.
            
            // Calculate a mask for the curved area
            float curveMask = (1.0 - aMask) * (1.0 - eMask);

            // Calculate the angles for both areas
            float thetaC = acos(onDiagonalCoord/radius);
            float thetaB = (0.5*3.14159) - thetaC;

            // Given these angles, calculate the offsets for each
            float bOffset = radius*thetaB - onDiagonalCoord;
            float cOffset = radius*(thetaC + 0.5*3.14159) - onDiagonalCoord;

            // From the offsets, calculate the region colours.
            vec4 bColour = sample(front, samplerTransform(front,
                destCoord() + bOffset * diagonal)) * curveMask;
            vec4 cColour = sample(front, samplerTransform(front,
                destCoord() + cOffset * diagonal)) * curveMask;

            // Darken the back side.
            cColour *= vec4(0.7, 0.7, 0.7, 1.0);

            // Calculate the final curve colour.
            vec4 curveColour = atop(cColour, bColour);

            vec4 frontColour = atop(curveColour, flatColour);

            return atop(frontColour, backColour);
          }
        '''

        self._transitionKernel = Kernel.CreateFromSource(kernelSource)
        self._output = Image.CreateFromKernel(self._transitionKernel)

        self._transitionKernel.SetValueForKey(0.707, 0.707, 'diagonal')
        self._transitionKernel.SetValueForKey(0.0, 'offset')

        self.set_radius(40.0)

    def front_image(self):
        return self._front_image

    def back_image(self):
        return self._back_image

    def radius(self):
        return self._radius

    def set_radius(self, radius):
        self._radius = radius
        self._transitionKernel.SetValueForKey(radius, 'radius')

    def set_front_image(self, im):
        self._front_image = im
        self._transitionKernel.SetValueForKey(self._front_image, 'front')

    def set_back_image(self, im):
        self._back_image = im
        self._transitionKernel.SetValueForKey(self._back_image, 'back')

    def set_progress(self, p):
        extent = self.get_output().GetExtent()
        diagonal = math.sqrt(extent.Size.Width*extent.Size.Width + 
            extent.Size.Height*extent.Size.Height)
        p = 1.0 - p
        self._transitionKernel.SetValueForKey((diagonal + self._radius) * p - self._radius, 'offset')
        self._transitionKernel.SetValueForKey(extent.Size.Width/diagonal,
            extent.Size.Height/diagonal, 'diagonal')

    def get_output(self):
        return self._output

class FirtreeScene:

    def init (self):
        sigma = 20.0

        # Create a rendering context.
        self.context = GLRenderer.Create()

        self._image_idx = -1
        self._image_files = [ 'bricks.jpg', 'tariffa.jpg', 'statues.jpg', 
            'painting.jpg', 'bigben.jpg' ]

        front = self.next_image()
        back = self.next_image()

        self.transition = PageCurlTransition()
        self.transition.set_front_image(front)
        self.transition.set_back_image(back)
        self.transition.set_progress(0.0)
        self.transition.set_radius(100.0)

        self._trans_start_time = None

    def next_image(self):
        self._image_idx += 1
        if(self._image_idx >= len(self._image_files)):
            self._image_idx = 0

        return Image.CreateFromFile(os.path.join(imageDir, self._image_files[self._image_idx]))

    def display (self, width, height):
        glClearColor(0,0,0,1)
        glClear(GL_COLOR_BUFFER_BIT)

        if(self.transition == None):
            pass

        if(self._trans_start_time == None):
            self._trans_start_time = float(glutGet(GLUT_ELAPSED_TIME)) / 1000.0

        now = float(glutGet(GLUT_ELAPSED_TIME)) / 1000.0
        delta = now - self._trans_start_time
        pos = 0.5 * delta

        self.transition.set_progress(max(0.0, min(1.0, pos)))
        
        # Render the composited image into the framebuffer.
        self.context.RenderWithOrigin(self.transition.get_output(), Point2D(0,0))

        if(pos > 1.0):
            self._trans_start_time = now + 1.0
            old_back_image = self.transition.back_image()
            self.transition.set_back_image(self.next_image())
            self.transition.set_front_image(old_back_image)

    def clear_up (self):
        print('Clearing up...')

        self.transition = None
        self.context = None
        
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
