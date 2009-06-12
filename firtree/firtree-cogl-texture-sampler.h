/* firtree-cogl-texture-sampler.h */

/* Firtree - A generic cogl_texture processing library
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

#ifndef _FIRTREE_COGL_TEXTURE_SAMPLER
#define _FIRTREE_COGL_TEXTURE_SAMPLER

#include "firtree.h"

#if FIRTREE_HAVE_CLUTTER

#include <glib-object.h>
#include <cogl/cogl.h>
#include <clutter/clutter.h>

#include "firtree-sampler.h"

/**
 * SECTION:firtree-cogl-texture-sampler
 * @short_description: A FirtreeSampler which can sample from a COGL texture.
 * @include: firtree/firtree-cogl-texture-sampler.h
 *
 * A FirtreeCoglTextureSampler is a FirtreeSampler which knows how to sample from
 * a COGL texture.
 */

G_BEGIN_DECLS

#define FIRTREE_TYPE_COGL_TEXTURE_SAMPLER firtree_cogl_texture_sampler_get_type()

#define FIRTREE_COGL_TEXTURE_SAMPLER(obj) \
    (G_TYPE_CHECK_INSTANCE_CAST ((obj), FIRTREE_TYPE_COGL_TEXTURE_SAMPLER, FirtreeCoglTextureSampler))

#define FIRTREE_COGL_TEXTURE_SAMPLER_CLASS(klass) \
    (G_TYPE_CHECK_CLASS_CAST ((klass), FIRTREE_TYPE_COGL_TEXTURE_SAMPLER, FirtreeCoglTextureSamplerClass))

#define FIRTREE_IS_COGL_TEXTURE_SAMPLER(obj) \
    (G_TYPE_CHECK_INSTANCE_TYPE ((obj), FIRTREE_TYPE_COGL_TEXTURE_SAMPLER))

#define FIRTREE_IS_COGL_TEXTURE_SAMPLER_CLASS(klass) \
    (G_TYPE_CHECK_CLASS_TYPE ((klass), FIRTREE_TYPE_COGL_TEXTURE_SAMPLER))

#define FIRTREE_COGL_TEXTURE_SAMPLER_GET_CLASS(obj) \
    (G_TYPE_INSTANCE_GET_CLASS ((obj), FIRTREE_TYPE_COGL_TEXTURE_SAMPLER, FirtreeCoglTextureSamplerClass))

/**
 * FirtreeCoglTextureSampler:
 * @parent: The parent FirtreeSampler.
 *
 * A structure representing a FirtreeCoglTextureSampler object.
 */
typedef struct {
    FirtreeSampler parent;
} FirtreeCoglTextureSampler;

typedef struct {
    FirtreeSamplerClass parent_class;
} FirtreeCoglTextureSamplerClass;

GType firtree_cogl_texture_sampler_get_type (void);

/**
 * firtree_cogl_texture_sampler_new:
 *
 * Construct an uninitialised cogl_texture sampler. Until this has been associated
 * with a cogl_texture via firtree_cogl_texture_sampler_new_set_cogl_texture(), the sampler is
 * invalid.
 *
 * Returns: A new FirtreeCoglTextureSampler.
 */
FirtreeCoglTextureSampler* 
firtree_cogl_texture_sampler_new (void);

/**
 * firtree_cogl_texture_sampler_set_cogl_texture:
 * @self: A FirtreeCoglTextureSampler.
 * @cogl_texture: A cogl_texture_t.
 *
 * Set @cogl_texture as the cogl_texture associated with this sampler. Drop any references
 * to any other cogl_texture previously associated. The sampler increments the 
 * reference count of the passed cogl_texture to 'claim' it.
 *
 * Pass NULL in order to desociate this sampler with any cogl_texture.
 */
void
firtree_cogl_texture_sampler_set_cogl_texture (FirtreeCoglTextureSampler* self,
        CoglHandle texture);

/**
 * firtree_cogl_texture_sampler_get_cogl_texture:
 * @self: A FirtreeCoglTextureSampler.
 *
 * Retrieve the cogl_texture previously associated with this sampler via
 * firtree_cogl_texture_sampler_set_cogl_texture(). If no cogl_texture is associated,
 * NULL is returned. This will also return the internal cogl texture associated
 * with any texture set via firtree_cogl_texture_sampler_set_clutter_texture().
 *
 * If the caller wishes to maintain a long-lived reference to the cogl_texture,
 * its reference count should be increased.
 *
 * Returns: The cogl_texture associated with the sampler or NULL if there is none.
 */
CoglHandle
firtree_cogl_texture_sampler_get_cogl_texture (FirtreeCoglTextureSampler* self);

/**
 * firtree_cogl_texture_sampler_set_cogl_texture:
 * @self: A FirtreeCoglTextureSampler.
 * @cogl_texture: A cogl_texture_t.
 *
 * Set @clutter_texture as the clutter texture associated with this sampler.
 * Drop any references to any other texture previously associated. The sampler
 * increments the reference count of the passed clutter texture to 'claim' it.
 * 
 * Pass NULL in order to desociate this sampler with any texture.
 */
void firtree_cogl_texture_sampler_set_clutter_texture (
        FirtreeCoglTextureSampler* self, ClutterTexture* texture);

/**
 * firtree_cogl_texture_sampler_get_cogl_texture:
 * @self: A FirtreeCoglTextureSampler.
 *
 * Retrieve the clutter texture previously associated with this sampler via
 * firtree_cogl_texture_sampler_set_clutter_texture(). If no texture is associated,
 * NULL is returned.
 *
 * If the caller wishes to maintain a long-lived reference to the texture,
 * its reference count should be increased. Note that this will return NULL if
 * the texture was set via firtree_cogl_texture_sampler_set_cogl_texture() rather
 * than firtree_cogl_texture_sampler_set_clutter_texture().
 *
 * Returns: The clutter texture associated with the sampler or NULL if there is none.
 */
ClutterTexture*
firtree_cogl_texture_sampler_get_clutter_texture (FirtreeCoglTextureSampler* self);

G_END_DECLS

#endif /* FIRTREE_HAVE_CLUTTER */

#endif /* _FIRTREE_COGL_TEXTURE_SAMPLER */

/* vim:sw=4:ts=4:et:cindent
 */
