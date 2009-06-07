# CMake project file for firtree
#
# Copyright (C) 2009 Rich Wareham <richwareham@gmail.com>
#
# See LICENSE file for distribution rights.

find_package(LLVM)
find_package(PythonInterp)

include_directories(${LLVM_INCLUDE_DIRS})

find_program(LLVM_GCC_EXECUTABLE llvm-gcc)

set(_add_custom 1)
if(NOT LLVM_GCC_EXECUTABLE)
    message(STATUS "llvm-gcc not found, some changes to source wont be noticed.")
    set(_add_custom 0)
endif(NOT LLVM_GCC_EXECUTABLE)

if(NOT PYTHONINTERP_FOUND)
    message(STATUS "Python interpreter not found, some changes to source wont be noticed.")
    set(_add_custom 0)
endif(NOT PYTHONINTERP_FOUND)

if(_add_custom)
    add_custom_command(
        OUTPUT 
            llvm-cpu-support.bc.h
            llvm-cpu-support.opt.bc
            llvm-cpu-support.bc
            render-buffer.o
            builtins.bc
        COMMAND ${LLVM_AS_EXECUTABLE}
            -f builtins.ll -o builtins.bc
        COMMAND ${LLVM_GCC_EXECUTABLE} 
            -fno-math-errno
            -o render-buffer.o
            -c -emit-llvm
            render-buffer.c
        COMMAND ${LLVM_LINK_EXECUTABLE}
            -f builtins.bc render-buffer.o -o llvm-cpu-support.bc
        COMMAND ${LLVM_OPT_EXECUTABLE} 
            -f -std-compile-opts -o llvm-cpu-support.opt.bc llvm-cpu-support.bc
        COMMAND ${PYTHON_EXECUTABLE} ../../../tools/bin2h.py
            llvm-cpu-support.opt.bc llvm-cpu-support.bc.h
        DEPENDS
            builtins.ll
            render-buffer.c)
endif(_add_custom)

# Build the CPU renderer
add_library(cpu-engine
    llvm-cpu-support.bc.h
    firtree-cpu-engine.h
    firtree-cpu-engine.cc)

set_source_files_properties(llvm-cpu-support.bc.h PROPERTIES GENERATED TRUE)

# vim:sw=4:ts=4:autoindent:et
