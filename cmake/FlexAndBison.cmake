# From http://www.cmake.org/pipermail/cmake/2006-August/010761.html

# search flex
MACRO(FIND_FLEX)
    IF(NOT FLEX_EXECUTABLE)
        FIND_PROGRAM(FLEX_EXECUTABLE flex)
        IF (NOT FLEX_EXECUTABLE)
          MESSAGE(FATAL_ERROR "flex not found - aborting")
        ENDIF (NOT FLEX_EXECUTABLE)
    ENDIF(NOT FLEX_EXECUTABLE)
ENDMACRO(FIND_FLEX)

MACRO(ADD_FLEX_FILES _sources )
    FIND_FLEX()

    FOREACH (_current_FILE ${ARGN})
      GET_FILENAME_COMPONENT(_in ${_current_FILE} ABSOLUTE)
      GET_FILENAME_COMPONENT(_basename ${_current_FILE} NAME_WE)

      SET(_out ${CMAKE_CURRENT_BINARY_DIR}/flex_${_basename}.cpp)

      ADD_CUSTOM_COMMAND(
         OUTPUT ${_out}
         COMMAND ${FLEX_EXECUTABLE}
         ARGS
         -o${_out}
         ${_in}
         DEPENDS ${_in}
      )

      SET(${_sources} ${${_sources}} ${_out} )
   ENDFOREACH (_current_FILE)
ENDMACRO(ADD_FLEX_FILES)


# search bison
MACRO(FIND_BISON)
    IF(NOT BISON_EXECUTABLE)
        FIND_PROGRAM(BISON_EXECUTABLE bison)
        IF (NOT BISON_EXECUTABLE)
          MESSAGE(FATAL_ERROR "bison not found - aborting")
        ENDIF (NOT BISON_EXECUTABLE)
    ENDIF(NOT BISON_EXECUTABLE)
ENDMACRO(FIND_BISON)

MACRO(ADD_BISON_FILES _sources )
    FIND_BISON()

    FOREACH (_current_FILE ${ARGN})
      GET_FILENAME_COMPONENT(_in ${_current_FILE} ABSOLUTE)
      GET_FILENAME_COMPONENT(_basename ${_current_FILE} NAME_WE)

      SET(_out ${CMAKE_CURRENT_BINARY_DIR}/bison_${_basename}.tab.cpp)
      SET(_outprefix ${CMAKE_CURRENT_BINARY_DIR}/bison_${_basename})

      ADD_CUSTOM_COMMAND(
         OUTPUT ${_out}
         COMMAND ${BISON_EXECUTABLE}
         ARGS
	 -t
	 -v
	 -d
         -b${_outprefix}
         ${_in}
         DEPENDS ${_in}
      )

      SET(${_sources} ${${_sources}} ${_out} )
   ENDFOREACH (_current_FILE)
ENDMACRO(ADD_BISON_FILES)

