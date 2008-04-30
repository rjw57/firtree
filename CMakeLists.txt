# CMake project file for firtree
#
# Copyright (C) 2007, 2008 Rich Wareham <richwareham@gmail.com>
#
# See LICENSE file for distribution rights.

# This project is called FIRTREE and is written in C++
PROJECT(FIRTREE)

SET(FIRTREE_VERSION_MAJOR "0")
SET(FIRTREE_VERSION_MINOR "0")
SET(FIRTREE_VERSION_PATCH "1")
SET(FIRTREE_VERSION "${FIRTREE_VERSION_MAJOR}.${FIRTREE_VERSION_MINOR}.${FIRTREE_VERSION_PATCH}")

# Update the module path to include any extra CMAKE modiles we might ship.
SET(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")

# The kernel language parser
ADD_SUBDIRECTORY(kernel-parser)

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

