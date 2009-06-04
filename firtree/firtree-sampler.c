/* firtree-sampler.c */

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

#include "firtree-sampler.h"

G_DEFINE_TYPE (FirtreeSampler, firtree_sampler, G_TYPE_OBJECT)

#define GET_PRIVATE(o) \
    (G_TYPE_INSTANCE_GET_PRIVATE ((o), FIRTREE_TYPE_SAMPLER, FirtreeSamplerPrivate))

typedef struct _FirtreeSamplerPrivate FirtreeSamplerPrivate;

struct _FirtreeSamplerPrivate {
        int dummy;
};

static void
firtree_sampler_get_property (GObject *object, guint property_id,
                                                            GValue *value, GParamSpec *pspec)
{
    switch (property_id) {
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    }
}

static void
firtree_sampler_set_property (GObject *object, guint property_id,
                                                            const GValue *value, GParamSpec *pspec)
{
    switch (property_id) {
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    }
}

static void
firtree_sampler_dispose (GObject *object)
{
    G_OBJECT_CLASS (firtree_sampler_parent_class)->dispose (object);
}

static void
firtree_sampler_finalize (GObject *object)
{
    G_OBJECT_CLASS (firtree_sampler_parent_class)->finalize (object);
}

static void
firtree_sampler_class_init (FirtreeSamplerClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS (klass);

    g_type_class_add_private (klass, sizeof (FirtreeSamplerPrivate));

    object_class->get_property = firtree_sampler_get_property;
    object_class->set_property = firtree_sampler_set_property;
    object_class->dispose = firtree_sampler_dispose;
    object_class->finalize = firtree_sampler_finalize;
}

static void
firtree_sampler_init (FirtreeSampler *self)
{
}

FirtreeSampler*
firtree_sampler_new (void)
{
    return g_object_new (FIRTREE_TYPE_SAMPLER, NULL);
}

FirtreeVec4
firtree_sampler_get_extent (FirtreeSampler* self)
{
    FirtreeVec4 extent = { 0, 0, 0, 0 };
    return extent;
}

/* vim:sw=4:ts=4:et:cindent
 */
