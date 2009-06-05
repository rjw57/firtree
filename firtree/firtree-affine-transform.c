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
    firtree_affine_transform_set_identity(self);
}

FirtreeAffineTransform*
firtree_affine_transform_new (void)
{
    return g_object_new (FIRTREE_TYPE_AFFINE_TRANSFORM, NULL);
}

FirtreeAffineTransform* 
firtree_affine_transform_clone (FirtreeAffineTransform* self)
{
    FirtreeAffineTransform* new_trans = firtree_affine_transform_new();
    firtree_affine_transform_set_transform(new_trans, self);
    return new_trans;
}

gboolean
firtree_affine_transform_is_identity (FirtreeAffineTransform* self)
{
    if((self->m11 == 1.f) && (self->m12 == 0.f) &&
            (self->m21 == 0.f) && (self->m22 == 1.f) &&
            (self->tx == 0.f) && (self->ty == 0.f)) 
    {
        return TRUE;
    }
    return FALSE;
}

/* Computes c <- a*b */
static void
_firtree_affine_transform_multiply(
        FirtreeAffineTransform* a,
        FirtreeAffineTransform* b,
        FirtreeAffineTransform* c)
{
    float m11 = (a->m11*b->m11) + (a->m12*b->m21);
    float m12 = (a->m11*b->m12) + (a->m12*b->m22);
    float m21 = (a->m21*b->m11) + (a->m22*b->m21);
    float m22 = (a->m21*b->m12) + (a->m22*b->m22);
    float tx = (a->m11*b->tx) + (a->m12*b->ty) + a->tx;
    float ty = (a->m21*b->tx) + (a->m22*b->ty) + a->ty;
    firtree_affine_transform_set_elements(c, m11, m12, m21, m22, tx, ty);
}

void
firtree_affine_transform_append_transform (FirtreeAffineTransform* self,
        FirtreeAffineTransform* trans)
{
    /* self <- trans * self */
    _firtree_affine_transform_multiply(trans, self, self);
}

void
firtree_affine_transform_prepend_transform (FirtreeAffineTransform* self,
        FirtreeAffineTransform* trans)
{
    /* self <- self * trans */
    _firtree_affine_transform_multiply(self, trans, self);
}

gboolean
firtree_affine_transform_invert (FirtreeAffineTransform* self)
{
    float d = (self->m11*self->m22) - (self->m12*self->m21);
    if(d == 0.f) {
        return FALSE;
    }
    float ood = 1.f / d;

    float m11 = ood * self->m22;
    float m22 = ood * self->m11;
    float m12 = ood * -self->m12;
    float m21 = ood * -self->m21;

    float tx = ood * (self->m12*self->ty - self->m22*self->tx);
    float ty = ood * (self->m21*self->tx - self->m11*self->ty);

    firtree_affine_transform_set_elements(self, m11, m12, m21, m22, tx, ty);

    return TRUE;
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
    FirtreeVec2 vec = {
        self->m11*x + self->m12*y + self->tx,
        self->m21*x + self->m22*y + self->ty,
    };
    return vec;
}

FirtreeVec2
firtree_affine_transform_transform_size (FirtreeAffineTransform* self,
        float width, float height)
{
    FirtreeVec2 vec = {
        self->m11*width + self->m12*height,
        self->m21*width + self->m22*height,
    };
    return vec;
}

void
firtree_affine_transform_set_identity (FirtreeAffineTransform* self)
{
    firtree_affine_transform_set_elements(self, 1, 0, 0, 1, 0, 0);
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

void
firtree_affine_transform_set_elements (FirtreeAffineTransform* self, 
        float m11, float m12, float m21, float m22, float tx, float ty)
{
    self->m11 = m11;
    self->m12 = m12;
    self->m21 = m21;
    self->m22 = m22;
    self->tx = tx;
    self->ty = ty;
}

void
firtree_affine_transform_get_elements (FirtreeAffineTransform* self, 
        float* m11, float* m12, float* m21, float* m22, float* tx, float* ty)
{
    *m11 = self->m11;
    *m12 = self->m12;
    *m21 = self->m21;
    *m22 = self->m22;
    *tx = self->tx;
    *ty = self->ty;
}

void
firtree_affine_transform_set_transform (FirtreeAffineTransform* self, 
        FirtreeAffineTransform* src)
{
    self->m11 = src->m11;
    self->m12 = src->m12;
    self->m21 = src->m21;
    self->m22 = src->m22;
    self->tx = src->tx;
    self->ty = src->ty;
}

/* vim:sw=4:ts=4:et:cindent
 */
