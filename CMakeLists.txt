# CMake project file for firtree
#
# Copyright (C) 2007, 2008 Rich Wareham <richwareham@gmail.com>
#
# See LICENSE file for distribution rights.

# This project is called FIRTREE and is written in C++
PROJECT(FIRTREE)

SET(CMAKE_BUILD_TYPE "Debug")

SET(FIRTREE_VERSION_MAJOR "0")
SET(FIRTREE_VERSION_MINOR "0")
SET(FIRTREE_VERSION_PATCH "1")
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

# The kernel language frontend
ADD_SUBDIRECTORY(glsl-frontend)

# The kernel language parser
ADD_SUBDIRECTORY(local-mesa)

# Some tools
ADD_SUBDIRECTORY(tools)

# Ignore some files in svn.
SET(CPACK_SOURCE_IGNORE_FILES 
    CMakeCache.txt
    _CPack_Packages
    "firtree-.*tar.*"
    "~$"
    ".swp$"
    "\\\\.cvsignore$"
    "\\\\.o$"
    "/.svn/"
    "^${PROJECT_SOURCE_DIR}.*/CVS/"
    "^${PROJECT_SOURCE_DIR}/debian/"
    "^${PROJECT_SOURCE_DIR}/old/"
    "^${PROJECT_SOURCE_DIR}.*/CVSROOT/"
    "^${PROJECT_SOURCE_DIR}/admin/"
)

