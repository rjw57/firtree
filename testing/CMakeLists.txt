# CMake project file for firtree
#
# Copyright (C) 2007, 2008 Rich Wareham <richwareham@gmail.com>
#
# See LICENSE file for distribution rights.

# This file will add a target to build and run the Firtree tests.

FIND_PACKAGE(PythonInterp)

IF(PYTHONINTERP_FOUND)
   ADD_CUSTOM_TARGET(test "${PYTHON_EXECUTABLE}"
       "${CMAKE_CURRENT_SOURCE_DIR}/test_harness.py")
ENDIF(PYTHONINTERP_FOUND)

