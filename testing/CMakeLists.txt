# CMake project file for firtree
#
# Copyright (C) 2009 Rich Wareham <richwareham@gmail.com>
#
# See LICENSE file for distribution rights.

find_package(PythonInterp)

if(PYTHONINTERP_FOUND)
	# Add a custom target that runs all the tests
    add_custom_target(test 
	    COMMAND ${PYTHON_EXECUTABLE} run_test_suite.py
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
else(PYTHONINTERP_FOUND)
    message(STATUS "Python interpreter could not be found, the test suite is unavailable.")
endif(PYTHONINTERP_FOUND)

# vim:sw=4:ts=4:autoindent:et
