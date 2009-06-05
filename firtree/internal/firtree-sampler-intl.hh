/* firtree-sampler-intl.hh */

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

#ifndef _FIRTREE_SAMPLER_INTL
#define _FIRTREE_SAMPLER_INTL

#include <glib-object.h>

#include "../firtree-sampler.h"

/**
 * SECTION:firtree-sampler-intl
 * @short_description: Internal sampler interface
 * @include: firtree/internal/firtree-sampler-intl.hh
 *
 * The private interface of a sampler.
 */

G_BEGIN_DECLS

struct _FirtreeSamplerIntlVTable {
    gboolean (* get_param) (FirtreeSampler* self, guint param, 
        gpointer dest, guint dest_size);
    llvm::Function* (* get_function) (FirtreeSampler* self);
};

/**
 * firtree_sampler_get_param:
 * @self: A FirtreeSampler instance.
 * @param: An index describing which parameter needs retrieving.
 * @dest: A pointer to a destination to write the parameter value.
 * @dest_size: The expected size of this parameter.
 *
 * A sampler may define parameters which it references in the LLVM module
 * it provides to implement it. These are retrieved through the builtin
 * sampler_get_param() function which has the following signature:
 *
 *    void sampler_get_param(FirtreeSampler* sampler, int param, void* dest);
 *
 * Ultimately, this is mapped into calls to this function. If @param does
 * not correspond to any parameters associated with this function or @dest_size
 * is incorrect, return FALSE. Otherwise, return TRUE.
 *
 * Returns: A flag indicating if the parameter was successfully retrieved.
 */
gboolean
firtree_sampler_get_param(FirtreeSampler* self, guint param, 
        gpointer dest, guint dest_size);

/**
 * firtree_sampler_get_function:
 * @self: A FirtreeSampler instance.
 *
 * Return an LLVM function which implements this sampler. It should take a 
 * 2d float vector specifying the sample co-ordinate and return a 4d float
 * vector giving the result. It's name should be globally unique. Ideally it is
 * the only external function defined in the module.
 */
llvm::Function*
firtree_sampler_get_function(FirtreeSampler* self);

G_END_DECLS

#endif /* _FIRTREE_SAMPLER_INTL */

/* vim:sw=4:ts=4:et:cindent
 */
