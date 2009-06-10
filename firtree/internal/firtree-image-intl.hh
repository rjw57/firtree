/* firtree-image-intl.hpp */

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

#ifndef _FIRTREE_IMAGE_INTL
#define _FIRTREE_IMAGE_INTL

#include "../firtree-image.h"

G_BEGIN_DECLS

/**
 * SECTION:firtree-image-intl
 * @short_description: Internal image interface
 * @include: firtree/internal/firtree-image-intl.hh
 *
 * The private interface for images.
 */

/**
 * firtree_image_lock_into_memory:
 * @self: The FirtreeImage to lock.
 * @preferred_format: The format the caller would prefer the image in.
 * @buffer: On exit, this is updated to point to the image buffer.
 * @width: On exit, this is updated to point to the image width in pixels.
 * @height: On exit, this is updated to point to the image height in pixels.
 * @stride: On exit, this is updated to point to the image row stride in bytes.
 * @format: On exit, this is updated to point to the format the buffer has been
 * locked into memory as.
 *
 * Locks the image into memory and returns information on the buffer. The
 * caller may request the image in a certain format the function is not
 * mandated to return the buffer in that format. Instead the caller should check
 * the value placed into @format on exit.
 *
 * If the image could not be locked into memory, this returns NULL otherwise
 * it returns a pointer to an opaque handle which should be passed to 
 * firtree_image_unlock_from_memory() when the caller is finished with the image.
 *
 * This call may call an implicit image to be rendered and so should only be 
 * used when necessary.
 *
 * Returns: NULL or a handle which should be passed to
 * firtree_image_unlock_from_memory().
 */
gpointer
firtree_image_lock_into_memory(FirtreeImage* self,
        FirtreeImageFormat preferred_format,
        guchar** buffer, guint* width, guint* height, guint* stride,
        FirtreeImageFormat* format);

/**
 * firtree_image_unlock_from_memory:
 * @self: The FirtreeImage to lock.
 * @handle: A handle returned from firtree_image_lock_into_memory().
 * 
 * After the image buffer returned from firtree_image_lock_into_memory() is
 * finished with, this function should be called to let the image know that
 * the buffer is no-longer required.
 */
void
firtree_image_unlock_from_memory(FirtreeImage* self, gpointer handle);

G_END_DECLS

#endif /* _FIRTREE_IMAGE_INTL */

/* vim:sw=4:ts=4:et:cindent
 */
