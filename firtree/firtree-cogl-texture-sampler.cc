/* firtree-cogl-texture-sampler.cc */

/* Firtree - A generic cogl_texture processing library
 * Copyright (C) 2009 Rich Wareham <richwareham@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as published
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

#include <llvm/Module.h>
#include <llvm/Function.h>
#include <llvm/DerivedTypes.h>
#include <llvm/Instructions.h>
#include <llvm/Constants.h>

#include <common/uuid.h>

#include "internal/firtree-cogl-texture-sampler-intl.hh"
#include "internal/firtree-engine-intl.hh"
#include "internal/firtree-sampler-intl.hh"

/**
 * SECTION:firtree-cogl-texture-sampler
 * @short_description: A FirtreeSampler which can sample from a COGL texture.
 * @include: firtree/firtree-cogl-texture-sampler.h
 *
 * A FirtreeCoglTextureSampler is a FirtreeSampler which knows how to sample from
 * a COGL texture.
 */

/**
 * FirtreeCoglTextureSampler:
 * @parent: The parent FirtreeSampler.
 *
 * A structure representing a FirtreeCoglTextureSampler object.
 */

/**
 * FirtreeCoglTextureSamplerPrivate:
 *
 * A structure representing the internal data of a FirtreeCoglTextureSampler
 * instance.
 */

G_DEFINE_TYPE(FirtreeCoglTextureSampler, firtree_cogl_texture_sampler,
	      FIRTREE_TYPE_SAMPLER)
#define GET_PRIVATE(o) \
    (G_TYPE_INSTANCE_GET_PRIVATE ((o), FIRTREE_TYPE_COGL_TEXTURE_SAMPLER, FirtreeCoglTextureSamplerPrivate))
typedef struct _FirtreeCoglTextureSamplerPrivate
 FirtreeCoglTextureSamplerPrivate;

struct _FirtreeCoglTextureSamplerPrivate {
	CoglHandle cogl_texture;
	ClutterTexture *clutter_texture;
	 llvm::Function * cached_function;

	guchar *cached_data;
	guint cached_data_size;
	guint cached_data_stride;
	FirtreeBufferFormat cached_data_format;
};

llvm::Function *
firtree_cogl_texture_sampler_get_sample_function(FirtreeSampler * self);

llvm::Function *
_firtree_cogl_texture_sampler_create_sample_function(FirtreeCoglTextureSampler *
						     self);

FirtreeVec4
firtree_cogl_texture_surface_sampler_get_extent(FirtreeSampler * self)
{
	FirtreeCoglTextureSamplerPrivate *p = GET_PRIVATE(self);
	CoglHandle tex =
	    firtree_cogl_texture_sampler_get_cogl_texture
	    (FIRTREE_COGL_TEXTURE_SAMPLER(self));

	if (!p || !tex) {
		/* return 'NULL' extent */
		FirtreeVec4 rv = { 0, 0, 0, 0 };
		return rv;
	}

	FirtreeVec4 rv = { 0, 0,
		cogl_texture_get_width(tex),
		cogl_texture_get_height(tex)
	};
	return rv;
}

/* invalidate (and release) any cached LLVM modules/functions. This
 * will cause them to be re-generated when ..._get_function() is next
 * called. */
static void
_firtree_cogl_texture_sampler_invalidate_llvm_cache(FirtreeCoglTextureSampler *
						    self)
{
	FirtreeCoglTextureSamplerPrivate *p = GET_PRIVATE(self);
	if (p && p->cached_function) {
		delete p->cached_function->getParent();
		p->cached_function = NULL;
	}

	if (p && p->cached_data) {
		g_slice_free1(p->cached_data_size, p->cached_data);
		p->cached_data = NULL;
		p->cached_data_size = 0;
		p->cached_data_stride = 0;
		p->cached_data_format = FIRTREE_FORMAT_LAST;
	}

	firtree_sampler_module_changed(FIRTREE_SAMPLER(self));
}

static void firtree_cogl_texture_sampler_dispose(GObject * object)
{
	G_OBJECT_CLASS(firtree_cogl_texture_sampler_parent_class)->dispose
	    (object);

	/* drop any references to any cogl_texture we might have. */
	firtree_cogl_texture_sampler_set_clutter_texture((FirtreeCoglTextureSampler *) object, NULL);

	/* dispose of any LLVM modules we might have. */
	_firtree_cogl_texture_sampler_invalidate_llvm_cache((FirtreeCoglTextureSampler *) object);
}

