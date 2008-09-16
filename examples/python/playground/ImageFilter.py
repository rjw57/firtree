import gtk
import gtk.gdk

from Firtree import *

class ImageFilter:
	def __init__(self, update_cb):
		pass
	
	def get_properties(self):
		''' Return a dictionary ot tuples. The key of the dictionary is
		    the property name, the value is a pair containing the 
		    human name, type and some type-specific data. '''
		return { }
	
	def __getattr__(self, name):
		accessor_name = 'get_%s' % name
		accessor = None
		try:
			accessor = getattr(self, accessor_name)
		except:
			raise AttributeError('No such attribute, "%s".' % name)
		return accessor()
	
	def __setattr__(self, name, value):
		accessor_name = 'set_%s' % name
		accessor = None
		try:
			accessor = getattr(self, accessor_name)
		except:
			self.__dict__[name] = value
		if(accessor != None):
			if(hasattr(value, '__iter__') and (len(value) > 1)):
				accessor(*value)
			else:
				accessor(value)
	
	def get_name(self):
		return 'IMAGE FILTER'
	
	def get_preferred_property_order(self):
		return None
	
	def get_output_image(self):
		return None

# vim:sw=4:ts=4:autoindent
