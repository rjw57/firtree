/* firtree-image.h */

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

#ifndef _FIRTREE_IMAGE
#define _FIRTREE_IMAGE

#include <glib-object.h>

/**
 * SECTION:firtree-image
 * @short_description: Compile Firtree images from source.
 * @include: firtree/firtree-image.h
 *
 * A FirtreeImage encapsulates a image compiled from source code in the
 * Firtree image language.
 */

G_BEGIN_DECLS

#define FIRTREE_TYPE_IMAGE firtree_image_get_type()

#define FIRTREE_IMAGE(obj) \
    (G_TYPE_CHECK_INSTANCE_CAST ((obj), FIRTREE_TYPE_IMAGE, FirtreeImage))

#define FIRTREE_IMAGE_CLASS(klass) \
    (G_TYPE_CHECK_CLASS_CAST ((klass), FIRTREE_TYPE_IMAGE, FirtreeImageClass))

#define FIRTREE_IS_IMAGE(obj) \
    (G_TYPE_CHECK_INSTANCE_TYPE ((obj), FIRTREE_TYPE_IMAGE))

#define FIRTREE_IS_IMAGE_CLASS(klass) \
    (G_TYPE_CHECK_CLASS_TYPE ((klass), FIRTREE_TYPE_IMAGE))

#define FIRTREE_IMAGE_GET_CLASS(obj) \
    (G_TYPE_INSTANCE_GET_CLASS ((obj), FIRTREE_TYPE_IMAGE, FirtreeImageClass))

/**
 * FirtreeImage:
 * @parent: The parent GObject.
 *
 * A FirtreeImage encapsulates a image compiled from source code in the
 * Firtree image language.
 */
typedef struct {
    GObject parent;
} FirtreeImage;

typedef struct {
    GObjectClass parent_class;
} FirtreeImageClass;

GType firtree_image_get_type (void);

/**
 * firtree_image_new:
 *
 * Create a firtree image. Call firtree_image_compile_from_source() to
 * actually compile a image.
 *
 * Returns: A new FirtreeImage instance.
 */
FirtreeImage* firtree_image_new (void);

G_END_DECLS

#endif /* _FIRTREE_IMAGE */

/* vim:sw=4:ts=4:et:cindent
 */
