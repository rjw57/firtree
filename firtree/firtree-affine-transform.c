/* firtree-affine-transform.c */

/* Firtree - A generic image processing library
 * Copyright (C) 2009 Rich Wareham <richwareham@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License verstion as published
 * by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.        See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program; if not, write to the Free Software Foundation, Inc., 51
 * Franklin Street, Fifth Floor, Boston, MA        02110-1301, USA
 */

#include "firtree-affine-transform.h"

G_DEFINE_TYPE (FirtreeAffineTransform, firtree_affine_transform, G_TYPE_OBJECT)

#define GET_PRIVATE(o) \
    (G_TYPE_INSTANCE_GET_PRIVATE ((o), FIRTREE_TYPE_AFFINE_TRANSFORM, FirtreeAffineTransformPrivate))

typedef struct _FirtreeAffineTransformPrivate FirtreeAffineTransformPrivate;

struct _FirtreeAffineTransformPrivate {
        int dummy;
};

static void
firtree_affine_transform_get_property (GObject *object, guint property_id,
                                                            GValue *value, GParamSpec *pspec)
{
    switch (property_id) {
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    }
}

static void
firtree_affine_transform_set_property (GObject *object, guint property_id,
                                                            const GValue *value, GParamSpec *pspec)
{
    switch (property_id) {
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    }
}

static void
firtree_affine_transform_dispose (GObject *object)
{
    G_OBJECT_CLASS (firtree_affine_transform_parent_class)->dispose (object);
}

static void
firtree_affine_transform_finalize (GObject *object)
{
    G_OBJECT_CLASS (firtree_affine_transform_parent_class)->finalize (object);
}

static void
firtree_affine_transform_class_init (FirtreeAffineTransformClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS (klass);

    g_type_class_add_private (klass, sizeof (FirtreeAffineTransformPrivate));

    object_class->get_property = firtree_affine_transform_get_property;
    object_class->set_property = firtree_affine_transform_set_property;
    object_class->dispose = firtree_affine_transform_dispose;
    object_class->finalize = firtree_affine_transform_finalize;
}

static void
firtree_affine_transform_init (FirtreeAffineTransform *self)
{
}

FirtreeAffineTransform*
firtree_affine_transform_new (void)
{
    return g_object_new (FIRTREE_TYPE_AFFINE_TRANSFORM, NULL);
}

FirtreeAffineTransform* 
firtree_affine_transform_clone (FirtreeAffineTransform* self)
{
    return NULL;
}

gboolean
firtree_affine_transform_is_identity (FirtreeAffineTransform* self)
{
    return FALSE;
}

void
firtree_affine_transform_append_transform (FirtreeAffineTransform* self,
        FirtreeAffineTransform* trans)
{
}

void
firtree_affine_transform_prepend_transform (FirtreeAffineTransform* self,
        FirtreeAffineTransform* trans)
{
}

gboolean
firtree_affine_transform_invert (FirtreeAffineTransform* self)
{
    return FALSE;
}

void
firtree_affine_transform_rotate_by_degrees (FirtreeAffineTransform* self,
        float angle)
{
}

void
firtree_affine_transform_rotate_by_radians (FirtreeAffineTransform* self,
        float angle)
{
}

void
firtree_affine_transform_scale_by (FirtreeAffineTransform* self, 
        float sx, float sy)
{
}

void
firtree_affine_transform_translate_by (FirtreeAffineTransform* self, 
        float tx, float ty)
{
}

FirtreeVec2
firtree_affine_transform_transform_point (FirtreeAffineTransform* self,
        float x, float y)
{
    FirtreeVec2 vec = {0,0};
    return vec;
}

FirtreeVec2
firtree_affine_transform_transform_size (FirtreeAffineTransform* self,
        float width, float height)
{
    FirtreeVec2 vec = {0,0};
    return vec;
}

void
firtree_affine_transform_set_rotation_by_degrees (FirtreeAffineTransform* self,
        float angle)
{
}

void
firtree_affine_transform_set_rotation_by_radians (FirtreeAffineTransform* self,
        float angle)
{
}

void
firtree_affine_transform_set_scaling_by (FirtreeAffineTransform* self, 
        float sx, float sy)
{
}

void
firtree_affine_transform_set_translation_by (FirtreeAffineTransform* self, 
        float tx, float ty)
{
}

/* vim:sw=4:ts=4:et:cindent
 */
