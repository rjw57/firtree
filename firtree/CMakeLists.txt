# CMake project file for firtree
#
# Copyright (C) 2009 Rich Wareham <richwareham@gmail.com>
#
# See LICENSE file for distribution rights.

set(LLVM_CONFIG_COMPONENTS core 
    codegen linker bitreader
    jit native)

find_package(LLVM)
find_package(PythonInterp)

find_program(GLIB_MKENUMS glib-mkenums PATHS "${GLIB_PREFIX}/bin")

if(NOT GLIB_MKENUMS)
	message(SEND_ERROR "Could not find glib-mkenums which is required for firtree.")
endif(NOT GLIB_MKENUMS)

include_directories(${LLVM_INCLUDE_DIRS})

if(NOT APPLE)
    find_package(PkgConfig)
    pkg_check_modules(CAIRO cairo)
    pkg_check_modules(CLUTTER clutter-0.8)
    pkg_check_modules(GDK_PIXBUF gdk-pixbuf-2.0)
endif(NOT APPLE)

set(ENGINE_EXTRA_LIBS "")

set(FIRTREE_HAVE_CAIRO 0)
if(CAIRO_FOUND)
    set(FIRTREE_HAVE_CAIRO 1)
    include_directories(${CAIRO_INCLUDE_DIRS})
    set(ENGINE_EXTRA_LIBS ${ENGINE_EXTRA_LIBS} ${CAIRO_LIBRARIES})
endif(CAIRO_FOUND)

set(FIRTREE_HAVE_CLUTTER 0)
if(CLUTTER_FOUND)
    set(FIRTREE_HAVE_CLUTTER 1)
    include_directories(${CLUTTER_INCLUDE_DIRS})
    set(ENGINE_EXTRA_LIBS ${ENGINE_EXTRA_LIBS} ${CLUTTER_LIBRARIES})
endif(CLUTTER_FOUND)

set(FIRTREE_HAVE_GDK_PIXBUF 0)
if(GDK_PIXBUF_FOUND)
    set(FIRTREE_HAVE_GDK_PIXBUF 1)
    include_directories(${GDK_PIXBUF_INCLUDE_DIRS})
    set(ENGINE_EXTRA_LIBS ${ENGINE_EXTRA_LIBS} ${GDK_PIXBUF_LIBRARIES})
endif(GDK_PIXBUF_FOUND)

configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/firtree.h.in"
    "${CMAKE_CURRENT_SOURCE_DIR}/firtree.h"
    @ONLY)

set(_firtree_public_headers 
    firtree-affine-transform.h
    firtree-buffer-sampler.h
    firtree-debug.h
    firtree-lock-free-set.h
    firtree-kernel.h
    firtree-kernel-sampler.h
    firtree-sampler.h
    firtree-types.h
    firtree-vector.h
    firtree.h)

set(_firtree_source_files
    ${CMAKE_CURRENT_BINARY_DIR}/firtree-type-builtins.c

    firtree-affine-transform.c
    firtree-buffer-sampler.cc
    firtree-debug.cc
    firtree-engine.cc
    firtree-lock-free-set.c
    firtree-kernel.cc
    firtree-kernel-sampler.cc
    firtree-sampler.cc
    firtree-vector.c

    ${LLVM_STATIC_OBJS})

if(FIRTREE_HAVE_CAIRO)
    set(_firtree_public_headers ${_firtree_public_headers}
        firtree-cairo-surface-sampler.h)
    set(_firtree_source_files ${_firtree_source_files} 
        firtree-cairo-surface-sampler.cc)
endif(FIRTREE_HAVE_CAIRO)

if(FIRTREE_HAVE_CLUTTER)
    set(_firtree_public_headers ${_firtree_public_headers}
        firtree-cogl-texture-sampler.h)
    set(_firtree_source_files ${_firtree_source_files} 
        firtree-cogl-texture-sampler.cc)
endif(FIRTREE_HAVE_CLUTTER)

if(FIRTREE_HAVE_GDK_PIXBUF)
    set(_firtree_public_headers ${_firtree_public_headers}
        firtree-pixbuf-sampler.h)
    set(_firtree_source_files ${_firtree_source_files} 
        firtree-pixbuf-sampler.cc)
endif(FIRTREE_HAVE_GDK_PIXBUF)
    
add_custom_command(OUTPUT firtree-type-builtins.h ${CMAKE_CURRENT_BINARY_DIR}/firtree-type-builtins.c
	COMMAND ${GLIB_MKENUMS} 
		--template firtree-type-builtins.h.template 
		${_firtree_public_headers} > firtree-type-builtins.h
	COMMAND ${GLIB_MKENUMS} 
		--template firtree-type-builtins.c.template
		${_firtree_public_headers} > ${CMAKE_CURRENT_BINARY_DIR}/firtree-type-builtins.c
    WORKING_DIRECTORY
        ${CMAKE_CURRENT_SOURCE_DIR}
	DEPENDS 
		${_firtree_public_headers}
		firtree-type-builtins.h.template 
		firtree-type-builtins.c.template
	VERBATIM)

include_directories("${CMAKE_CURRENT_SOURCE_DIR}")

set(_firtree_internal_headers
    internal/firtree-cogl-texture-sampler-intl.hh
    internal/firtree-engine-intl.hh
    internal/firtree-kernel-intl.hh
    internal/firtree-sampler-intl.hh

    firtree-type-builtins.h
)

add_library(firtree SHARED
    ${_firtree_public_headers}
    ${_firtree_internal_headers}
    ${_firtree_source_files})
    
set_source_files_properties(
    firtree-type-builtins.h
    firtree-type-builtins.c
    PROPERTIES GENERATED TRUE)

# set the library version.
set_target_properties(firtree 
    PROPERTIES
    VERSION 
        "${FIRTREE_VERSION_MAJOR}.${FIRTREE_VERSION_MINOR}.${FIRTREE_VERSION_PATCH}"
    SOVERSION 
        "${FIRTREE_VERSION_MAJOR}"
    OUTPUT_NAME
        "firtree-core-${FIRTREE_VERSION_MAJOR}.${FIRTREE_VERSION_MINOR}"
    )

target_link_libraries(firtree
    common llvm-frontend 
    ${GLIB_LIBRARIES}
    ${ENGINE_EXTRA_LIBS}
    ${LLVM_STATIC_LIBS})

# The magic runes required to install a library.
install(TARGETS firtree
  RUNTIME DESTINATION bin
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib
)

set(FIRTREE_HEADER_INSTALL_DEST "include/firtree-${FIRTREE_VERSION_MAJOR}.${FIRTREE_VERSION_MINOR}")

# Install the public headers
install(FILES ${_firtree_public_headers} firtree-type-builtins.h DESTINATION "${FIRTREE_HEADER_INSTALL_DEST}/firtree")
install(FILES ${_firtree_internal_headers} DESTINATION "${FIRTREE_HEADER_INSTALL_DEST}/firtree/internal")

# Update the firtree pkg-config file
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/firtree.pc.in")
    configure_file(
        "${CMAKE_CURRENT_SOURCE_DIR}/firtree.pc.in"
        "${CMAKE_CURRENT_SOURCE_DIR}/firtree.pc"
        @ONLY)
endif(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/firtree.pc.in")
install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/firtree.pc" DESTINATION lib/pkgconfig)

# Build the Engines
add_subdirectory(engines)

# vim:sw=4:ts=4:autoindent:et

