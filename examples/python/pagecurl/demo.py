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
        self._start_image = None
        self._end_image = None

        kernelSource = '''
          kernel vec4 transitionKernel(sampler front, sampler back,
                                       vec2 diagonal, float offset,
                                       float radius)
          {
            float onDiagonalCoord = dot(destCoord(), diagonal);
            float offDiagonalCoord = dot(destCoord(), diagonal.yx * vec2(-1,1));

            onDiagonalCoord -= offset;

            vec4 untouchedFront = sample(front, samplerCoord(front));
            untouchedFront *= step(onDiagonalCoord, 0.0);

            float cosThetaB = onDiagonalCoord / radius;
            float thetaB = acos(cosThetaB);
            float thetaA = 0.5 * 3.14159 - thetaB;
            float upturnOffset = radius * thetaA - onDiagonalCoord;
            float downturnOffset = radius * (thetaB + 0.5*3.14159) - onDiagonalCoord;

            vec4 upturnColour = sample(front, samplerTransform(front,
                destCoord() + upturnOffset*diagonal));
            vec4 downturnColour = sample(front, samplerTransform(front,
                destCoord() + downturnOffset*diagonal));
            downturnColour *= vec4(0.5, 0.5, 0.5, 1.0);

            float backsideOffset = - 2.0 * onDiagonalCoord + (3.14159 * radius);
            vec4 backsideColour = sample(front, samplerTransform(front,
                destCoord() + backsideOffset*diagonal));
            backsideColour *= step(cosThetaB, 0.0);
            backsideColour *= vec4(0.5, 0.5, 0.5, 1.0);

            float mask = step(0.0, cosThetaB) * step(cosThetaB, 1.0);
            upturnColour *= mask;
            downturnColour *= mask;

            vec4 turnColour = downturnColour + (1.0 - downturnColour.a) * upturnColour;

            vec4 frontCol = (1.0-backsideColour.a)*(untouchedFront + turnColour) + backsideColour;
            vec4 backCol = sample(back, samplerCoord(back));

            return (1.0-frontCol.a)*backCol + frontCol;
          }
        '''

        self._transitionKernel = Kernel.CreateFromSource(kernelSource)
        self._output = Image.CreateFromKernel(self._transitionKernel)

        self._transitionKernel.SetValueForKey(0.707, 0.707, 'diagonal')
        self._transitionKernel.SetValueForKey(0.0, 'offset')

        self._radius = 40.0
        self._transitionKernel.SetValueForKey(self._radius, 'radius')


    def set_start_image(self, im):
        self._start_image = im
        self._transitionKernel.SetValueForKey(self._start_image, 'front')

    def set_end_image(self, im):
        self._end_image = im
        self._transitionKernel.SetValueForKey(self._end_image, 'back')

    def set_progress(self, p):
        
        extent = self.get_output().GetExtent()
        diagonal = math.sqrt(extent.Size.Width*extent.Size.Width + 
            extent.Size.Height*extent.Size.Height)
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

        front = Image.CreateFromFile(os.path.join(imageDir,'tariffa.jpg'))
        back = Image.CreateFromFile(os.path.join(imageDir,'statues.jpg'))

        self.transition = PageCurlTransition()
        self.transition.set_start_image(front)
        self.transition.set_end_image(back)
        self.transition.set_progress(0.0)

    def display (self, width, height):
        glClearColor(0,0,0,1)
        glClear(GL_COLOR_BUFFER_BIT)

        if(self.transition == None):
            pass

        t = float(glutGet(GLUT_ELAPSED_TIME)) / 1000.0
        self.transition.set_progress(0.5 * (1.0 + math.sin(1.0 * t)))
        #self.transition.set_progress(0.5)
        
        # Render the composited image into the framebuffer.
        self.context.RenderWithOrigin(self.transition.get_output(), Point2D(0,0))

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
