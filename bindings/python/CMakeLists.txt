# CMake project file for FIRTREE
#
# Copyright (C) 2007, 2008 Rich Wareham <richwareham@gmail.com>
#
# See LICENSE file for distribution rights.

# See if we can locate the SWIG tool.
FIND_PACKAGE(SWIG)
FIND_PACKAGE(PythonInterp)
FIND_PACKAGE(PythonLibs)
IF(SWIG_FOUND)
    INCLUDE(UseSWIG)
ENDIF(SWIG_FOUND)

# See if python was found
SET(PYTHON_FOUND 0)
IF(PYTHON_LIBRARIES)
    SET(PYTHON_FOUND 1)
ENDIF(PYTHON_LIBRARIES)

# Setup SWIG options in general.
IF(SWIG_FOUND AND PYTHON_FOUND AND PYTHONINTERP_FOUND)
    EXECUTE_PROCESS(
      COMMAND
      ${PYTHON_EXECUTABLE} -c 
      "from distutils import sysconfig; print sysconfig.get_python_lib(1,0,prefix='${CMAKE_INSTALL_EXEC_PREFIX}')"
      OUTPUT_VARIABLE PYTHON_INSTDIR
      OUTPUT_STRIP_TRAILING_WHITESPACE
    )

    SET(CMAKE_SWIG_OUTDIR ${CMAKE_CURRENT_SOURCE_DIR})
    MESSAGE(STATUS "=> Generating Python wrappers.")

    INCLUDE_DIRECTORIES(${PYTHON_INCLUDE_PATH})

    CONFIGURE_FILE(
        "${CMAKE_CURRENT_SOURCE_DIR}/../firtree.i"
    	"${CMAKE_CURRENT_BINARY_DIR}/firtree-python.i"
        @ONLY)
    SET_SOURCE_FILES_PROPERTIES(firtree-python.i PROPERTIES GENERATED 1)
    SET_SOURCE_FILES_PROPERTIES(firtree-python.i PROPERTIES CPLUSPLUS 1)
    SWIG_ADD_MODULE(Firtree python firtree-python.i)
    SWIG_LINK_LIBRARIES(Firtree firtree ${FIRTREE_LIBS} ${PYTHON_LIBRARIES})

    # On Windows, the generated DLL has to be named _Firtree.pyd.
    IF(WIN32)
        ADD_CUSTOM_COMMAND(TARGET _Firtree
        COMMAND ${CMAKE_COMMAND}
            ARGS -E copy _Firtree.dll _Firtree.pyd)
        ADD_CUSTOM_COMMAND(TARGET _Firtree
        COMMAND ${CMAKE_COMMAND}
            ARGS -E remove _Firtree.dll)
    ENDIF(WIN32)

    INSTALL(TARGETS _Firtree DESTINATION ${PYTHON_INSTDIR})
    INSTALL(FILES Firtree.py DESTINATION ${PYTHON_INSTDIR})
ELSE(SWIG_FOUND AND PYTHON_FOUND)
    MESSAGE(STATUS 
    "=> Not generating Python wrappers.")
    IF(NOT SWIG_FOUND)
        MESSAGE(STATUS "   `-> SWIG was not found.")
    ENDIF(NOT SWIG_FOUND)

    IF(NOT PYTHON_FOUND)
        MESSAGE(STATUS 
        "   `-> The Python development libraries were not found.")
    ENDIF(NOT PYTHON_FOUND)
    
    IF(NOT PYTHONINTERP_FOUND)
        MESSAGE(STATUS 
        "   `-> The Python interpreter was not found.")
    ENDIF(NOT PYTHONINTERP_FOUND)
ENDIF(SWIG_FOUND AND PYTHON_FOUND AND PYTHONINTERP_FOUND)

# Vim modeline
# vim:ts=4:sw=4:et
