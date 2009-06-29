/* firtree-engine-intl.h */

/* Firtree - A generic image processing library
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

#ifndef _FIRTREE_COGL_TEXTURE_SAMPLER_INTL
#define _FIRTREE_COGL_TEXTURE_SAMPLER_INTL

#include <glib-object.h>
#include <firtree/firtree.h>

#if FIRTREE_HAVE_CLUTTER

#include <firtree/firtree-cogl-texture-sampler.h>
#include "firtree-engine-intl.hh"

G_BEGIN_DECLS

/**
 * firtree_cogl_texture_sampler_get_data:
 *
 * Returns: The size of the cached data.
 */
guint
firtree_cogl_texture_sampler_get_data(FirtreeCoglTextureSampler* self,
        guchar** data, guint* rowstride,
        FirtreeBufferFormat* format);

G_END_DECLS

#endif /* FIRTREE_HAVE_CLUTTER */

#endif /* _FIRTREE_COGL_TEXTURE_SAMPLER_INTL */

/* vim:sw=4:ts=4:et:cindent
 */
