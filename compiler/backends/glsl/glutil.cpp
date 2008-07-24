// FIRTREE - A generic image processing library
// Copyright (C) 2007, 2008 Rich Wareham <richwareham@gmail.com>
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License verstion as published
// by the Free Software Foundation.
//
// This program is distributed in the hope that it will be useful, but WITHOUT
// ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
// FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
// more details.
//
// You should have received a copy of the GNU General Public License along with
// this program; if not, write to the Free Software Foundation, Inc., 51
// Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA

//=============================================================================
// This file implements the FIRTREE GLSL backend utility functions.
//=============================================================================

#include "glutil.h"

#include <firtree/opengl.h>

namespace Firtree { namespace GLSLInternal {
    
// ============================================================================
void* GetProcAddress(const char *procName)
{
#if defined(FIRTREE_UNIX) && !defined(FIRTREE_APPLE)
    return (void*)(glXGetProcAddress((const GLubyte*)procName));
#else
#   warning Compiling a stub version of GetProcAddress.
    return NULL;
#endif
}

//=============================================================================
} } // namespace Firtree::GLSLInternal


//=============================================================================
// vim:sw=4:ts=4:cindent:et