static FirtreeSamplerIntlVTable _firtree_cogl_texture_sampler_class_vtable;

static void
firtree_cogl_texture_sampler_class_init(FirtreeCoglTextureSamplerClass * klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS(klass);

	g_type_class_add_private(klass,
				 sizeof(FirtreeCoglTextureSamplerPrivate));

	object_class->dispose = firtree_cogl_texture_sampler_dispose;

	/* override the sampler virtual functions with our own */
	FirtreeSamplerClass *sampler_class = FIRTREE_SAMPLER_CLASS(klass);

	sampler_class->get_extent =
	    firtree_cogl_texture_surface_sampler_get_extent;

	sampler_class->intl_vtable =
	    &_firtree_cogl_texture_sampler_class_vtable;

	sampler_class->intl_vtable->get_sample_function =
	    firtree_cogl_texture_sampler_get_sample_function;
}

static void firtree_cogl_texture_sampler_init(FirtreeCoglTextureSampler * self)
{
	FirtreeCoglTextureSamplerPrivate *p = GET_PRIVATE(self);
	p->cogl_texture = NULL;
	p->clutter_texture = NULL;
	p->cached_function = NULL;
	p->cached_data = NULL;
	p->cached_data_size = 0;
	p->cached_data_stride = 0;
	p->cached_data_format = FIRTREE_FORMAT_LAST;
}

/**
 * firtree_cogl_texture_sampler_new:
 *
 * Construct an uninitialised cogl_texture sampler. Until this has been associated
 * with a cogl_texture via firtree_cogl_texture_sampler_new_set_cogl_texture(), the sampler is
 * invalid.
 *
 * Returns: A new FirtreeCoglTextureSampler.
 */
FirtreeCoglTextureSampler *firtree_cogl_texture_sampler_new(void)
{
	return (FirtreeCoglTextureSampler *)
	    g_object_new(FIRTREE_TYPE_COGL_TEXTURE_SAMPLER, NULL);
}

/**
 * firtree_cogl_texture_sampler_set_cogl_texture:
 * @self: A FirtreeCoglTextureSampler.
 * @texture: A cogl_texture_t.
 *
 * Set @cogl_texture as the cogl_texture associated with this sampler. Drop any references
 * to any other cogl_texture previously associated. The sampler increments the 
 * reference count of the passed cogl_texture to 'claim' it.
 *
 * Pass NULL in order to desociate this sampler with any cogl_texture.
 */
void
firtree_cogl_texture_sampler_set_cogl_texture(FirtreeCoglTextureSampler * self,
					      CoglHandle cogl_texture)
{
	FirtreeCoglTextureSamplerPrivate *p = GET_PRIVATE(self);
	/* unref any cogl_texture we already have. */

	if (p->cogl_texture) {
		cogl_texture_unref(p->cogl_texture);
		p->cogl_texture = NULL;
	}

	if (p->clutter_texture) {
		g_object_unref(p->clutter_texture);
		p->clutter_texture = NULL;
	}

	if (cogl_texture) {
		cogl_texture_ref(cogl_texture);
		p->cogl_texture = cogl_texture;
	}

	_firtree_cogl_texture_sampler_invalidate_llvm_cache(self);

	firtree_sampler_extents_changed(FIRTREE_SAMPLER(self));
}

/**
 * firtree_cogl_texture_sampler_get_cogl_texture:
 * @self: A FirtreeCoglTextureSampler.
 *
 * Retrieve the cogl_texture previously associated with this sampler via
 * firtree_cogl_texture_sampler_set_cogl_texture(). If no cogl_texture is associated,
 * NULL is returned. This will also return the internal cogl texture associated
 * with any texture set via firtree_cogl_texture_sampler_set_clutter_texture().
 *
 * If the caller wishes to maintain a long-lived reference to the cogl_texture,
 * its reference count should be increased.
 *
 * Returns: The cogl_texture associated with the sampler or NULL if there is none.
 */
CoglHandle
firtree_cogl_texture_sampler_get_cogl_texture(FirtreeCoglTextureSampler * self)
{
	FirtreeCoglTextureSamplerPrivate *p = GET_PRIVATE(self);

	if (p->clutter_texture) {
		return clutter_texture_get_cogl_texture(p->clutter_texture);
	}

	return p->cogl_texture;
}

