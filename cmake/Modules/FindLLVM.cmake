# - Find LLVM
# Set the following variables on entry
#
#  LLVM_CONFIG_COMPONENTS (optional) - list of components to use.
#
# This module finds an installed LLVM. It sets the following variables:
#  LLVM_FOUND - set to true if LLVM is found
#  LLVM_VERSION - set to the LLVM version.
#  LLVM_MAJOR - set to the LLVM major version number.
#  LLVM_MINOR - set to the LLVM minor version number.
#  LLVM_CONFIG_EXECUTABLE - the path to the llvm-config executable
#  LLVM_AS_EXECUTABLE - the path to the llvm-as executable
#  LLVM_OPT_EXECUTABLE - the path to the llvm opt executable
#  LLVM_LINK_EXECUTABLE - the path to the llvm opt executable
#  LLVM_HOST_TARGET - Target triple used to configure LLVM.
#  LLVM_INCLUDE_DIR - where to find the LLVM headers.
#  LLVM_LIBRARY_DIR - the LLVM library directory
#  LLVM_LIBRARIES - the LLVM libraries to link against.
#

FIND_PROGRAM(LLVM_CONFIG_EXECUTABLE llvm-config)
FIND_PROGRAM(LLVM_AS_EXECUTABLE llvm-as)
FIND_PROGRAM(LLVM_OPT_EXECUTABLE opt)
FIND_PROGRAM(LLVM_LINK_EXECUTABLE llvm-link)

MACRO(LLVM_RUN_CONFIG arg outvar)
  EXECUTE_PROCESS(COMMAND "${LLVM_CONFIG_EXECUTABLE}" ${LLVM_CONFIG_COMPONENTS} "${arg}"
    OUTPUT_VARIABLE ${outvar}
    ERROR_VARIABLE LLVM_llvm_config_error
    RESULT_VARIABLE LLVM_llvm_config_result)
  IF(LLVM_llvm_config_result)
    MESSAGE(SEND_ERROR "Command \"${LLVM_CONFIG_EXECUTABLE} ${arg}\" failed with output:\n${LLVM_llvm_config_error}")
  ENDIF(LLVM_llvm_config_result)
ENDMACRO(LLVM_RUN_CONFIG)

IF(LLVM_CONFIG_EXECUTABLE)
  LLVM_RUN_CONFIG("--version" LLVM_VERSION)
  LLVM_RUN_CONFIG("--libdir" LLVM_LIBRARY_DIR)
  LLVM_RUN_CONFIG("--includedir" LLVM_INCLUDE_DIR)
  LLVM_RUN_CONFIG("--libfiles" LLVM_LIBRARIES)
  LLVM_RUN_CONFIG("--host-target" LLVM_HOST_TARGET)
  STRING(REGEX REPLACE "[ \n\t]+" "" LLVM_HOST_TARGET ${LLVM_HOST_TARGET})
  STRING(REGEX REPLACE "[ \n\t]+" "" LLVM_VERSION ${LLVM_VERSION})
  STRING(REGEX REPLACE "[ \n\t]+" " " LLVM_LIBRARY_DIR ${LLVM_LIBRARY_DIR})
  STRING(REGEX REPLACE "[ \n\t]+" " " LLVM_INCLUDE_DIR ${LLVM_INCLUDE_DIR})
  STRING(REGEX REPLACE "[ \n\t]+" " " LLVM_LIBRARIES ${LLVM_LIBRARIES})

  STRING(REGEX REPLACE "([0-9]+)\\.([0-9]+)" "\\1" LLVM_MAJOR ${LLVM_VERSION})
  STRING(REGEX REPLACE "([0-9]+)\\.([0-9]+)" "\\2" LLVM_MINOR ${LLVM_VERSION})

  STRING(REGEX REPLACE "[^;\n\t ]+\\.o" "" LLVM_STATIC_LIBS ${LLVM_LIBRARIES})
  STRING(REGEX REPLACE "[^;\n\t ]+\\.a" "" LLVM_STATIC_OBJS ${LLVM_LIBRARIES})

  SEPARATE_ARGUMENTS(LLVM_LIBRARIES)
  SEPARATE_ARGUMENTS(LLVM_STATIC_LIBS)
  SEPARATE_ARGUMENTS(LLVM_STATIC_OBJS)
ENDIF(LLVM_CONFIG_EXECUTABLE)

# Assume LLVM is found unless anything indicates otherwise
SET( LLVM_FOUND "YES" )

IF(NOT LLVM_CONFIG_EXECUTABLE)
  SET( LLVM_FOUND "NO" )
ENDIF(NOT LLVM_CONFIG_EXECUTABLE)

IF(NOT LLVM_LIBRARY_DIR)
  SET( LLVM_FOUND "NO" )
ENDIF(NOT LLVM_LIBRARY_DIR)

IF(NOT LLVM_INCLUDE_DIR)
  SET( LLVM_FOUND "NO" )
ENDIF(NOT LLVM_INCLUDE_DIR)

IF(NOT LLVM_LIBRARIES)
  SET( LLVM_FOUND "NO" )
ENDIF(NOT LLVM_LIBRARIES)

# vim:sw=2:ts=2:et
