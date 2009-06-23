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

find_program(LLVM_GCC_EXECUTABLE llvm-gcc)

if(NOT LLVM_GCC_EXECUTABLE)
    message(SEND_ERROR "llvm-gcc not found.")
endif(NOT LLVM_GCC_EXECUTABLE)

if(NOT PYTHONINTERP_FOUND)
    message(SEND_ERROR "Python interpreter not found.")
endif(NOT PYTHONINTERP_FOUND)

include_directories(${LLVM_INCLUDE_DIRS})

if(NOT APPLE)
    find_package(PkgConfig)
    pkg_check_modules(CAIRO cairo)
    pkg_check_modules(CLUTTER clutter-0.8)
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

configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/firtree.h.in"
    "${CMAKE_CURRENT_SOURCE_DIR}/firtree.h"
    @ONLY)

add_custom_command(
    OUTPUT 
        engines/cpu/llvm-cpu-support.bc.h
        engines/cpu/llvm-cpu-support.opt.bc
        engines/cpu/llvm-cpu-support.bc
        engines/cpu/render-buffer.o
        engines/cpu/builtins.bc
    COMMAND ${LLVM_AS_EXECUTABLE}
        -f builtins.ll -o builtins.bc
    COMMAND ${LLVM_GCC_EXECUTABLE} 
        -fno-math-errno
        -o render-buffer.o
        -c -emit-llvm
        render-buffer.c
    COMMAND ${LLVM_LINK_EXECUTABLE}
        -f builtins.bc render-buffer.o -o llvm-cpu-support.bc
    COMMAND ${LLVM_OPT_EXECUTABLE} 
        -f -std-compile-opts -o llvm-cpu-support.opt.bc llvm-cpu-support.bc
    COMMAND ${PYTHON_EXECUTABLE} ../../../tools/bin2h.py
        llvm-cpu-support.opt.bc llvm-cpu-support.bc.h
    WORKING_DIRECTORY
        ${CMAKE_CURRENT_SOURCE_DIR}/engines/cpu
    DEPENDS
        engines/cpu/builtins.ll
        engines/cpu/render-buffer.c)

set(_firtree_public_headers 

    firtree-affine-transform.h
    firtree-cairo-surface-sampler.h
    firtree-cogl-texture-sampler.h
    firtree-debug.h
    firtree-kernel.h
    firtree-kernel-sampler.h
    firtree-pixbuf-sampler.h
    firtree-sampler.h
    firtree-vector.h
    firtree.h)
    

add_library(firtree SHARED
    ${_firtree_public_headers}

    engines/cpu/firtree-cpu-engine.h

    firtree-affine-transform.c
    firtree-cairo-surface-sampler.cc
    firtree-cogl-texture-sampler.cc
    firtree-debug.cc
    firtree-kernel.cc
    firtree-kernel-sampler.cc
    firtree-pixbuf-sampler.cc
    firtree-sampler.cc
    firtree-vector.c

    firtree-engine.cc

    internal/firtree-cogl-texture-sampler-intl.hh
    internal/firtree-engine-intl.hh
    internal/firtree-kernel-intl.hh
    internal/firtree-sampler-intl.hh

    # CPU rendering library
    engines/cpu/llvm-cpu-support.bc.h
    engines/cpu/firtree-cpu-engine.cc

    ${LLVM_STATIC_OBJS})
    
set_source_files_properties(
    engines/cpu/llvm-cpu-support.bc.h
    engines/cpu/firtree-cpu-engine.h
    PROPERTIES GENERATED TRUE)

# set the library version.
set_target_properties(firtree 
    PROPERTIES
    VERSION 
        "${FIRTREE_VERSION_MAJOR}.${FIRTREE_VERSION_MINOR}.${FIRTREE_VERSION_PATCH}"
    SOVERSION 
        "${FIRTREE_VERSION_MAJOR}.${FIRTREE_VERSION_MINOR}.${FIRTREE_VERSION_PATCH}"
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

# Install the public headers
install(FILES ${_firtree_public_headers} DESTINATION include/firtree)
install(FILES engines/cpu/firtree-cpu-engine.h DESTINATION include/firtree/engines/cpu)

# Update the firtree pkg-config file
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/firtree.pc.in")
    configure_file(
        "${CMAKE_CURRENT_SOURCE_DIR}/firtree.pc.in"
        "${CMAKE_CURRENT_SOURCE_DIR}/firtree.pc"
        @ONLY)
endif(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/firtree.pc.in")
install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/firtree.pc" DESTINATION lib/pkgconfig)

# vim:sw=4:ts=4:autoindent:et

