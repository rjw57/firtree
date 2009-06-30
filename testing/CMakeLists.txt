# CMake project file for firtree
#
# Copyright (C) 2009 Rich Wareham <richwareham@gmail.com>
#
# See LICENSE file for distribution rights.

find_package(PythonInterp)

if(PYTHONINTERP_FOUND)
	# Add a custom target that runs all the tests
    add_custom_target(test 
	    COMMAND ${PYTHON_EXECUTABLE} -E -tt run_test_suite.py
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})

    # Add a target to run the tests in valgrind
    find_program(VALGRIND_EXECUTABLE valgrind)
    if(VALGRIND_EXECUTABLE)
        add_custom_target(test-mem
            COMMAND ${VALGRIND_EXECUTABLE}
                --tool=memcheck 
                --leak-check=full
                --suppressions=valgrind-python.supp
                --suppressions=valgrind-gtk.supp
                --suppressions=valgrind-misc.supp
                # --gen-suppressions=yes
                ${PYTHON_EXECUTABLE} -E -tt run_test_suite.py --valgrind
            WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
    else(VALGRIND_EXECUTABLE)
        message(STATUS "Could not find valgrind, memory testing disabled.")
    endif(VALGRIND_EXECUTABLE)
else(PYTHONINTERP_FOUND)
    message(STATUS "Python interpreter could not be found, the test suite is unavailable.")
endif(PYTHONINTERP_FOUND)

# vim:sw=4:ts=4:autoindent:et
