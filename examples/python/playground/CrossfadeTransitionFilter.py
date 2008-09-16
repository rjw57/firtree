from Firtree import *
from ImageFilter import *

class CrossfadeTransitionFilter(ImageFilter):
	def __init__(self, update_cb):
		ImageFilter.__init__(self, update_cb)

		self._start_image = None
		self._end_image = None
		self._update_cb = update_cb
		self._kernel = Kernel.CreateFromSource('''
			kernel vec4 filterKernel(sampler start, sampler end, float lambda)
			{
				vec4 sCol = sample(start, samplerCoord(start));
				vec4 eCol = sample(end, samplerCoord(end));
				return lambda * eCol + (1.0 - lambda) * sCol;
			}
		''')
		self._image = Image.CreateFromKernel(self._kernel)

		self.progress = 0.0
		self.end_image = self.get_null_image()
	
	def set_progress(self, progress):
		self._progress = progress
		self._kernel.SetValueForKey(float(progress), 'lambda')
		self._update_cb()
	
	def get_progress(self):
		return self._progress
	
	def set_start_image(self, image):
		self._start_image = image
		self._kernel.SetValueForKey(image, 'start')
		self._update_cb()
	
	def get_start_image(self):
		return self._start_image
	
	def set_end_image(self, image):
		self._end_image = image
		self._kernel.SetValueForKey(image, 'end')
		self._update_cb()
	
	def get_end_image(self):
		return self._end_image

	def get_properties(self):
		return { 
			'start_image': ('Start', 'FirtreeImage', None),
			'end_image': ('End', 'FirtreeImage', None),
			'progress': ('Time', 'Range', (0.0, 0.0, 1.0, 0.0)), # init, min, max, step
		}

	def get_preferred_property_order(self):
		return [ 'start_image', 'end_image', 'progress' ]
	
	def get_output_image(self):
		if((self._start_image == None) or (self._end_image == None)):
			return None
		return self._image
	
	def get_name(self):
		return 'Crossfade'

# vim:sw=4:ts=4:autoindent
