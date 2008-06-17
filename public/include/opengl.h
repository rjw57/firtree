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

// ============================================================================
/// \file opengl.h Platform independent wat to include the OpenGL headers
///
/// Not all platforms supported by FIRTREE have their OpenGL headers under
/// GL/. This file includes gl.h, glext.h and glut.h for all supported
/// platforms.
// ============================================================================

// ============================================================================
#ifndef FIRTREE_OPENGL_H
#define FIRTREE_OPENGL_H
// ============================================================================

#include <public/include/platform.h>

#if defined(FIRTREE_UNIX) && !defined(FIRTREE_APPLE)
# if !defined(FIRTREE_NO_GLEW)
#  include <GL/glew.h>
# endif
# include <GL/gl.h>
# include <GL/glu.h>
# if !defined(FIRTREE_NO_GLX)
#  include <GL/glx.h>
#  include <GL/glxext.h>
# endif
# ifdef FIRTREE_HAVE_GLUT
#  define HAVE_FREEGLUT // FIXME: Hell of an assumption!
#  include <GL/glut.h>
#  include <GL/freeglut_ext.h>
# endif
#elif defined(FIRTREE_APPLE)
# if !defined(FIRTREE_NO_GLEW)
#  include <GL/glew.h>
# endif
# include <OpenGL/gl.h>
# include <OpenGL/glext.h>
# include <GLUT/glut.h>
#elif defined(FIRTREE_WIN32)
# include <windows.h>
# include <wingdi.h> // For wgl
# if !defined(FIRTREE_NO_GLEW)
#  include <GL/glew.h>
# endif
# include <GL/gl.h>
# include <GL/glu.h>
# include <GL/glext.h>
# ifdef FIRTREE_HAVE_GLUT
#  include <GL/glut.h>
# endif
#endif

// ============================================================================
#endif // FIRTREE_OPENGL_H
// ============================================================================
