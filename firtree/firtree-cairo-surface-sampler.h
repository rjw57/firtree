/* firtree-cairo-surface-sampler.h */

/* Firtree - A generic cairo_surface processing library
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

#ifndef _FIRTREE_CAIRO_SURFACE_SAMPLER
#define _FIRTREE_CAIRO_SURFACE_SAMPLER

#include "firtree.h"

#if FIRTREE_HAVE_CAIRO

#include <glib-object.h>
#include <cairo/cairo.h>

#include "firtree-sampler.h"

/**
 * SECTION:firtree-cairo-surface-sampler
 * @short_description: A FirtreeSampler which can sample from a Cairo image surface.
 * @include: firtree/firtree-cairo-surface-sampler.h
 *
 * A FirtreeCairoSurfaceSampler is a FirtreeSampler which knows how to sample from
 * a Cairo image surface.
 */

G_BEGIN_DECLS

#define FIRTREE_TYPE_CAIRO_SURFACE_SAMPLER firtree_cairo_surface_sampler_get_type()

#define FIRTREE_CAIRO_SURFACE_SAMPLER(obj) \
    (G_TYPE_CHECK_INSTANCE_CAST ((obj), FIRTREE_TYPE_CAIRO_SURFACE_SAMPLER, FirtreeCairoSurfaceSampler))

#define FIRTREE_CAIRO_SURFACE_SAMPLER_CLASS(klass) \
    (G_TYPE_CHECK_CLASS_CAST ((klass), FIRTREE_TYPE_CAIRO_SURFACE_SAMPLER, FirtreeCairoSurfaceSamplerClass))

#define FIRTREE_IS_CAIRO_SURFACE_SAMPLER(obj) \
    (G_TYPE_CHECK_INSTANCE_TYPE ((obj), FIRTREE_TYPE_CAIRO_SURFACE_SAMPLER))

#define FIRTREE_IS_CAIRO_SURFACE_SAMPLER_CLASS(klass) \
    (G_TYPE_CHECK_CLASS_TYPE ((klass), FIRTREE_TYPE_CAIRO_SURFACE_SAMPLER))

#define FIRTREE_CAIRO_SURFACE_SAMPLER_GET_CLASS(obj) \
    (G_TYPE_INSTANCE_GET_CLASS ((obj), FIRTREE_TYPE_CAIRO_SURFACE_SAMPLER, FirtreeCairoSurfaceSamplerClass))

/**
 * FirtreeCairoSurfaceSampler:
 * @parent: The parent FirtreeSampler.
 *
 * A structure representing a FirtreeCairoSurfaceSampler object.
 */
typedef struct {
    FirtreeSampler parent;
} FirtreeCairoSurfaceSampler;

typedef struct {
    FirtreeSamplerClass parent_class;
} FirtreeCairoSurfaceSamplerClass;

GType firtree_cairo_surface_sampler_get_type (void);

/**
 * firtree_cairo_surface_sampler_new:
 *
 * Construct an uninitialised cairo_surface sampler. Until this has been associated
 * with a cairo_surface via firtree_cairo_surface_sampler_new_set_cairo_surface(), the sampler is
 * invalid.
 *
 * Returns: A new FirtreeCairoSurfaceSampler.
 */
FirtreeCairoSurfaceSampler* 
firtree_cairo_surface_sampler_new (void);

/**
 * firtree_cairo_surface_sampler_set_cairo_surface:
 * @self: A FirtreeCairoSurfaceSampler.
 * @cairo_surface: A cairo_surface_t.
 *
 * Set @cairo_surface as the cairo_surface associated with this sampler. Drop any references
 * to any other cairo_surface previously associated. The sampler increments the 
 * reference count of the passed cairo_surface to 'claim' it.
 *
 * Pass NULL in order to desociate this sampler with any cairo_surface.
 */
void
firtree_cairo_surface_sampler_set_cairo_surface (FirtreeCairoSurfaceSampler* self,
        cairo_surface_t* cairo_surface);

/**
 * firtree_cairo_surface_sampler_get_cairo_surface:
 * @self: A FirtreeCairoSurfaceSampler.
 *
 * Retrieve the cairo_surface previously associated with this sampler via
 * firtree_cairo_surface_sampler_set_cairo_surface(). If no cairo_surface is associated,
 * NULL is returned.
 *
 * If the caller wishes to maintain a long-lived reference to the cairo_surface,
 * its reference count should be increased.
 *
 * Returns: The cairo_surface associated with the sampler or NULL if there is none.
 */
cairo_surface_t*
firtree_cairo_surface_sampler_get_cairo_surface (FirtreeCairoSurfaceSampler* self);

/**
 * firtree_cairo_surface_sampler_get_do_interpolation:
 * @self:  A FirtreeCairoSurfaceSampler.
 *
 * Get a flag which indicates if the sampler should attempt linear interpolation
 * of the pixel values.
 *
 * Returns: A flag indicating if interpolation is performed.
 */
gboolean
firtree_cairo_surface_sampler_get_do_interpolation (FirtreeCairoSurfaceSampler* self);

/**
 * firtree_cairo_surface_sampler_set_do_interpolation:
 * @self:  A FirtreeCairoSurfaceSampler.
 * @do_interpolation: A flag indicating if interpolation is performed.
 *
 * Set a flag which indicates if the sampler should attempt linear interpolation
 * of the pixel values.
 */
void
firtree_cairo_surface_sampler_set_do_interpolation (FirtreeCairoSurfaceSampler* self,
        gboolean do_interpolation);

G_END_DECLS

#endif /* FIRTREE_HAVE_CAIRO */

#endif /* _FIRTREE_CAIRO_SURFACE_SAMPLER */

/* vim:sw=4:ts=4:et:cindent
 */
