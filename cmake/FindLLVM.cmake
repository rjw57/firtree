# - Find LLVM
# This module finds an installed LLVM. It sets the following variables:
#  LLVM_FOUND - set to true if LLVM is found
#  LLVM_CONFIG_EXECUTABLE - the path to the llvm-config executable
#  LLVM_INCLUDE_DIR - where to find the LLVM headers.
#  LLVM_LIBRARY_DIR - the LLVM library directory
#  LLVM_LIBRARIES - the LLVM libraries to link against.
#

FIND_PROGRAM(LLVM_CONFIG_EXECUTABLE llvm-config)

MACRO(LLVM_RUN_CONFIG arg outvar)
  EXECUTE_PROCESS(COMMAND "${LLVM_CONFIG_EXECUTABLE}" "${arg}"
    OUTPUT_VARIABLE ${outvar}
    ERROR_VARIABLE LLVM_llvm_config_error
    RESULT_VARIABLE LLVM_llvm_config_result)
  IF(LLVM_llvm_config_result)
    MESSAGE(SEND_ERROR "Command \"${LLVM_CONFIG_EXECUTABLE} ${arg}\" failed with output:\n${LLVM_llvm_config_error}")
  ENDIF(LLVM_llvm_config_result)
ENDMACRO(LLVM_RUN_CONFIG)

IF(LLVM_CONFIG_EXECUTABLE)
  LLVM_RUN_CONFIG("--libdir" LLVM_LIBRARY_DIR)
  LLVM_RUN_CONFIG("--includedir" LLVM_INCLUDE_DIR)
  LLVM_RUN_CONFIG("--libs" LLVM_LIBRARIES)
  STRING(REGEX REPLACE "[ \n\t]+" " " LLVM_LIBRARIES ${LLVM_LIBRARIES})
  SEPARATE_ARGUMENTS(LLVM_LIBRARIES)
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
