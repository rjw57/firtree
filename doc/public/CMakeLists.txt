# CMake project file for firtree
#
# Copyright (C) 2009 Rich Wareham <richwareham@gmail.com>
#
# See LICENSE file for distribution rights.

# Build the public documentation.

find_package(GtkDoc)
if(NOT GTKDOC_FOUND)
	message(STATUS "gtkdoc was not found, public documentation will not be built.")
else(NOT GTKDOC_FOUND)
	include(UseGtkDoc)
	gtk_doc_add_module(firtree ${CMAKE_SOURCE_DIR}/firtree 
		#XML firtree-docs.xml # When the time comes, copy this from firtree-docs.xml
		# IGNOREHEADERS firtreetypebuiltins.h firtree-priv.h
		DEPENDS firtree)
endif(NOT GTKDOC_FOUND)

# vim:sw=4:ts=4:autoindent:et
