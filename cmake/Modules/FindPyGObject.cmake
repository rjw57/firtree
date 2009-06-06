# CMake macros to find the Python GObject bindings.

# Output variables:
#
#   PYGOBJECT_FOUND          ... set to 1 if PyGObject was foung
#
# If PYGOBJECT_FOUND == 1:
#
#   PYGOBJECT_CODEGEN_DIR    ... the directory holding the codegen files.
#   PYGOBJECT_CODEGEN        ... the location of the codegen.py utility
#   PYGOBJECT_CREATEDEFS     ... the location of the createdefs.py utility
#   PYGOBJECT_PYGTK_INCLUDE_DIRS   ... the include dirs for pygobject.h etc.
#
#   PYGOBJECT_LIBRARIES      ... only the libraries (w/o the '-l')
#   PYGOBJECT_LIBRARY_DIRS   ... the paths of the libraries (w/o the '-L')
#   PYGOBJECT_LDFLAGS        ... all required linker flags
#   PYGOBJECT_LDFLAGS_OTHER  ... all other linker flags
#   PYGOBJECT_INCLUDE_DIRS   ... the '-I' preprocessor flags (w/o the '-I')
#   PYGOBJECT_CFLAGS         ... all required cflags
#   PYGOBJECT_CFLAGS_OTHER   ... the other compiler flags

include(StandardOptionParsing)

# We use pkg-config to find the bindings.
find_package(PkgConfig)

SET(_pygobject_module pygobject-2.0)

# Look for pygobject
pkg_search_module(PYGOBJECT ${_pygobject_module})

# Check we have the python interpreter
find_package(PythonInterp)

if(NOT PYTHONINTERP_FOUND)
    message(STATUS "The Python interpreter could not be found. Cannot use PyGObject.")
    set(PYGOBJECT_FOUND 0)
endif(NOT PYTHONINTERP_FOUND)

# Check we have the python libraries
find_package(PythonLibs)

if(NOT PYTHONLIBS_FOUND)
    message(STATUS "The Python libraries could not be found. Cannot use PyGObject.")
    set(PYGOBJECT_FOUND 0)
endif(NOT PYTHONLIBS_FOUND)

# macro to get a pkg-config variable for the pygobject module
macro(_pygobject_pkgconfig_var _varname _outvar _failed_flag)
    execute_process(
        COMMAND "${PKG_CONFIG_EXECUTABLE}" ${_pygobject_module} "--variable=${_varname}"
        OUTPUT_VARIABLE _tmp_output
        RESULT_VARIABLE _tmp_failed)

    if(_tmp_failed)
        set(${_outvar} NOTFOUND)
    else(_tmp_failed)
        # strip trailing newlines
        string(REGEX REPLACE "[\r\n]" "" _tmp_output "${_tmp_output}")
        # assign output variable
        set(${_outvar} "${_tmp_output}")
    endif(_tmp_failed)

    set(${_failed_flag} "${_tmp_failed}")
endmacro(_pygobject_pkgconfig_var)

if(PYGOBJECT_FOUND)
    # Now try to find the codegen directory
    _pygobject_pkgconfig_var(codegendir PYGOBJECT_CODEGEN_DIR _fail_flag)

    if(_fail_flag)
        # If we couldn't find the codegen dir, set the fail flag.
        SET(PYGOBJECT_FOUND 0)
    endif(_fail_flag)

    _pygobject_pkgconfig_var(pygtkincludedir PYGOBJECT_PYGTK_INCLUDE_DIRS _fail_flag)

    if(_fail_flag)
        # If we couldn't find the pygtkincludedir dir, set the fail flag.
        SET(PYGOBJECT_FOUND 0)
    endif(_fail_flag)

    # Set the location for the codegen program (FIXME: do we want to signal non-presence?)
    find_file(PYGOBJECT_CODEGEN NAMES codegen.py 
        PATHS "${PYGOBJECT_CODEGEN_DIR}" NO_DEFAULT_PATH)

    # Set the location for the createdefs program (FIXME: do we want to signal non-presence?)
    find_file(PYGOBJECT_CREATEDEFS NAMES createdefs.py 
        PATHS "${PYGOBJECT_CODEGEN_DIR}" NO_DEFAULT_PATH)
