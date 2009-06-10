/* firtree-image-sampler.h */

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

#ifndef _FIRTREE_IMAGE_SAMPLER
#define _FIRTREE_IMAGE_SAMPLER

#include <glib-object.h>

#include "firtree-sampler.h"

/**
 * SECTION:firtree-image-sampler
 * @short_description: A FirtreeSampler which can sample from a FirtreeImage.
 * @include: firtree/firtree-image-sampler.h
 *
 * A FirtreeImageSampler is a FirtreeSampler which knows how to sample from
 * a FirtreeImage.
 */

G_BEGIN_DECLS

#define FIRTREE_TYPE_IMAGE_SAMPLER firtree_image_sampler_get_type()

#define FIRTREE_IMAGE_SAMPLER(obj) \
    (G_TYPE_CHECK_INSTANCE_CAST ((obj), FIRTREE_TYPE_IMAGE_SAMPLER, FirtreeImageSampler))

#define FIRTREE_IMAGE_SAMPLER_CLASS(klass) \
    (G_TYPE_CHECK_CLASS_CAST ((klass), FIRTREE_TYPE_IMAGE_SAMPLER, FirtreeImageSamplerClass))

#define FIRTREE_IS_IMAGE_SAMPLER(obj) \
    (G_TYPE_CHECK_INSTANCE_TYPE ((obj), FIRTREE_TYPE_IMAGE_SAMPLER))

#define FIRTREE_IS_IMAGE_SAMPLER_CLASS(klass) \
    (G_TYPE_CHECK_CLASS_TYPE ((klass), FIRTREE_TYPE_IMAGE_SAMPLER))

#define FIRTREE_IMAGE_SAMPLER_GET_CLASS(obj) \
    (G_TYPE_INSTANCE_GET_CLASS ((obj), FIRTREE_TYPE_IMAGE_SAMPLER, FirtreeImageSamplerClass))

/**
 * FirtreeImageSampler:
 * @parent: The parent FirtreeSampler.
 *
 * A structure representing a FirtreeImageSampler object.
 */
typedef struct {
    FirtreeSampler parent;
} FirtreeImageSampler;

typedef struct {
    FirtreeSamplerClass parent_class;
} FirtreeImageSamplerClass;

GType firtree_image_sampler_get_type (void);

/**
 * firtree_image_sampler_new:
 *
 * Construct an uninitialised image sampler. Until this has been associated
 * with a image via firtree_image_sampler_new_set_image(), the sampler is
 * invalid.
 *
 * Returns: A new FirtreeImageSampler.
 */
FirtreeImageSampler* 
firtree_image_sampler_new (void);

/**
 * firtree_image_sampler_set_image:
 * @self: A FirtreeImageSampler.
 * @image: A FirtreeImage.
 *
 * Set @image as the image associated with this sampler. Drop any references
 * to any other image previously associated. The sampler increments the 
 * reference count of the passed image to 'claim' it.
 *
 * Pass NULL in order to desociate this sampler with any image.
 */
void
firtree_image_sampler_set_image (FirtreeImageSampler* self,
        FirtreeImage* image);

/**
 * firtree_image_sampler_get_image:
 * @self: A FirtreeImageSampler.
 *
 * Retrieve the image previously associated with this sampler via
 * firtree_image_sampler_set_image(). If no image is associated,
 * NULL is returned.
 *
 * If the caller wishes to maintain a long-lived reference to the image,
 * its reference count should be increased.
 *
 * Returns: The image associated with the sampler or NULL if there is none.
 */
FirtreeImage*
firtree_image_sampler_get_image (FirtreeImageSampler* self);

G_END_DECLS

#endif /* _FIRTREE_IMAGE_SAMPLER */

/* vim:sw=4:ts=4:et:cindent
 */