/**
 * firtree_cogl_texture_sampler_set_clutter_texture:
 * @self: A FirtreeCoglTextureSampler.
 * @texture: A Clutter texture.
 *
 * Set @texture as the clutter texture associated with this sampler.
 * Drop any references to any other texture previously associated. The sampler
 * increments the reference count of the passed clutter texture to 'claim' it.
 * 
 * Pass NULL in order to desociate this sampler with any texture.
 */
void firtree_cogl_texture_sampler_set_clutter_texture(FirtreeCoglTextureSampler
						      * self,
						      ClutterTexture * texture)
{
	FirtreeCoglTextureSamplerPrivate *p = GET_PRIVATE(self);
	/* unref any cogl_texture we already have. */

	if (p->cogl_texture) {
		cogl_texture_unref(p->cogl_texture);
		p->cogl_texture = NULL;
	}

	if (p->clutter_texture) {
		g_object_unref(p->clutter_texture);
		p->clutter_texture = NULL;
	}

	if (texture) {
		g_object_ref(texture);
		p->clutter_texture = texture;
	}

	_firtree_cogl_texture_sampler_invalidate_llvm_cache(self);
}

/**
 * firtree_cogl_texture_sampler_get_clutter_texture:
 * @self: A FirtreeCoglTextureSampler.
 *
 * Retrieve the clutter texture previously associated with this sampler via
 * firtree_cogl_texture_sampler_set_clutter_texture(). If no texture is associated,
 * NULL is returned.
 *
 * If the caller wishes to maintain a long-lived reference to the texture,
 * its reference count should be increased. Note that this will return NULL if
 * the texture was set via firtree_cogl_texture_sampler_set_cogl_texture() rather
 * than firtree_cogl_texture_sampler_set_clutter_texture().
 *
 * Returns: The clutter texture associated with the sampler or NULL if there is none.
 */
ClutterTexture
    * firtree_cogl_texture_sampler_get_clutter_texture(FirtreeCoglTextureSampler
						       * self)
{
	FirtreeCoglTextureSamplerPrivate *p = GET_PRIVATE(self);
	return p->clutter_texture;
}

/**
 * firtree_cogl_texture_sampler_get_data:
 * @self: A FirtreeCoglTextureSampler instance.
 * @data: On exit set to a pointer to the texture data.
 * @rowstride: On exit set the the number od bytes per row.
 * @out_format: On exit set to the format of the texture data.
 *
 * Note: Internal method.
 *
 * Returns: The size of the cached data.
 */
guint
firtree_cogl_texture_sampler_get_data(FirtreeCoglTextureSampler * self,
				      guchar ** data, guint * rowstride,
				      FirtreeBufferFormat * out_format)
{
	FirtreeCoglTextureSamplerPrivate *p = GET_PRIVATE(self);

	CoglHandle texture =
	    firtree_cogl_texture_sampler_get_cogl_texture(self);
	if (!texture) {
		g_debug
		    ("Internal firtree_cogl_texture_sampler_get_data() function "
		     "called on invalid sampler.");
		return 0;
	}

	if (p->cached_data) {
		if (data) {
			*data = p->cached_data;
		}
		if (rowstride) {
			*rowstride = p->cached_data_stride;
		}
		if (out_format) {
			*out_format = p->cached_data_format;
		}
		return p->cached_data_size;
	}

	guint width = cogl_texture_get_width(texture);
	guint stride = cogl_texture_get_rowstride(texture);
	CoglPixelFormat format = cogl_texture_get_format(texture);

	FirtreeBufferFormat firtree_format = FIRTREE_FORMAT_LAST;

	/* FIXME: This implicitly assumes a little-endian machine. */

	switch (format) {
	case COGL_PIXEL_FORMAT_BGRA_8888:
		firtree_format = FIRTREE_FORMAT_BGRA32;
		break;
	case COGL_PIXEL_FORMAT_BGRA_8888_PRE:
		firtree_format = FIRTREE_FORMAT_BGRA32_PREMULTIPLIED;
		break;
	case COGL_PIXEL_FORMAT_RGBA_8888:
		firtree_format = FIRTREE_FORMAT_RGBA32;
		break;
	case COGL_PIXEL_FORMAT_RGBA_8888_PRE:
		firtree_format = FIRTREE_FORMAT_RGBA32_PREMULTIPLIED;
		break;
	case COGL_PIXEL_FORMAT_RGB_888:
		firtree_format = FIRTREE_FORMAT_RGB24;
		break;
	case COGL_PIXEL_FORMAT_BGR_888:
		firtree_format = FIRTREE_FORMAT_BGR24;
		break;
	default:
		g_debug
		    ("Warning, converting Cogl texture format from %i. May be slow.",
		     format);
		format = COGL_PIXEL_FORMAT_BGRA_8888;
		firtree_format = FIRTREE_FORMAT_ARGB32;
		stride = 4 * width;
		break;
	}

	guint data_size = cogl_texture_get_data(texture,
						format, stride, NULL);
	g_assert(data_size != 0);

	if (rowstride) {
		*rowstride = stride;
	}

	if (out_format) {
		*out_format = firtree_format;
	}

	/* If no output buffer was specified, shortcut the actual copy. */
	if (!data) {
		return data_size;
	}

	p->cached_data_size = data_size;
	p->cached_data_stride = stride;
	p->cached_data_format = firtree_format;
	p->cached_data = (guchar *) g_slice_alloc(p->cached_data_size);
	cogl_texture_get_data(texture, format, stride, p->cached_data);

	*data = p->cached_data;

	return p->cached_data_size;
}