endif(PYGOBJECT_FOUND)

### User visible macros

# pygobject_target_add_bindings(var_prefix file_prefix
#                               [MODULEPREFIX mod_prefix]
#                               DEFS defs1... [OVERRIDE override])
# 
# Add rules for generating bindinds from the .defs files 'defs1...'. Write a list of source files to
# <var_prefix>_SOURCES and any required python libraries for bindings to 
# <var_prefix>_LIBRARIES. <var_prefix>_INCLUDE_DIRS and <var_prefix>_LINK_DIRS
# are similarly set.
#
# file_prefix is a prefix used for all generated files. mod_prefix is the prefix used
# for generated modules. If omitted, py${file_prefix} is used.
#
# You should add the required libraries corresponding to each header manually via
# target_link_libraries().
macro(pygobject_target_add_bindings _prefix _file_prefix)
    set(_list_names "MODULEPREFIX" "DEFS" "OVERRIDE" "REGISTER")
    set(_list_variables "_module_prefix" "_defs" "_overrides" "_register")
    parse_options(_list_names _list_variables ${ARGN})

    list(LENGTH _module_prefix _module_prefix_length)
    list(LENGTH _overrides _overrides_length)
    list(LENGTH _defs _defs_length)

    # _module_prefix defaults to _file_prefix
    if(_module_prefix_length EQUAL 0)
        set(_module_prefix "py${_file_prefix}")
    endif(_module_prefix_length EQUAL 0)

    set(_opts_valid 1)
    if(NOT _defs_length GREATER 0)
        message(SEND_ERROR "Must have at least one .defs file specified.")
        set(_opts_valid 0)
    endif(NOT _defs_length GREATER 0)

    if(_overrides_length GREATER 1)
        message(SEND_ERROR "Can only specify one override file")
        set(_opts_valid 0)
    endif(_overrides_length GREATER 1)

    set(_output_defs "${CMAKE_CURRENT_BINARY_DIR}/${_file_prefix}.generated.defs")
    set(_output_source "${CMAKE_CURRENT_BINARY_DIR}/${_file_prefix}.generated.c")

    set(${_prefix}_SOURCES "")
    set(${_prefix}_LIBRARIES "${PYTHON_LIBRARIES}")
    set(${_prefix}_INCLUDE_DIRS "${PYTHON_INCLUDE_PATH}")
    set(${_prefix}_LINK_DIRS "")

    if(_opts_valid)
        # Construct the options for codegen
        set(_codegen_opts "--prefix" "${_module_prefix}")
        if(_overrides)
            list(APPEND _codegen_opts "--override" ${_overrides})
        endif(_overrides)

        foreach(_reg ${_register})
            list(APPEND _codegen_opts "--register" ${_reg})
        endforeach(_reg in _register)

        # Generate a target to output overall defs
        add_custom_command(OUTPUT "${_output_defs}"
            COMMAND "${PYTHON_EXECUTABLE}" 
                "${PYGOBJECT_CREATEDEFS}" "${_output_defs}" ${_defs}
            DEPENDS ${_defs}
            VERBATIM)

        # Generate a target to output code
        add_custom_command(OUTPUT "${_output_source}"
            COMMAND "${PYTHON_EXECUTABLE}" 
                "${PYGOBJECT_CODEGEN}" ${_codegen_opts} "${_output_defs}" > "${_output_source}"
            DEPENDS "${_output_defs}" ${_overrides}
            VERBATIM)

        set(${_prefix}_SOURCES "${_output_source}")
        set_source_files_properties(${_prefix}_SOURCES PROPERTIES GENERATED TRUE)
    endif(_opts_valid)
endmacro(pygobject_target_add_bindings)

# vim:sw=4:ts=4:et:autoindent

