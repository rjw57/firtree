import gtk

from Firtree import *
from ImageFilter import *

class SimpleImageFilter(ImageFilter):
	def __init__(self, update_cb):
		ImageFilter.__init__(self, update_cb)

		self._image = None
		self._update_cb = update_cb
	
	def set_input_image(self, image):
		self._image = image
		self._update_cb()
	
	def get_input_image(self):
		return self._image

	def get_properties(self):
		return { 'input_image': ('Image', 'FirtreeImage', None) }
	
	def get_output_image(self):
		return self._image
	
	def get_name(self):
		return 'Image'

# vim:sw=4:ts=4:autoindent
