/* firtree-buffer-sampler.h */

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

#ifndef _FIRTREE_BUFFER_SAMPLER
#define _FIRTREE_BUFFER_SAMPLER

#include <glib-object.h>

#include "firtree-types.h"
#include "firtree-sampler.h"

/**
 * SECTION:firtree-buffer-sampler
 * @short_description: A FirtreeSampler which can sample from a buffer in memory.
 * @include: firtree/firtree-buffer-sampler.h
 *
 * A FirtreeBufferSampler is a FirtreeSampler which knows how to sample from
 * a buffer in memory. This is intended to be subclassed so that the 
 * get_buffer virtual function is overridden.
 */

G_BEGIN_DECLS

#define FIRTREE_TYPE_BUFFER_SAMPLER firtree_buffer_sampler_get_type()

#define FIRTREE_BUFFER_SAMPLER(obj) \
    (G_TYPE_CHECK_INSTANCE_CAST ((obj), FIRTREE_TYPE_BUFFER_SAMPLER, FirtreeBufferSampler))

#define FIRTREE_BUFFER_SAMPLER_CLASS(klass) \
    (G_TYPE_CHECK_CLASS_CAST ((klass), FIRTREE_TYPE_BUFFER_SAMPLER, FirtreeBufferSamplerClass))

#define FIRTREE_IS_BUFFER_SAMPLER(obj) \
    (G_TYPE_CHECK_INSTANCE_TYPE ((obj), FIRTREE_TYPE_BUFFER_SAMPLER))

#define FIRTREE_IS_BUFFER_SAMPLER_CLASS(klass) \
    (G_TYPE_CHECK_CLASS_TYPE ((klass), FIRTREE_TYPE_BUFFER_SAMPLER))

#define FIRTREE_BUFFER_SAMPLER_GET_CLASS(obj) \
    (G_TYPE_INSTANCE_GET_CLASS ((obj), FIRTREE_TYPE_BUFFER_SAMPLER, FirtreeBufferSamplerClass))

/**
 * FirtreeBufferSampler:
 * @parent: The parent FirtreeSampler.
 *
 * A structure representing a FirtreeBufferSampler object.
 */
typedef struct {
    FirtreeSampler parent;
} FirtreeBufferSampler;

typedef struct {
    FirtreeSamplerClass parent_class;
} FirtreeBufferSamplerClass;

GType firtree_buffer_sampler_get_type (void);

/**
 * firtree_buffer_sampler_new:
 *
 * Construct an uninitialised buffer sampler. Until this has been associated
 * with a buffer via firtree_buffer_sampler_new_set_buffer(), the sampler is
 * invalid.
 *
 * Returns: A new FirtreeBufferSampler.
 */
FirtreeBufferSampler* 
firtree_buffer_sampler_new (void);

/**
 * firtree_buffer_sampler_get_do_interpolation:
 * @self:  A FirtreeBufferSampler.
 *
 * Get a flag which indicates if the sampler should attempt linear interpolation
 * of the pixel values.
 *
 * Returns: A flag indicating if interpolation is performed.
 */
gboolean
firtree_buffer_sampler_get_do_interpolation (FirtreeBufferSampler* self);

/**
 * firtree_buffer_sampler_set_do_interpolation:
 * @self:  A FirtreeBufferSampler.
 * @do_interpolation: A flag indicating if interpolation is performed.
 *
 * Set a flag which indicates if the sampler should attempt linear interpolation
 * of the pixel values.
 */
void
firtree_buffer_sampler_set_do_interpolation (FirtreeBufferSampler* self,
        gboolean do_interpolation);

/**
 * firtree_buffer_sampler_set_buffer:
 * @self: A FirtreeBufferSampler
 * @buffer: The location of the buffer in memory or NULL to unset the buffer.
 * @width: The buffer width in pixels.
 * @height: The buffer height in rows.
 * @stride: The size of one row in bytes.
 * @format: The format of the buffer.
 *
 * Associate an in-memory buffer with this sampler. The buffer must be
 * valid for the lifetime of the sampler or until 
 * firtree_buffer_sampler_set_buffer() is called again. 
 */
void
firtree_buffer_sampler_set_buffer (FirtreeBufferSampler* self,
            gpointer buffer, guint width, guint height,
            guint stride, FirtreeBufferFormat format);

/**
 * firtree_buffer_sampler_set_buffer_no_copy:
 * @self: A FirtreeBufferSampler
 * @buffer: The location of the buffer in memory or NULL to unset the buffer.
 * @width: The buffer width in pixels.
 * @height: The buffer height in rows.
 * @stride: The size of one row in bytes.
 * @format: The format of the buffer.
 *
 * Associate an in-memory buffer with this sampler like
 * firtree_buffer_sampler_set_buffer() except that no implicit copy is made.
 * The buffer must be valid for the lifetime of the sampler or until
 * firtree_buffer_sampler_set_buffer() or
 * firtree_buffer_sampler_set_buffer_no_copy()  is called again. 
 */
void firtree_buffer_sampler_set_buffer_no_copy (FirtreeBufferSampler* self, gpointer
        buffer, guint width, guint height, guint stride, FirtreeBufferFormat
        format);

/**
 * firtree_buffer_sampler_unset_buffer:
 * @self: A FirtreeBufferSampler
 *
 * Equivalent to calling firtree_buffer_sampler_set_buffer() with
 * NULL as the passed buffer.
 */
void
firtree_buffer_sampler_unset_buffer (FirtreeBufferSampler* self);

G_END_DECLS

#endif /* _FIRTREE_BUFFER_SAMPLER */

/* vim:sw=4:ts=4:et:cindent
 */
