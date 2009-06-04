# CMake project file for firtree
#
# Copyright (C) 2009 Rich Wareham <richwareham@gmail.com>
#
# See LICENSE file for distribution rights.

# The name of this project is 'Firtree'.
project(Firtree)

# Set the minimum CMake version
cmake_minimum_required(VERSION 2.6)

# Update the module path to include any extra CMake modiles we might ship.
set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake/Modules")

# Set some variables indicating the version number of Firtree.
set(FIRTREE_VERSION_MAJOR "0")
set(FIRTREE_VERSION_MINOR "1")
set(FIRTREE_VERSION_PATCH "0")
set(FIRTREE_VERSION "${FIRTREE_VERSION_MAJOR}.${FIRTREE_VERSION_MINOR}.${FIRTREE_VERSION_PATCH}")

set(CMAKE_C_FLAGS ${CMAKE_C_FLAGS} -pthread)
set(CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS} -pthread)

# Add the root source dir as an include dir
include_directories("${CMAKE_SOURCE_DIR}")

if(NOT APPLE)
    # We use pkg-config to fing glib et al
    find_package(PkgConfig)
    # Find glib et al
    pkg_check_modules(GLIB REQUIRED glib-2.0 gobject-2.0)
    include_directories(${GLIB_INCLUDE_DIRS})
else(NOT APPLE)
    find_library(GLIB_LIBRARIES GLib)
    if(NOT GLIB_LIBRARIES)
        message(SEND_ERROR "GLib framework is required.")
    endif(NOT GLIB_LIBRARIES)

    find_path(GLIB_INCLUDE_DIRS glib.h)
    include_directories(${GLIB_INCLUDE_DIRS})
endif(NOT APPLE)

# Magic to set GCC-specific compile flags
set(GCC_FLAGS "-Wall -O9 -g")
if(CMAKE_COMPILER_IS_GNUCC)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${GCC_FLAGS}")
endif(CMAKE_COMPILER_IS_GNUCC)
if(CMAKE_COMPILER_IS_GNUCXX)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${GCC_FLAGS}")
endif(CMAKE_COMPILER_IS_GNUCXX)

# Build the common C++ support libraries
add_subdirectory(common)

# Build the Kernel language -> LLVM frontend.
add_subdirectory(llvm-frontend)

# Build the example command line tools
add_subdirectory(tools)

# The Firtree library itself
add_subdirectory(firtree)

# Build the library bindings
add_subdirectory(bindings)

# Build the documentation
add_subdirectory(doc)

# The test suite
add_subdirectory(testing)

# vim:sw=4:ts=4:autoindent:et

