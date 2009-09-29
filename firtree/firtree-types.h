/* firtree-types.h */

/* Firtree - A generic buffer processing library
 * Copyright (C) 2009 Rich Wareham <richwareham@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as published
 * by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.    See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program; if not, write to the Free Software Foundation, Inc., 51
 * Franklin Street, Fifth Floor, Boston, MA    02110-1301, USA
 */

#ifndef _FIRTREE_TYPES
#define _FIRTREE_TYPES

#include <glib-object.h>

/*
 * FirtreeBufferFormat:
 * @FIRTREE_FORMAT_ARGB32:
 * @FIRTREE_FORMAT_ARGB32_PREMULTIPLIED:
 * @FIRTREE_FORMAT_XRGB32:
 * @FIRTREE_FORMAT_RGBA32:
 * @FIRTREE_FORMAT_RGBA32_PREMULTIPLIED:
 * @FIRTREE_FORMAT_ABGR32:
 * @FIRTREE_FORMAT_ABGR32_PREMULTIPLIED:
 * @FIRTREE_FORMAT_XBGR32:
 * @FIRTREE_FORMAT_BGRA32:
 * @FIRTREE_FORMAT_BGRA32_PREMULTIPLIED:
 * @FIRTREE_FORMAT_RGB24:
 * @FIRTREE_FORMAT_BGR24:
 * @FIRTREE_FORMAT_RGBX32:
 * @FIRTREE_FORMAT_BGRX32:
 * @FIRTREE_FORMAT_L8:
 * @FIRTREE_FORMAT_RGBA_F32: 4 component 32-bit floating point numbers.
 * 
 * A set of possible formats memory buffers can be in. The names are of the
 * form FIRTREE_FORMAT_abcdNN where abcd gives the byte order of the packed 
 * components in memory and NN is the number of bits per pixel.
 *
 * Exceptions to this are the _FOURCC and _F32 formats. The former are laid 
 * out in memory as specified by their four character code and the latter are
 * laid out as arrays of 4-component floating point vectors.
 *
 * Since these are specified in terms of byte order in memory, they are
 * endian independent.
 */
typedef enum {
    FIRTREE_FORMAT_ARGB32                   = 0x00, 
    FIRTREE_FORMAT_ARGB32_PREMULTIPLIED     = 0x01, 
    FIRTREE_FORMAT_XRGB32                   = 0x02, 
    FIRTREE_FORMAT_RGBA32                   = 0x03, 
    FIRTREE_FORMAT_RGBA32_PREMULTIPLIED     = 0x04, 

    FIRTREE_FORMAT_ABGR32                   = 0x05, 
    FIRTREE_FORMAT_ABGR32_PREMULTIPLIED     = 0x06, 
    FIRTREE_FORMAT_XBGR32                   = 0x07, 
    FIRTREE_FORMAT_BGRA32                   = 0x08, 
    FIRTREE_FORMAT_BGRA32_PREMULTIPLIED     = 0x09, 

    FIRTREE_FORMAT_RGB24                    = 0x0a,
    FIRTREE_FORMAT_BGR24                    = 0x0b,

    FIRTREE_FORMAT_RGBX32                   = 0x0c, 
    FIRTREE_FORMAT_BGRX32                   = 0x0d, 

    FIRTREE_FORMAT_L8                       = 0x0e, 
    FIRTREE_FORMAT_I420_FOURCC              = 0x0f, 
    FIRTREE_FORMAT_YV12_FOURCC              = 0x10, 

    FIRTREE_FORMAT_RGBA_F32                 = 0x11, 

    FIRTREE_FORMAT_LAST
} FirtreeBufferFormat;

#endif /* _FIRTREE_TYPES */

/* vim:sw=4:ts=4:et:cindent
 */
