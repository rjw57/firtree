import gtk
import math

from Firtree import *
from ImageFilter import *

class WipeTransitionFilter(ImageFilter):
	def __init__(self, update_cb):
		ImageFilter.__init__(self, update_cb)

		self._start_image = None
		self._end_image = None
		self._update_cb = update_cb
		self._kernel = Kernel.CreateFromSource('''
			kernel vec4 filterKernel(sampler start, sampler end, 
				vec2 direction, float offset, float oneOverRadius)
			{
				float diagCoord = dot(destCoord(), direction) - offset;
				vec4 sCol = sample(start, samplerCoord(start));
				vec4 eCol = sample(end, samplerCoord(end));
				diagCoord = max(0.0, diagCoord);
				diagCoord *= oneOverRadius;
				diagCoord *= diagCoord;
				diagCoord = 1.0 - diagCoord;
				float lambda = max(0.0, diagCoord);
				return lambda * eCol + (1.0 - lambda) * sCol;
			}
		''')
		self._image = Image.CreateFromKernel(self._kernel)

		self.progress = 0.0
		self.radius = 100.0
		self.end_image = self.get_null_image()
	
	def _update_kernel(self):
		if((self._start_image == None) or (self._end_image == None)):
			return

		total_rect = Rect2D.Union(self._start_image.GetExtent(),
			self._end_image.GetExtent())
		diag_length = math.sqrt((total_rect.Size.Width*total_rect.Size.Width) +
			(total_rect.Size.Height*total_rect.Size.Height))

		self._kernel.SetValueForKey(total_rect.Size.Width / diag_length,
			total_rect.Size.Height / diag_length, 'direction')
		self._kernel.SetValueForKey(self._progress * (diag_length + self._radius) -
			self._radius, 'offset')
		self._kernel.SetValueForKey(1.0 / self._radius, 'oneOverRadius')
		self._kernel.SetValueForKey(self._start_image, 'start')
		self._kernel.SetValueForKey(self._end_image, 'end')
		self._update_cb()
		
	def set_radius(self, radius):
		self._radius = radius
		self._update_kernel()
	
	def get_radius(self):
		return self._radius

	def set_progress(self, progress):
		self._progress = progress
		self._update_kernel()
	
	def get_progress(self):
		return self._progress
	
	def set_start_image(self, image):
		self._start_image = image
		self._update_kernel()
	
	def get_start_image(self):
		return self._start_image
	
	def set_end_image(self, image):
		self._end_image = image
		self._update_kernel()
	
	def get_end_image(self):
		return self._end_image

	def get_properties(self):
		return { 
			'start_image': ('Start', 'FirtreeImage', None),
			'end_image': ('End', 'FirtreeImage', None),
			'progress': ('Time', 'Range', (self._progress, 0.0, 1.0, 0.0)), # init, min, max, step
			'radius': ('Radius', 'Range', (self._radius, 0.5, 200.0, 0.0)),
		}

	def get_preferred_property_order(self):
		return [ 'start_image', 'end_image', 'progress', 'radius' ]
	
	def get_output_image(self):
		if((self._start_image == None) or (self._end_image == None)):
			return None
		return self._image
	
	def get_name(self):
		return 'Wipe'

# vim:sw=4:ts=4:autoindent
