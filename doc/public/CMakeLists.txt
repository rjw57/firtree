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
    get_target_property(_firtree_output_name firtree OUTPUT_NAME)
	include(UseGtkDoc)
	gtk_doc_add_module(firtree ${CMAKE_SOURCE_DIR}/firtree
		#XML firtree-docs.xml # When the time comes, copy this from firtree-docs.xml
		# IGNOREHEADERS firtreetypebuiltins.h firtree-priv.h
        # CFLAGS -I../../firtree
        SUFFIXES h c hh cc # <- what files to scan
        LDFLAGS
            -L${CMAKE_CURRENT_BINARY_DIR}/../../firtree 
            -l${_firtree_output_name}
        LDPATH ${CMAKE_CURRENT_BINARY_DIR}/../../firtree
		DEPENDS firtree)
endif(NOT GTKDOC_FOUND)

# vim:sw=4:ts=4:autoindent:et
