/* firtree-image.c */

#define __STDC_LIMIT_MACROS
#define __STDC_CONSTANT_MACROS

#include "internal/firtree-image-intl.hh"
#include "firtree-sampler.h"
#include "firtree-vector.h"

G_DEFINE_TYPE (FirtreeImage, firtree_image, G_TYPE_OBJECT)

#define GET_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), FIRTREE_TYPE_IMAGE, FirtreeImagePrivate))

typedef struct _FirtreeImagePrivate FirtreeImagePrivate;

struct _FirtreeImagePrivate {
    guint   dummy;
};

static void
firtree_image_get_property (GObject *object, guint property_id,
                              GValue *value, GParamSpec *pspec)
{
    FirtreeImagePrivate* p = GET_PRIVATE(object);
    switch (property_id) {
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    }
}

static void
firtree_image_set_property (GObject *object, guint property_id,
        const GValue *value, GParamSpec *pspec)
{
    switch (property_id) {
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    }
}

static void
_firtree_image_reset_compile_status (FirtreeImage *self)
{
    FirtreeImagePrivate* p = GET_PRIVATE(self);
}

static void
firtree_image_dispose (GObject *object)
{
    FirtreeImagePrivate* p = GET_PRIVATE(object);

    _firtree_image_reset_compile_status(FIRTREE_IMAGE(object));

    G_OBJECT_CLASS (firtree_image_parent_class)->dispose (object);
}

static void
firtree_image_finalize (GObject *object)
{
    G_OBJECT_CLASS (firtree_image_parent_class)->finalize (object);
}

static void
firtree_image_class_init (FirtreeImageClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS (klass);
    GParamSpec* param_spec;

    g_type_class_add_private (klass, sizeof (FirtreeImagePrivate));

    object_class->get_property = firtree_image_get_property;
    object_class->set_property = firtree_image_set_property;
    object_class->dispose = firtree_image_dispose;
    object_class->finalize = firtree_image_finalize;
}

static void
firtree_image_init (FirtreeImage *self)
{
    FirtreeImagePrivate* p = GET_PRIVATE(self);
}

FirtreeImage*
firtree_image_new (void)
{
    return (FirtreeImage*) g_object_new (FIRTREE_TYPE_IMAGE, NULL);
}

/* vim:sw=4:ts=4:et:cindent
 */
