#!/usr/bin/env python

import gtk			# Calls gtk module
import gtk.glade		# Calls gtk.glade module
import gtk.gtkgl		# Calls gtk.gtkgl module

from ImageRenderer import *

import sys
import os

from Firtree import *

from SimpleImageFilter import *
from ColouriseImageFilter import *
from CrossfadeTransitionFilter import *
from WipeTransitionFilter import *
from PageCurlTransitionFilter import *

filters = {
	'Image': SimpleImageFilter,
	'Colourise': ColouriseImageFilter,
	'Crossfade': CrossfadeTransitionFilter,
	'Wipe': WipeTransitionFilter,
	'Page Curl': PageCurlTransitionFilter,
}

class PlaygroundApp:
	def clearup(self):
		self._filters = None
		self._filter_box = None
		self._imageRenderer.destroy()
		del self._imageRenderer 

	def __init__(self, glade):
		self._imageRenderer = ImageRenderer()

		vbox = glade.get_widget('image_box')
		vbox.pack_start(self._imageRenderer)

		self._filters = []
		self._filter_box = glade.get_widget('filter_box')

		list_store = gtk.ListStore(str, object)
		self._filter_modules = []
		for k, v in filters.items():
			list_store.append([k, v])
			self._filter_modules.append(v)

		combo = glade.get_widget('filter_combo')
		combo.set_model(list_store)

		renderer = gtk.CellRendererText()
		combo.pack_start(renderer)
		combo.set_attributes(renderer, text = 0)
		combo.set_active(0)
		self._filter_combo = combo
	
	def create_widget_for_property(self, target, property_name):
		prop_dict = target.get_properties()
		if(not property_name in prop_dict):
			raise Exception('Unknown property "%s".' % property_name)
		(name, type, data) = prop_dict[property_name]
		widget = None

		if(type == 'Color'):
			widget = gtk.HBox(False, 3)
			widget.pack_start(gtk.Label('%s:' % name), False, True)
			colour_chooser = gtk.ColorButton()
			map = gtk.Entry().get_colormap()
			colour_chooser.set_color(map.alloc_color('white'))
			colour_chooser.set_use_alpha(True)
			setattr(target, property_name, (1.0, 1.0, 1.0, 1.0))

			def color_set_handler(widget):
				self._imageRenderer.get_context().Begin()
				col = widget.get_color()
				red = col.red / 65535.0
				green = col.green / 65535.0
				blue = col.blue / 65535.0
				alpha = widget.get_alpha() / 65535.0
				setattr(target, property_name, (red,green,blue,alpha))
				self._imageRenderer.get_context().End()

			colour_chooser.connect('color-set', color_set_handler)
			widget.pack_start(colour_chooser, True, True)
		elif(type == 'FirtreeImage'):
			widget = gtk.HBox(False, 3)
			widget.pack_start(gtk.Label('%s:' % name), False, True)
			file_chooser = gtk.FileChooserButton('Select image file')
			file_chooser.set_action(gtk.FILE_CHOOSER_ACTION_OPEN)

			def file_set_handler(widget):
				self._imageRenderer.get_context().Begin()
				image = Image.CreateFromFile(widget.get_filename())
				setattr(target, property_name, image)
				self._imageRenderer.get_context().End()

			file_chooser.connect('file-set', file_set_handler)
			widget.pack_start(file_chooser, True, True)
		elif(type == 'Range'):
			widget = gtk.HBox(False, 3)
			widget.pack_start(gtk.Label('%s:' % name), False, True)
			scale = gtk.HScale(gtk.Adjustment(data[0], data[1], data[2], data[3], data[3], data[3]))
			scale.set_digits(2)
			scale.set_value_pos(gtk.POS_RIGHT)

			def value_changed_handler(widget):
				self._imageRenderer.get_context().Begin()
				setattr(target, property_name, widget.get_value())
				self._imageRenderer.get_context().End()

			scale.connect('value-changed', value_changed_handler)
			widget.pack_start(scale, True, True)
		else:
			print('Warning: Unknown property type "%s".' % type)

		return widget
	
	def redraw(self):
		self._imageRenderer.get_context().Begin()

		images = []
		for f in self._filters:
			try:
				im = f.output_image
				if(im != None):
					images.append(im)
			except:
				print('Could not get output from filter %O.' % f)
		
		self._imageRenderer.set_images(images)

		self._imageRenderer.get_context().End()

	def add_new_filter(self):
		self._imageRenderer.get_context().Begin()
		module = self._filter_modules[self._filter_combo.get_active()]
		new_filter = module(self.redraw)
		self._filters.append(new_filter)
		
		vBox = gtk.VBox(False, 3)

		hBox = gtk.HBox(False, 1)
		label = gtk.Label()
		label.set_markup('<b>%s</b>' % new_filter.name)
		hBox.pack_start(label, True, True)

		delete_button = gtk.Button('')
		im = gtk.Image()
		im.set_from_stock(gtk.STOCK_REMOVE, gtk.ICON_SIZE_SMALL_TOOLBAR)
		delete_button.set_image(im)
		delete_button.set_relief(gtk.RELIEF_NONE)
		hBox.pack_start(delete_button, False, False)

		def delete_clicked_handler(widget):
			self._filter_box.remove(vBox)
			self._filters.remove(new_filter)
			self.redraw()
			widget.destroy()

		delete_button.connect('clicked', delete_clicked_handler)

		vBox.pack_start(hBox)

		prop_names = new_filter.preferred_property_order
		if(prop_names == None):
			prop_names = sorted(new_filter.get_properties().keys())

		for prop_name in prop_names:
			w = self.create_widget_for_property(new_filter, prop_name)
			if(w != None):
				vBox.pack_start(w, False, True)

		vBox.pack_start(gtk.HSeparator())
		vBox.show_all()

		self._filter_box.pack_start(vBox, False, True)

		self._imageRenderer.get_context().End()
		self.redraw()
	
	def on_main_window_delete_event(self, widget, event):
		gtk.main_quit()
	
	def on_add_button_clicked(self, widget):
		self.add_new_filter()


if(__name__ == '__main__'):
	print 'Firtree Playground Example'
	print '(C) 2008 Rich Wareham <richwareham@gmail.com>'
	print 'See LICENSE file for distribution rights'

	#==============================================================================
	# Use sys to find the script path and hence construct a path in which to 
	# search for support files.
	scriptDir = sys.path[0]

	# Load the UI
	playground_ui = gtk.glade.XML(os.path.join(scriptDir, 'playground_ui.glade'))

	# Load the main widget
	main_window = playground_ui.get_widget('main_window')

	# Create the app
	playgroundApp = PlaygroundApp(playground_ui)
	playground_ui.signal_autoconnect(playgroundApp)

	main_window.show_all()
	gtk.main()

	print('Exiting and clearing up...')
	playgroundApp.clearup()

# vim:sw=4:ts=4:autoindent
