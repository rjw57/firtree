# - Use Styx

# WARNING! Due to the nature of the styx ctoh program it will DELETE
# all files of the form *.h in the source directory. Ideally all of your 
# styx sources should be in their own directory to guard against this.

MACRO(FIND_STYX)
  FIND_PACKAGE(Styx)

  IF(NOT STYX_FOUND)
    MESSAGE(FATAL_ERROR "Styx (http://speculate.de/) is required.")
  ENDIF(NOT STYX_FOUND)
ENDMACRO(FIND_STYX)

MACRO(ADD_STYX_GRAMMARS)
  FIND_STYX()

  FOREACH(_grammar_FILE ${ARGV})
    MESSAGE(STATUS "Generating grammar from Styx source ${_grammar_FILE}.")

    GET_FILENAME_COMPONENT(_absolute ${_grammar_FILE} ABSOLUTE)
    GET_FILENAME_COMPONENT(_path ${_absolute} PATH)
    GET_FILENAME_COMPONENT(_base ${_absolute} NAME_WE)

    ADD_CUSTOM_COMMAND(
      OUTPUT
        "${_path}/${_base}_int.c" "${_path}/${_base}_pim.c" 
        "${_path}/${_base}_lim.c" "${_path}/${_base}.abs"
      COMMAND
        ${STYX_EXECUTABLE}
      ARGS
        -makeC "${_base}"
      WORKING_DIRECTORY
        "${_path}"
      DEPENDS
        "${_grammar_FILE}"
    )

    ADD_CUSTOM_COMMAND(
      OUTPUT
        "${_path}/${_base}_int.h" "${_path}/${_base}_pim.h" 
        "${_path}/${_base}_lim.h" "${_path}/ctoh.cth"
      COMMAND
        ${STYX_CTOH_EXECUTABLE}
      ARGS
        "-CPATH=${_path}"
        "-HPATH=${_path}"
        "-PRJ=${_path}"
      WORKING_DIRECTORY
        "${_path}"
      DEPENDS
        "${_path}/${_base}_int.c" "${_path}/${_base}_pim.c" 
        "${_path}/${_base}_lim.c"
    )
  ENDFOREACH(_grammar_FILE)
ENDMACRO(ADD_STYX_GRAMMARS)

# vim:sw=2:ts=2:et
