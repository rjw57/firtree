# CMake project file for firtree
#
# Copyright (C) 2009 Rich Wareham <richwareham@gmail.com>
#
# See LICENSE file for distribution rights.

set(FIRTREE_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../..")
set(FIRTREE_BINARY_DIR "${CMAKE_CURRENT_BINARY_DIR}/../..")
set(FIRTREE_INCLUDE_DIR "${FIRTREE_SOURCE_DIR}/..")

set(LLVM_CONFIG_COMPONENTS core 
    codegen linker bitreader
    jit native)

find_package(LLVM)
find_package(PythonInterp)

find_program(LLVM_GCC_EXECUTABLE llvm-gcc)

if(NOT LLVM_GCC_EXECUTABLE)
    message(SEND_ERROR "llvm-gcc not found.")
endif(NOT LLVM_GCC_EXECUTABLE)

if(NOT PYTHONINTERP_FOUND)
    message(SEND_ERROR "Python interpreter not found.")
endif(NOT PYTHONINTERP_FOUND)

include_directories(${LLVM_INCLUDE_DIRS} ${FIRTREE_INCLUDE_DIR})

add_custom_command(
    OUTPUT 
        llvm-cpu-support.bc.h
        llvm-cpu-support.opt.bc
        llvm-cpu-support.bc
        render-buffer.o
        builtins.bc
    COMMAND ${LLVM_AS_EXECUTABLE}
        -f builtins.ll -o builtins.nonopt.bc
    COMMAND ${LLVM_OPT_EXECUTABLE}
        -O3 builtins.nonopt.bc -f -o=builtins.bc
    COMMAND ${LLVM_GCC_EXECUTABLE} 
        -fno-math-errno
        -o render-buffer.o
        -c -emit-llvm
        ${GLIB_CFLAGS}
        -I "${FIRTREE_INCLUDE_DIR}"
        render-buffer.c
    COMMAND ${LLVM_LINK_EXECUTABLE}
        -f builtins.bc render-buffer.o -o llvm-cpu-support.bc
    COMMAND ${LLVM_OPT_EXECUTABLE} 
        -f -std-compile-opts -o llvm-cpu-support.opt.bc llvm-cpu-support.bc
    COMMAND ${PYTHON_EXECUTABLE} ../../../tools/bin2h.py
        llvm-cpu-support.opt.bc llvm-cpu-support.bc.h
    WORKING_DIRECTORY
        ${CMAKE_CURRENT_SOURCE_DIR}
    DEPENDS
        builtins.ll
        render-buffer.c)

set(_firtree_cpu_public_headers 
    firtree-cpu-renderer.h)
    
add_custom_command(OUTPUT firtree-type-builtins.h ${CMAKE_CURRENT_BINARY_DIR}/firtree-type-builtins.c
	COMMAND ${GLIB_MKENUMS} 
		--template firtree-type-builtins.h.template 
		${_firtree_public_headers} > firtree-type-builtins.h
	COMMAND ${GLIB_MKENUMS} 
		--template firtree-type-builtins.c.template
		${_firtree_public_headers} > ${CMAKE_CURRENT_BINARY_DIR}/firtree-type-builtins.c
    WORKING_DIRECTORY
        ${CMAKE_CURRENT_SOURCE_DIR}
	DEPENDS 
		${_firtree_public_headers}
		firtree-type-builtins.h.template 
		firtree-type-builtins.c.template
	VERBATIM)

include_directories("${CMAKE_CURRENT_SOURCE_DIR}")

add_library(firtree-engine-cpu SHARED
    ${_firtree_cpu_public_headers}
    
    # CPU rendering library
    llvm-cpu-support.bc.h

    firtree-cpu-renderer.cc   
    firtree-cpu-jit.cc      firtree-cpu-jit.hh
    )

set_source_files_properties(
    engines/cpu/llvm-cpu-support.bc.h
    engines/cpu/firtree-cpu-renderer.h
    PROPERTIES GENERATED TRUE)

# set the library version.
set_target_properties(firtree-engine-cpu
    PROPERTIES
    VERSION 
        "${FIRTREE_VERSION_MAJOR}.${FIRTREE_VERSION_MINOR}.${FIRTREE_VERSION_PATCH}"
    SOVERSION 
        "${FIRTREE_VERSION_MAJOR}"
    OUTPUT_NAME
        "firtree-engine-cpu-${FIRTREE_VERSION_MAJOR}.${FIRTREE_VERSION_MINOR}"
    )

target_link_libraries(firtree-engine-cpu 
    firtree
    common llvm-frontend 
    ${GLIB_LIBRARIES}
    ${ENGINE_EXTRA_LIBS})

# The magic runes required to install a library.
install(TARGETS firtree-engine-cpu
  RUNTIME DESTINATION bin
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib
)

# Install the public headers
install(FILES ${_firtree_cpu_public_headers} DESTINATION "${FIRTREE_HEADER_INSTALL_DEST}/firtree/engines/cpu")

# vim:sw=4:ts=4:autoindent:et

