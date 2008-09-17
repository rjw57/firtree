import gtk

from Firtree import *
from ImageFilter import *

class RandomFilter(ImageFilter):
	def __init__(self, update_cb):
		ImageFilter.__init__(self, update_cb)

		self._update_cb = update_cb

		randKernel = Kernel.CreateFromSource('''
			vec4 mccool_rand(vec4 seed)
			{
				// From:
				//  McCool, M.C. and W. Heidrich. Texture Shaders. 1999
				//  SIGGRAPH/Eurographics Workshop on Graphics Hardware, Aug.
				//  1999, pp. 117-126.

				// The constant below is equivalient to: 
				// float pi = 3.14159;
				// vec4 a = vec4(pi * pi * pi * pi, exp(4.0),
				//	pow(13.0, pi / 2.0), sqrt(1997.0));

				vec4 a = vec4(97.40876, 54.59815, 56.204499, 44.6878);

				vec4 result = seed;

				for(int i=0; i<3; i++)
				{
					result.x = fract(dot(result, a));
					result.y = fract(dot(result, a));
					result.z = fract(dot(result, a));
					result.w = fract(dot(result, a));
				}

				return result;
			}

			kernel vec4 randKernel()
			{
				return mccool_rand(vec4(destCoord(), destCoord()));
			}
		''')
		self._image = Image.CreateFromKernel(randKernel)
	
	def get_properties(self):
		return { }
	
	def get_output_image(self):
		return self._image
	
	def get_name(self):
		return 'Random'

# vim:sw=4:ts=4:autoindent
