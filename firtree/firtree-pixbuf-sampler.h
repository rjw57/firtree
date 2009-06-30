/* firtree-pixbuf-sampler.h */

/* Firtree - A generic pixbuf processing library
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

#ifndef _FIRTREE_PIXBUF_SAMPLER
#define _FIRTREE_PIXBUF_SAMPLER

#include <glib-object.h>
#include <gdk-pixbuf/gdk-pixbuf.h>

#include "firtree-sampler.h"

/**
 * SECTION:firtree-pixbuf-sampler
 * @short_description: A FirtreeSampler which can sample from a GdkPixbuf.
 * @include: firtree/firtree-pixbuf-sampler.h
 *
 * A FirtreePixbufSampler is a FirtreeSampler which knows how to sample from
 * a GdkPixbuf.
 */

G_BEGIN_DECLS

#define FIRTREE_TYPE_PIXBUF_SAMPLER firtree_pixbuf_sampler_get_type()

#define FIRTREE_PIXBUF_SAMPLER(obj) \
    (G_TYPE_CHECK_INSTANCE_CAST ((obj), FIRTREE_TYPE_PIXBUF_SAMPLER, FirtreePixbufSampler))

#define FIRTREE_PIXBUF_SAMPLER_CLASS(klass) \
    (G_TYPE_CHECK_CLASS_CAST ((klass), FIRTREE_TYPE_PIXBUF_SAMPLER, FirtreePixbufSamplerClass))

#define FIRTREE_IS_PIXBUF_SAMPLER(obj) \
    (G_TYPE_CHECK_INSTANCE_TYPE ((obj), FIRTREE_TYPE_PIXBUF_SAMPLER))

#define FIRTREE_IS_PIXBUF_SAMPLER_CLASS(klass) \
    (G_TYPE_CHECK_CLASS_TYPE ((klass), FIRTREE_TYPE_PIXBUF_SAMPLER))

#define FIRTREE_PIXBUF_SAMPLER_GET_CLASS(obj) \
    (G_TYPE_INSTANCE_GET_CLASS ((obj), FIRTREE_TYPE_PIXBUF_SAMPLER, FirtreePixbufSamplerClass))

/**
 * FirtreePixbufSampler:
 * @parent: The parent FirtreeSampler.
 *
 * A structure representing a FirtreePixbufSampler object.
 */
typedef struct {
    FirtreeSampler parent;
} FirtreePixbufSampler;

typedef struct {
    FirtreeSamplerClass parent_class;
} FirtreePixbufSamplerClass;

GType firtree_pixbuf_sampler_get_type (void);

/**
 * firtree_pixbuf_sampler_new:
 *
 * Construct an uninitialised pixbuf sampler. Until this has been associated
 * with a pixbuf via firtree_pixbuf_sampler_new_set_pixbuf(), the sampler is
 * invalid.
 *
 * Returns: A new FirtreePixbufSampler.
 */
FirtreePixbufSampler* 
firtree_pixbuf_sampler_new (void);

/**
 * firtree_pixbuf_sampler_set_pixbuf:
 * @self: A FirtreePixbufSampler.
 * @pixbuf: A GdkPixbuf.
 *
 * Set @pixbuf as the pixbuf associated with this sampler. Drop any references
 * to any other pixbuf previously associated. The sampler increments the 
 * reference count of the passed pixbuf to 'claim' it.
 *
 * Pass NULL in order to desociate this sampler with any pixbuf.
 */
void
firtree_pixbuf_sampler_set_pixbuf (FirtreePixbufSampler* self,
        GdkPixbuf* pixbuf);

/**
 * firtree_pixbuf_sampler_get_pixbuf:
 * @self: A FirtreePixbufSampler.
 *
 * Retrieve the pixbuf previously associated with this sampler via
 * firtree_pixbuf_sampler_set_pixbuf(). If no pixbuf is associated,
 * NULL is returned.
 *
 * If the caller wishes to maintain a long-lived reference to the pixbuf,
 * its reference count should be increased.
 *
 * Returns: The pixbuf associated with the sampler or NULL if there is none.
 */
GdkPixbuf*
firtree_pixbuf_sampler_get_pixbuf (FirtreePixbufSampler* self);

/**
 * firtree_pixbuf_sampler_get_do_interpolation:
 * @self:  A FirtreePixbufSampler.
 *
 * Get a flag which indicates if the sampler should attempt linear interpolation
 * of the pixel values.
 *
 * Returns: A flag indicating if interpolation is performed.
 */
gboolean
firtree_pixbuf_sampler_get_do_interpolation (FirtreePixbufSampler* self);

/**
 * firtree_pixbuf_sampler_set_do_interpolation:
 * @self:  A FirtreePixbufSampler.
 * @do_interpolation: A flag indicating if interpolation is performed.
 *
 * Set a flag which indicates if the sampler should attempt linear interpolation
 * of the pixel values.
 */
void
firtree_pixbuf_sampler_set_do_interpolation (FirtreePixbufSampler* self,
        gboolean do_interpolation);

G_END_DECLS

#endif /* _FIRTREE_PIXBUF_SAMPLER */

/* vim:sw=4:ts=4:et:cindent
 */
