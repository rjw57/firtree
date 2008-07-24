#!/usr/bin/env python

import gtk			# Calls gtk module
import gtk.glade		# Calls gtk.glade module
import gtk.gtkgl		# Calls gtk.gtkgl module

from OpenGL.GL import *
from OpenGL.GLU import *

from ImageRenderer import *

import sys
import os
import gc

import Firtree

class ChromaKeyApp:
	def __init__(self, mainWindow, glade):
		self._mainWindow = mainWindow
		self._imageRenderer = ImageRenderer()

		self._white_level_scale = glade.get_widget('white_level_scale')
		self._black_level_scale = glade.get_widget('black_level_scale')
		self._bg_color_button = glade.get_widget('bg_color_button')
		self._display_original = glade.get_widget('display_original')
		self._display_matte = glade.get_widget('display_matte')
		self._display_composite = glade.get_widget('display_composite')
		self._orig_image = None

		vbox = glade.get_widget('image_box')
		vbox.pack_start(self._imageRenderer)

		# Create a checkerboard image
		self._checker_kernel = Firtree.Kernel.CreateFromSource('''
	 		kernel vec4 checkerKernel(__color foreColor, __color backColor)
		    {
				float squareSize = 32.0;
        		vec2 dc = mod(destCoord(), squareSize*2.0);
		        vec2 discriminant = step(squareSize, dc);
        		float flag = discriminant.x + discriminant.y - 
                		     2.0*discriminant.x*discriminant.y;
		        return mix(foreColor, backColor, flag);
    		}
		''')
		checkerImage = Firtree.Image.CreateFromKernel(self._checker_kernel)

		# Create a composite 'over' kernel.
		overKernel = Firtree.Kernel.CreateFromSource('''
			kernel vec4 overKernel(sampler over, sampler under)
			{
				vec4 overCol = sample(over, samplerCoord(over));
				vec4 underCol = sample(under, samplerCoord(under));
				return overCol + underCol * (1.0 - overCol.a);
			}
		''')
		self._composite_image = Firtree.Image.CreateFromKernel(overKernel)

		# Compute an alpha matte from an image
		self._matteKernel = Firtree.Kernel.CreateFromSource('''
			vec4 rgb2yuv(vec4 srcCol)
			{
				vec4 rgb2y = vec4(0.299, 0.587, 0.114, 0.0);
				vec4 rgb2u = vec4(-0.147, -0.289, 0.436, 0.0);
				vec4 rgb2v = vec4(0.615, -0.515, -0.1, 0.0);

				vec4 yuvCol = vec4(
					dot(srcCol, rgb2y),
					dot(srcCol, rgb2u),
					dot(srcCol, rgb2v),
					srcCol.a
				);

				return yuvCol;
			}

			kernel vec4 matteKernel(sampler source, float whiteLevel, float blackLevel,
				__color bgColor)
			{
				bgColor = rgb2yuv(bgColor);
				vec4 srcCol = sample(source, samplerCoord(source));
				vec4 yuvCol = rgb2yuv(srcCol);

				vec2 delta = yuvCol.gb - bgColor.gb;
				float colourDelta = dot(vec2(1,1), abs(delta));

				colourDelta -= blackLevel;
				colourDelta /= (whiteLevel - blackLevel);

				float matte = max(0.0, min(1.0, colourDelta));

				return vec4(matte, matte, matte, matte) * srcCol.a;
			}
		''')
		self._matte_image = Firtree.Image.CreateFromKernel(self._matteKernel)
		
		# Set the alpha value for an image
		self._alphaKernel = Firtree.Kernel.CreateFromSource('''
			kernel vec4 alphaKernel(sampler source, sampler matte,
				__color bgColor)
			{
				vec4 srcCol = sample(source, samplerCoord(source));
				vec4 matteCol = sample(matte, samplerCoord(matte));

				vec4 result = srcCol + bgColor*(matteCol.a-1.0);

				return vec4(result.rgb * matteCol.a, matteCol.a);
			}
		''')
		self._alpha_image = Firtree.Image.CreateFromKernel(self._alphaKernel)
		self._alphaKernel.SetValueForKey(self._matte_image, 'matte')

		# Create a blank image
		self._blank_kernel = Firtree.Kernel.CreateFromSource('''
			kernel vec4 blankKernel() { return vec4(0,0,0,0); }
			''')
		blankImage = Firtree.Image.CreateFromKernel(self._blank_kernel)
		self._alphaKernel.SetValueForKey(blankImage, 'source')
		self._matteKernel.SetValueForKey(blankImage, 'source')

		self._compositeKernel = overKernel
		self._compositeKernel.SetValueForKey(blankImage, 'over')

		self.set_display_image(blankImage, True)

		overKernel.SetValueForKey(checkerImage, 'under')

		self._imageRenderer.set_image(self._composite_image)

		self.sync_kernels_to_controls()
	
	def set_display_image(self, image, with_checks = True):
		if(with_checks):
			self._checker_kernel.SetValueForKey(0.25,0.25,0.25,1, 'backColor')
			self._checker_kernel.SetValueForKey(0.75,0.75,0.75,1, 'foreColor')
		else:
			self._checker_kernel.SetValueForKey(0.0,0.0,0.0,1, 'backColor')
			self._checker_kernel.SetValueForKey(0.0,0.0,0.0,1, 'foreColor')
		
		self._compositeKernel.SetValueForKey(image, 'over')

	def open_image(self, filename):
		self._orig_image = Firtree.Image.CreateFromFile(filename)

		extent = self._orig_image.GetExtent()
		print('Extent: %f,%f+%f+%f' % (extent.Origin.X, extent.Origin.Y, extent.Size.Width, extent.Size.Height))

		self._alphaKernel.SetValueForKey(self._orig_image, 'source')
		self._matteKernel.SetValueForKey(self._orig_image, 'source')

		self.set_display_image(self._orig_image)
		self._display_original.set_active(True)

		extent = self._alpha_image.GetExtent()
		print('Alpha extent: %f,%f+%f+%f' % (extent.Origin.X, extent.Origin.Y, extent.Size.Width, extent.Size.Height))

		self._imageRenderer.zoom_to_rect(self._orig_image.GetExtent())

	def on_main_window_delete_event(self, widget, event):
		gtk.main_quit()

	def on_quit_activate(self, widget):
		gtk.main_quit()
	
	def on_save_activate(self, widget):
		print('SAVE!')
		self._imageRenderer.get_context().WriteImageToFile(
			self._alpha_image, 'foo.png')

	def sync_kernels_to_controls(self):
		whiteLevel = self._white_level_scale.get_value()
		self._matteKernel.SetValueForKey(whiteLevel, 'whiteLevel')
		blackLevel = self._black_level_scale.get_value()
		self._matteKernel.SetValueForKey(blackLevel, 'blackLevel')

		bgColor = self._bg_color_button.get_color()
		self._matteKernel.SetValueForKey(
			float(bgColor.red) / 65535.0, 
			float(bgColor.green) / 65535.0, 
			float(bgColor.blue) / 65535.0, 
			1.0, 'bgColor')

		self._alphaKernel.SetValueForKey(
			float(bgColor.red) / 65535.0, 
			float(bgColor.green) / 65535.0, 
			float(bgColor.blue) / 65535.0, 
			1.0, 'bgColor')

		if(self._orig_image != None):
			if(self._display_original.get_active()):
				self.set_display_image(self._orig_image)
			elif(self._display_matte.get_active()):
				self.set_display_image(self._matte_image, False)
			elif(self._display_composite.get_active()):
				self.set_display_image(self._alpha_image)

		self._imageRenderer.refresh()
	
	def on_display_toggled(self, widget):
		self.sync_kernels_to_controls()
	
	def on_bg_color_button_color_set(self, widget):
		self.sync_kernels_to_controls()
	
	def on_white_level_scale_value_changed(self, widget):
		self.sync_kernels_to_controls()
	
	def on_black_level_scale_value_changed(self, widget):
		self.sync_kernels_to_controls()
	
	def on_open_activate(self, widget):
		# Create a file chooser
		chooser = gtk.FileChooserDialog(title=None, 
			action=gtk.FILE_CHOOSER_ACTION_OPEN,
			buttons=(gtk.STOCK_CANCEL,gtk.RESPONSE_CANCEL,gtk.STOCK_OPEN,gtk.RESPONSE_OK))

		response = chooser.run()

		if(response == gtk.RESPONSE_OK):
			self.open_image(chooser.get_filename())
		
		chooser.destroy()


if(__name__ == '__main__'):
	print 'Firtree Chroma Key Example'
	print '(C) 2008 Rich Wareham <richwareham@gmail.com>'
	print 'See LICENSE file for distribution rights'

	#==============================================================================
	# Use sys to find the script path and hence construct a path in which to 
	# search for support files.
	scriptDir = sys.path[0]

	# Load the UI
	chromakey_ui = gtk.glade.XML(os.path.join(scriptDir, 'chromakey_ui.glade'))

	# Load the main widget
	main_window = chromakey_ui.get_widget('main_window')

	# Create the app
	chromaKeyApp = ChromaKeyApp(main_window, chromakey_ui)
	chromakey_ui.signal_autoconnect(chromaKeyApp)

	main_window.show_all()
	gtk.main()

	print('Exiting and clearing up...')
	gc.collect()


# vim:sw=4:ts=4:autoindent
