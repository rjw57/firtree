import gtk
import gtk.gdk

from Firtree import *
from ImageFilter import *

class FalseColourImageFilter(ImageFilter):
	def __init__(self, update_cb):
		ImageFilter.__init__(self, update_cb)

		self._input_image = None
		self._update_cb = update_cb
		self._kernel = Kernel.CreateFromSource('''
			kernel vec4 colouriseKernel(sampler src, __color posColour, __color negColour)
			{
				vec4 inColour = unpremultiply(sample(src, samplerCoord(src)));
				vec4 rgb2y = vec4(0.299, 0.587, 0.114, 0.0);
				float y = dot(rgb2y, inColour) * inColour.a;

				vec4 outColour = posColour * y + negColour * (1.0-y);
				outColour *= inColour.a;
				return outColour;
			}
		''')
		self._image = Image.CreateFromKernel(self._kernel)

		self.input_image = self.get_null_image()
		self.pos_color = (1.0,1.0,1.0,1.0)
		self.neg_color = (0.0,0.0,0.0,1.0)
	
	def get_properties(self):
		# Return a dictionary ot tuples. The key of the dictionary is
		# the property name, the value is a pair containing the 
		# human name, type and some type-specific data
		return {
			'input_image': ('Image', 'FirtreeImage', None),
			'pos_color': ('Positive Colour', 'Color', None),
			'neg_color': ('Negative Colour', 'Color', None),
		}
	
	def get_name(self):
		return 'False Colour'
		
	def get_neg_color(self):
		return self._neg_color

	def set_neg_color(self, r, g, b, a):
		self._neg_color = (r,g,b,a)
		self._kernel.SetValueForKey(r, g, b, a, 'negColour')
		self._update_cb()

	def get_pos_color(self):
		return self._pos_color

	def set_pos_color(self, r, g, b, a):
		self._pos_color = (r,g,b,a)
		self._kernel.SetValueForKey(r, g, b, a, 'posColour')
		self._update_cb()

	def get_input_image(self, image):
		return self._input_image
	
	def set_input_image(self, image):
		self._input_image = image
		self._kernel.SetValueForKey(self._input_image, 'src')
		self._update_cb()
	
	def get_output_image(self):
		if(self._input_image == None):
			return None
		return self._image

# vim:sw=4:ts=4:autoindent
