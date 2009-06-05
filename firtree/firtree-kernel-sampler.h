/* firtree-kernel-sampler.h */

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

#ifndef _FIRTREE_KERNEL_SAMPLER
#define _FIRTREE_KERNEL_SAMPLER

#include <glib-object.h>

#include "firtree-sampler.h"

/**
 * SECTION:firtree-kernel-sampler
 * @short_description: A FirtreeSampler which can sample from a FirtreeKernel.
 * @include: firtree/firtree-kernel-sampler.h
 *
 * A FirtreeKernelSampler is a FirtreeSampler which knows how to sample from
 * a FirtreeKernel.
 */

G_BEGIN_DECLS

#define FIRTREE_TYPE_KERNEL_SAMPLER firtree_kernel_sampler_get_type()

#define FIRTREE_KERNEL_SAMPLER(obj) \
    (G_TYPE_CHECK_INSTANCE_CAST ((obj), FIRTREE_TYPE_KERNEL_SAMPLER, FirtreeKernelSampler))

#define FIRTREE_KERNEL_SAMPLER_CLASS(klass) \
    (G_TYPE_CHECK_CLASS_CAST ((klass), FIRTREE_TYPE_KERNEL_SAMPLER, FirtreeKernelSamplerClass))

#define FIRTREE_IS_KERNEL_SAMPLER(obj) \
    (G_TYPE_CHECK_INSTANCE_TYPE ((obj), FIRTREE_TYPE_KERNEL_SAMPLER))

#define FIRTREE_IS_KERNEL_SAMPLER_CLASS(klass) \
    (G_TYPE_CHECK_CLASS_TYPE ((klass), FIRTREE_TYPE_KERNEL_SAMPLER))

#define FIRTREE_KERNEL_SAMPLER_GET_CLASS(obj) \
    (G_TYPE_INSTANCE_GET_CLASS ((obj), FIRTREE_TYPE_KERNEL_SAMPLER, FirtreeKernelSamplerClass))

typedef struct {
    FirtreeSampler parent;
} FirtreeKernelSampler;

typedef struct {
    FirtreeSamplerClass parent_class;
} FirtreeKernelSamplerClass;

GType firtree_kernel_sampler_get_type (void);

/**
 * firtree_kernel_sampler_new:
 *
 * Construct an uninitialised kernel sampler. Until this has been associated
 * with a kernel via firtree_kernel_sampler_new_set_kernel(), the sampler is
 * invalid.
 *
 * Returns: A new FirtreeKernelSampler.
 */
FirtreeKernelSampler* 
firtree_kernel_sampler_new (void);

G_END_DECLS

#endif /* _FIRTREE_KERNEL_SAMPLER */

/* vim:sw=4:ts=4:et:cindent
 */
