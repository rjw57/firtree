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
