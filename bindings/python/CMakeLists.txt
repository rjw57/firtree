# CMake project file for firtree
#
# Copyright (C) 2009 Rich Wareham <richwareham@gmail.com>
#
# See LICENSE file for distribution rights.

# Attempt to find pygobject
find_package(PyGObject)

set(BUILD_PYTHON_BINDINGS 1)

if(NOT APPLE)
    find_package(PkgConfig)
    pkg_check_modules(CAIRO cairo)
    pkg_check_modules(CLUTTER clutter-1.0)
    if(NOT CLUTTER_FOUND)
        pkg_check_modules(CLUTTER clutter-0.8)
        if(CLUTTER_FOUND)
            pkg_check_modules(PYCLUTTER REQUIRED pyclutter-0.8)
        endif(CLUTTER_FOUND)
    else(NOT CLUTTER_FOUND)
        pkg_check_modules(PYCLUTTER REQUIRED pyclutter-1.0)
    endif(NOT CLUTTER_FOUND)
    if(CAIRO_FOUND)
        pkg_check_modules(PYCAIRO REQUIRED pycairo)
    endif(CAIRO_FOUND)
endif(NOT APPLE)

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

	pygobject_target_add_bindings(FIRTREE_BINDINGS firtree MODULEPREFIX pyfirtree
        DEFS 
            firtree-core-types.defs firtree-core.defs firtree-core-overriden.defs 
            firtree-engine-cpu-types.defs firtree-engine-cpu.defs firtree-engine-cpu-overriden.defs 
            firtree-cairo-types.defs firtree-cairo.defs # firtree-cairo-overriden.defs 
            firtree-clutter-types.defs firtree-clutter.defs # firtree-clutter-overriden.defs 
            firtree-gdk-pixbuf-types.defs firtree-gdk-pixbuf.defs # firtree-gdk-pixbuf-overriden.defs 
        REGISTER 
            /usr/share/pygtk/2.0/defs/gdk-types.defs
            /usr/share/pyclutter/1.0/defs/clutter-types.defs
        OVERRIDE firtree.override)
	include_directories(
        ${FIRTREE_BINDINGS_INCLUDE_DIRS}
        ${PYGOBJECT_PYGTK_INCLUDE_DIRS}
        ${CLUTTER_INCLUDE_DIRS}
        ${CAIRO_INCLUDE_DIRS}
        ${PYCLUTTER_INCLUDE_DIRS}
        ${PYCAIRO_INCLUDE_DIRS})
	add_library(pyfirtree MODULE firtreemodule.c
        ${FIRTREE_BINDINGS_SOURCES})
	link_directories(${SVFBINDINGS_LINK_DIRS})
	target_link_libraries(pyfirtree firtree firtree-engine-cpu
        ${FIRTREE_BINDINGS_LIBRARIES}
        ${CAIRO_LIBRARIES}
        ${CLUTTER_LIBRARIES}
        ${PYCLUTTER_LIBRARIES}
        ${PYCAIRO_LIBRARIES})
    set_target_properties(pyfirtree PROPERTIES PREFIX "")

    # Find where to install python libraries.
    execute_process(
      COMMAND
      ${PYTHON_EXECUTABLE} -c 
      "from distutils import sysconfig; print sysconfig.get_python_lib(1,0,prefix='${CMAKE_INSTALL_EXEC_PREFIX}')"
      OUTPUT_VARIABLE PYTHON_INSTDIR
      OUTPUT_STRIP_TRAILING_WHITESPACE
    )

    install(TARGETS pyfirtree DESTINATION ${PYTHON_INSTDIR})
endif(BUILD_PYTHON_BINDINGS)

# vim:sw=4:ts=4:autoindent:et
