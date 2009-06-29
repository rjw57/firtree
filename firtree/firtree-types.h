/* firtree-types.h */

/* Firtree - A generic buffer processing library
 * Copyright (C) 2009 Rich Wareham <richwareham@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License verstion as published
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
 * @FIRTREE_FORMAT_ARGB32: Packed buffer of 32-bit native endian integers of
 * the form 0xAARRGGBB.
 * @FIRTREE_FORMAT_ARGB32_PREMULTIPLIED: Packed buffer of 32-bit native endian 
 * integers of the form 0xAARRGGBB where the alpha has been pre-multiplied 
 * into R, G and B.
 * @FIRTREE_FORMAT_XRGB32: Packed buffer of 32-bit native endian integers of
 * the form 0x??RRGGBB where the upper 8 bits of alpha is ignored and alpha
 * is implcitly 1.0.
 * @FIRTREE_FORMAT_ABGR32: Packed buffer of 32-bit native endian integers of
 * the form 0xAABBGGRR.
 * @FIRTREE_FORMAT_ABGR32_PREMULTIPLIED: Packed buffer of 32-bit native endian 
 * integers of the form 0xAABBGGRR where the alpha has been pre-multiplied 
 * into R, G and B.
 * @FIRTREE_FORMAT_XBGR32: Packed buffer of 32-bit native endian integers of
 * the form 0x??BBGGRR where the upper 8 bits of alpha is ignored and alpha
 * is implcitly 1.0.
 * @FIRTREE_FORMAT_RGB24: Packed buffer of 24-bit native endian integers of
 * the form 0xRRGGBB. Alpha is implicitly 1.0.
 * @FIRTREE_FORMAT_RGB24: Packed buffer of 24-bit native endian integers of
 * the form 0xBBGGRR. Alpha is implicitly 1.0.
 * 
 * A set of possible formats memory buffers can be in.
 */
typedef enum {
    FIRTREE_FORMAT_ARGB32                   = 0x00, 
    FIRTREE_FORMAT_ARGB32_PREMULTIPLIED     = 0x01, 
    FIRTREE_FORMAT_XRGB32                   = 0x02, 
    FIRTREE_FORMAT_ABGR32                   = 0x03, 
    FIRTREE_FORMAT_ABGR32_PREMULTIPLIED     = 0x04, 
    FIRTREE_FORMAT_XBGR32                   = 0x05, 
    FIRTREE_FORMAT_RGB24                    = 0x06,
    FIRTREE_FORMAT_BGR24                    = 0x07,

    FIRTREE_FORMAT_LAST
} FirtreeBufferFormat;

#endif /* _FIRTREE_TYPES */

/* vim:sw=4:ts=4:et:cindent
 */