llvm::Function *
firtree_cogl_texture_sampler_get_sample_function(FirtreeSampler * self)
{
	FirtreeCoglTextureSamplerPrivate *p = GET_PRIVATE(self);
	if (p->cached_function) {
		return p->cached_function;
	}

	return
	    _firtree_cogl_texture_sampler_create_sample_function
	    (FIRTREE_COGL_TEXTURE_SAMPLER(self));
}

/* Create the sampler function */
llvm::Function *
_firtree_cogl_texture_sampler_create_sample_function(FirtreeCoglTextureSampler *
						     self)
{
	FirtreeCoglTextureSamplerPrivate *p = GET_PRIVATE(self);
	CoglHandle texture =
	    firtree_cogl_texture_sampler_get_cogl_texture(self);
	if (!texture) {
		return NULL;
	}

	_firtree_cogl_texture_sampler_invalidate_llvm_cache(self);

#if FIRTREE_LLVM_AT_LEAST_2_6
	llvm::Module * m = new llvm::Module("cogl_texture",
			llvm::getGlobalContext());
#else
	llvm::Module * m = new llvm::Module("cogl_texture");
#endif

	/* declare the sample_image_buffer() function which will be implemented
	 * by the engine. */
	llvm::Function * sample_texture_func =
	    firtree_engine_create_sample_cogl_texture_prototype(m);
	g_assert(sample_texture_func);

	/* declare the sample() function which we shall implement. */
	llvm::Function * sample_func =
	    firtree_engine_create_sample_function_prototype(m);
	g_assert(sample_func);

	/* This looks dirty but is apparently valid.
	 *   See: http://www.nabble.com/Creating-Pointer-Constants-td22401381.html */
	llvm::Constant * llvm_tex_int =
	    llvm::ConstantInt::get(FIRTREE_LLVM_INT64_TY, (uint64_t) self, false);
	llvm::Value * llvm_tex =
	    llvm::ConstantExpr::getIntToPtr(llvm_tex_int,
					    llvm::PointerType::
					    getUnqual(FIRTREE_LLVM_INT8_TY));

	/* Implement the sample function. */
	llvm::BasicBlock * bb = llvm::BasicBlock::Create(FIRTREE_LLVM_CONTEXT 
			"entry", sample_func);

	std::vector < llvm::Value * >func_args;
	func_args.push_back(llvm_tex);
	func_args.push_back(sample_func->arg_begin());

	llvm::Value * ret_val = llvm::CallInst::Create(sample_texture_func,
						       func_args.begin(),
						       func_args.end(), "rv",
						       bb);

	llvm::ReturnInst::Create(FIRTREE_LLVM_CONTEXT ret_val, bb);

	p->cached_function = sample_func;
	return sample_func;
}

/* vim:sw=8:ts=8:tw=78:noet:cindent
 */
