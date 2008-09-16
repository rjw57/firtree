from Firtree import *
from ImageFilter import *
import math

class PageCurlTransition:
    def __init__(self):
        self._curve_image = None
        self._front_image = None
        self._back_image = None
        self._accum_image = None

        curveKernelSource = '''
          vec4 atop(vec4 a, vec4 b)
          {
            return a + (1.0-a.a)*b;
          }

          kernel vec4 curve_kernel(sampler front, sampler back,
                                       vec2 diagonal, float offset,
                                       float radius)
          {
            // Firstly we want to calculate the 'along diagonal' 
            // co-ordinate.
            float onDiagonalCoord = dot(destCoord(), diagonal);

            // We offset this co-ordinate to advance the animation.
            onDiagonalCoord -= offset;

            // Calculate masks for regions A and E. These masks are
            // 1.0 in the corresponding regions and 0.0 outside.
            float aMask = step(onDiagonalCoord, 0.0);
            float eMask = step(radius, onDiagonalCoord);

            // Calculate a mask for the curved area
            float curveMask = (1.0 - aMask) * (1.0 - eMask);

            // Calculate the angles for both areas
            float thetaC = acos(curveMask * onDiagonalCoord / radius);
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

            return curveColour;
          }
        '''

        flatKernelSource = '''
          vec4 atop(vec4 a, vec4 b)
          {
            return a + (1.0-a.a)*b;
          }

          kernel vec4 flat_kernel(sampler front, sampler back,
                                  vec2 diagonal, float offset,
                                  float radius)
          {
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

            return flatColour;
          }
        '''

        self._curve_kernel = Kernel.CreateFromSource(curveKernelSource)
        self._curve_image = Image.CreateFromKernel(self._curve_kernel)

        self._curve_kernel.SetValueForKey(0.707, 0.707, 'diagonal')
        self._curve_kernel.SetValueForKey(0.0, 'offset')

        self._flat_kernel = Kernel.CreateFromSource(flatKernelSource)
        self._flat_image = Image.CreateFromKernel(self._flat_kernel)

        self._flat_kernel.SetValueForKey(0.707, 0.707, 'diagonal')
        self._flat_kernel.SetValueForKey(0.0, 'offset')

        self.set_radius(40.0)

    def front_image(self):
        return self._front_image

    def back_image(self):
        return self._back_image

    def radius(self):
        return self._radius

    def set_radius(self, radius):
        self._radius = radius
        self._curve_kernel.SetValueForKey(radius, 'radius')
        self._flat_kernel.SetValueForKey(radius, 'radius')

    def set_front_image(self, im):
        self._front_image = im
        self._curve_kernel.SetValueForKey(self._front_image, 'front')
        self._flat_kernel.SetValueForKey(self._front_image, 'front')

        if(self._back_image != None):
            self._create_accum_if_necessary()

    def set_back_image(self, im):
        self._back_image = im
        self._curve_kernel.SetValueForKey(self._back_image, 'back')
        self._flat_kernel.SetValueForKey(self._back_image, 'back')

        if(self._front_image != None):
            self._create_accum_if_necessary()

    def _create_accum_if_necessary(self):
        if((self._front_image == None) or (self._back_image == None)):
            return

        unionRect = Rect2D.Union(self._front_image.GetExtent(), self._back_image.GetExtent())

        if((self._accum_image == None) or (not Rect2D.AreEqual(unionRect, self._accum_image.GetImage().GetExtent()))):
            self._accum_image = ImageAccumulator.Create(unionRect)

    def set_progress(self, p):
        if(self._accum_image == None):
            return

        extent = self._accum_image.GetImage().GetExtent()
        diagonal = math.sqrt(extent.Size.Width*extent.Size.Width + 
            extent.Size.Height*extent.Size.Height)
        p = 1.0 - p
        self._curve_kernel.SetValueForKey((diagonal + self._radius) * p - self._radius, 'offset')
        self._curve_kernel.SetValueForKey(extent.Size.Width/diagonal,
            extent.Size.Height/diagonal, 'diagonal')
        self._flat_kernel.SetValueForKey((diagonal + self._radius) * p - self._radius, 'offset')
        self._flat_kernel.SetValueForKey(extent.Size.Width/diagonal,
            extent.Size.Height/diagonal, 'diagonal')

    def get_output(self):
        self._accum_image.Clear()
        self._accum_image.RenderImage(self._back_image)
        self._accum_image.RenderImage(self._curve_image)
        self._accum_image.RenderImage(self._flat_image)

        return self._accum_image.GetImage()

class PageCurlTransitionFilter(ImageFilter):
    def __init__(self, update_cb):
        ImageFilter.__init__(self, update_cb)

        self._start_image = None
        self._end_image = None
        self._transition = PageCurlTransition()
        self._update_cb = update_cb
        self._radius = 0.0

        self.progress = 0.0
        self.end_image = self.get_null_image()
        self.radius = 100.0
    
    def _update(self):
        if((self._start_image == None) or (self._end_image == None)):
            return

        self._transition.set_front_image(self._start_image)
        self._transition.set_back_image(self._end_image)
        self._transition.set_radius(self._radius)
        self._transition.set_progress(self._progress)
        self._update_cb()
     
    def set_radius(self, radius):
        self._radius = radius
        self._update()
    
    def get_radius(self):
        return self._radius

    def set_progress(self, progress):
        self._progress = progress
        self._update()
    
    def get_progress(self):
        return self._progress
    
    def set_start_image(self, image):
        self._start_image = image
        self._update()
    
    def get_start_image(self):
        return self._start_image
    
    def set_end_image(self, image):
        self._end_image = image
        self._update()
    
    def get_end_image(self):
        return self._end_image

    def get_properties(self):
        return { 
            'start_image': ('Start', 'FirtreeImage', None),
            'end_image': ('End', 'FirtreeImage', None),
            'progress': ('Time', 'Range', (self._progress, 0.0, 1.0, 0.0)), # init, min, max, step
            'radius': ('Radius', 'Range', (self._radius, 0.0, 200.0, 0.0)), # init, min, max, step
        }

    def get_preferred_property_order(self):
        return [ 'start_image', 'end_image', 'progress', 'radius' ]
    
    def get_output_image(self):
        if((self._start_image == None) or (self._end_image == None)):
            return None
        return self._transition.get_output()
    
    def get_name(self):
        return 'PageCurl'

# vim:sw=4:ts=4:autoindent
