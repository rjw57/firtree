/* firtree-kernel-sampler.cc */

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

#define __STDC_LIMIT_MACROS
#define __STDC_CONSTANT_MACROS

#include "internal/firtree-kernel-intl.hh"
#include "internal/firtree-sampler-intl.hh"
#include "firtree-kernel-sampler.h"

G_DEFINE_TYPE (FirtreeKernelSampler, firtree_kernel_sampler, FIRTREE_TYPE_SAMPLER)

#define GET_PRIVATE(o) \
    (G_TYPE_INSTANCE_GET_PRIVATE ((o), FIRTREE_TYPE_KERNEL_SAMPLER, FirtreeKernelSamplerPrivate))

typedef struct _FirtreeKernelSamplerPrivate FirtreeKernelSamplerPrivate;

struct _FirtreeKernelSamplerPrivate {
    int dummy;
};

static void
firtree_kernel_sampler_get_property (GObject *object, guint property_id,
        GValue *value, GParamSpec *pspec)
{
    switch (property_id) {
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    }
}

static void
firtree_kernel_sampler_set_property (GObject *object, guint property_id,
        const GValue *value, GParamSpec *pspec)
{
    switch (property_id) {
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    }
}

static void
firtree_kernel_sampler_dispose (GObject *object)
{
    G_OBJECT_CLASS (firtree_kernel_sampler_parent_class)->dispose (object);
}

static void
firtree_kernel_sampler_finalize (GObject *object)
{
    G_OBJECT_CLASS (firtree_kernel_sampler_parent_class)->finalize (object);
}

static void
firtree_kernel_sampler_class_init (FirtreeKernelSamplerClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS (klass);

    g_type_class_add_private (klass, sizeof (FirtreeKernelSamplerPrivate));

    object_class->get_property = firtree_kernel_sampler_get_property;
    object_class->set_property = firtree_kernel_sampler_set_property;
    object_class->dispose = firtree_kernel_sampler_dispose;
    object_class->finalize = firtree_kernel_sampler_finalize;
}

static void
firtree_kernel_sampler_init (FirtreeKernelSampler *self)
{
}

FirtreeKernelSampler*
firtree_kernel_sampler_new (void)
{
    return (FirtreeKernelSampler*)
        g_object_new (FIRTREE_TYPE_KERNEL_SAMPLER, NULL);
}

/* vim:sw=4:ts=4:et:cindent
 */
