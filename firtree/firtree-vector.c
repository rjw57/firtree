/* firtree-vector.c */

/* Firtree - A generic image processing library
 * Copyright (C) 2009 Rich Wareham <richwareham@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License verstion as published
 * by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program; if not, write to the Free Software Foundation, Inc., 51
 * Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#include "firtree-vector.h"

#include <string.h>

static GType _firtree_vec2_type = 0;
static GType _firtree_vec3_type = 0;
static GType _firtree_vec4_type = 0;

gpointer 
_firtree_vec2_copy(gpointer ptr) 
{
    FirtreeVec2* new_vec2 = g_slice_new(FirtreeVec2);
    memcpy(new_vec2, ptr, sizeof(FirtreeVec2));
    return new_vec2;
}

void
_firtree_vec2_free(gpointer ptr) 
{
    g_slice_free(FirtreeVec2, ptr);
}

GType
firtree_vec2_get_type()
{
    if(0==_firtree_vec2_type) {
        _firtree_vec2_type = g_boxed_type_register_static(
                "firtree_vec2", _firtree_vec2_copy, _firtree_vec2_free);
    }
    return _firtree_vec2_type;
}

gpointer 
_firtree_vec3_copy(gpointer ptr) 
{
    FirtreeVec3* new_vec3 = g_slice_new(FirtreeVec3);
    memcpy(new_vec3, ptr, sizeof(FirtreeVec3));
    return new_vec3;
}

void
_firtree_vec3_free(gpointer ptr) 
{
    g_slice_free(FirtreeVec3, ptr);
}

GType
firtree_vec3_get_type()
{
    if(0==_firtree_vec3_type) {
        _firtree_vec3_type = g_boxed_type_register_static(
                "firtree_vec3", _firtree_vec3_copy, _firtree_vec3_free);
    }
    return _firtree_vec3_type;
}

gpointer 
_firtree_vec4_copy(gpointer ptr) 
{
    FirtreeVec4* new_vec4 = g_slice_new(FirtreeVec4);
    memcpy(new_vec4, ptr, sizeof(FirtreeVec4));
    return new_vec4;
}

void
_firtree_vec4_free(gpointer ptr) 
{
    g_slice_free(FirtreeVec4, ptr);
}

GType
firtree_vec4_get_type()
{
    if(0==_firtree_vec4_type) {
        _firtree_vec4_type = g_boxed_type_register_static(
                "firtree_vec4", _firtree_vec4_copy, _firtree_vec4_free);
    }
    return _firtree_vec4_type;
}

/* vim:sw=4:ts=4:et:cindent
 */
