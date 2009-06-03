# CMake project file for firtree
#
# Copyright (C) 2009 Rich Wareham <richwareham@gmail.com>
#
# See LICENSE file for distribution rights.

# Attempt to find pygobject
find_package(PyGObject)

# For finding things like glib, etc.
find_package(PkgConfig)

set(BUILD_PYTHON_BINDINGS 1)

if(NOT PYGOBJECT_FOUND)
	message(STATUS "PyGObject could not be found, Python bindings will not be built.")
	set(BUILD_PYTHON_BINDINGS 0)
endif(NOT PYGOBJECT_FOUND)

if(BUILD_PYTHON_BINDINGS)
    if(CMAKE_COMPILER_IS_GNUCC)
        # warning 'fix' for GCC warning about a lack of 
        # long long in C90.
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wno-long-long")
    endif(CMAKE_COMPILER_IS_GNUCC)

	pygobject_target_add_bindings(SVFBINDINGS firtree MODULEPREFIX pyfirtree
        DEFS firtree-kernel.defs OVERRIDE firtree.override)
	include_directories(${SVFBINDINGS_INCLUDE_DIRS} ${PYGOBJECT_PYGTK_INCLUDE_DIRS})
	add_library(pyfirtree MODULE firtreemodule.c ${SVFBINDINGS_SOURCES})
	link_directories(${SVFBINDINGS_LINK_DIRS})
	target_link_libraries(pyfirtree firtree ${SVFBINDINGS_LIBRARIES})
    set_target_properties(pyfirtree PROPERTIES PREFIX "")
endif(BUILD_PYTHON_BINDINGS)

# vim:sw=4:ts=4:autoindent:et
