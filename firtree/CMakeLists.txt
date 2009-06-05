# CMake project file for firtree
#
# Copyright (C) 2009 Rich Wareham <richwareham@gmail.com>
#
# See LICENSE file for distribution rights.

add_library(firtree SHARED
    firtree-affine-transform.c  firtree-affine-transform.h
    firtree-kernel.cc           firtree-kernel.h
    firtree-kernel-sampler.cc   firtree-kernel-sampler.h
    firtree-sampler.cc          firtree-sampler.h
    firtree-vector.h            firtree-vector.c

    internal/firtree-kernel-intl.hh
    internal/firtree-sampler-intl.hh
    
    firtree.h)

# set the library version.
set_target_properties(firtree 
    PROPERTIES
    VERSION 
        "${FIRTREE_VERSION_MAJOR}.${FIRTREE_VERSION_MINOR}.${FIRTREE_VERSION_PATCH}"
    SOVERSION 
        "${FIRTREE_VERSION_MAJOR}.${FIRTREE_VERSION_MINOR}.${FIRTREE_VERSION_PATCH}"
    )

target_link_libraries(firtree
    common llvm-frontend
    ${GLIB_LIBRARIES})

# vim:sw=4:ts=4:autoindent:et

