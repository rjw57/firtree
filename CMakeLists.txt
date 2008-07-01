# CMake project file for firtree
#
# Copyright (C) 2007, 2008 Rich Wareham <richwareham@gmail.com>
#
# See LICENSE file for distribution rights.

# This project is called FIRTREE 
PROJECT(FIRTREE)

# Set the minimum required version of cmake
CMAKE_MINIMUM_REQUIRED(VERSION 2.4)

# New linking policy in CMake 2.6. See 'cmake --help-policy CMP0003'
IF(COMMAND cmake_policy)
    cmake_policy(SET CMP0003 NEW)
ENDIF(COMMAND cmake_policy)

# For the moment, force Debug builds.
SET(CMAKE_BUILD_TYPE "Debug")

# Set some variables indicating the version number of Firtree.
SET(FIRTREE_VERSION_MAJOR "0")
SET(FIRTREE_VERSION_MINOR "0")
SET(FIRTREE_VERSION_PATCH "3")
SET(FIRTREE_VERSION "${FIRTREE_VERSION_MAJOR}.${FIRTREE_VERSION_MINOR}.${FIRTREE_VERSION_PATCH}")

# Update the module path to include any extra CMAKE modiles we might ship.
SET(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")

# On x86_64 we need to compile with -fPIC
IF(UNIX AND NOT WIN32)
  FIND_PROGRAM(CMAKE_UNAME uname /bin /usr/bin /usr/local/bin )
  IF(CMAKE_UNAME)
    EXEC_PROGRAM(uname ARGS -m OUTPUT_VARIABLE CMAKE_SYSTEM_PROCESSOR)
    SET(CMAKE_SYSTEM_PROCESSOR ${CMAKE_SYSTEM_PROCESSOR} 
    	CACHE INTERNAL "processor type (i386 and x86_64)")
    IF(CMAKE_SYSTEM_PROCESSOR MATCHES "x86_64")
      ADD_DEFINITIONS(-fPIC)
    ENDIF(CMAKE_SYSTEM_PROCESSOR MATCHES "x86_64")
  ENDIF(CMAKE_UNAME)
ENDIF(UNIX AND NOT WIN32)

# Setup include paths
INCLUDE_DIRECTORIES("${CMAKE_CURRENT_SOURCE_DIR}/firtree/include")

# The doxygen documentation
ADD_SUBDIRECTORY(doc)

# The kernel language frontend
ADD_SUBDIRECTORY(glsl-frontend)

# The kernel language compiler
ADD_SUBDIRECTORY(compiler)

# The FIRTREE library itself.
ADD_SUBDIRECTORY(firtree)

# The various language bindings
ADD_SUBDIRECTORY(bindings)

# Some tools
ADD_SUBDIRECTORY(tools)

# Some exampler
IF(NOT FIRTREE_NO_BUILD_EXAMPLES)
  ADD_SUBDIRECTORY(examples)
ENDIF(NOT FIRTREE_NO_BUILD_EXAMPLES)

