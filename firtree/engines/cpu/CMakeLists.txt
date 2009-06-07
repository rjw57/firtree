# CMake project file for firtree
#
# Copyright (C) 2009 Rich Wareham <richwareham@gmail.com>
#
# See LICENSE file for distribution rights.

find_package(LLVM)
find_package(PythonInterp)

include_directories(${LLVM_INCLUDE_DIRS})

find_program(LLVM_GCC llvm-gcc)

set(_add_custom 1)
if(NOT LLVM_GCC)
    message(STATUS "llvm-gcc not found, some changes to source wont be noticed.")
    set(_add_custom 0)
endif(NOT LLVM_GCC)

if(NOT PYTHONINTERP_FOUND)
    message(STATUS "Python interpreter not found, some changes to source wont be noticed.")
    set(_add_custom 0)
endif(NOT PYTHONINTERP_FOUND)

if(_add_custom)
    add_custom_command(
        OUTPUT 
            render-buffer.llvm.bc render-buffer.llvm.bc.h
        COMMAND ${LLVM_GCC} -O9
            render-buffer.c -c -emit-llvm -o render-buffer.llvm.bc
        COMMAND ${PYTHON_EXECUTABLE} ../../../tools/bin2h.py 
            render-buffer.llvm.bc render-buffer.llvm.bc.h
        DEPENDS
            render-buffer.c)
    set_source_files_properties(render-buffer.llvm.bc.h
        PROPERTIES GENERATED TRUE)
endif(_add_custom)

# Build the CPU renderer
add_library(cpu-engine
    render-buffer.llvm.bc.h
    firtree-cpu-engine.h
    firtree-cpu-engine.cc)

# vim:sw=4:ts=4:autoindent:et
