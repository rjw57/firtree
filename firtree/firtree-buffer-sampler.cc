/* firtree-buffer-sampler.cc */

/* Firtree - A generic buffer processing library
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

#include "internal/firtree-engine-intl.hh"
#include "internal/firtree-sampler-intl.hh"
#include "firtree-buffer-sampler.h"

#include <string.h>

/**
 * SECTION:firtree-buffer-sampler
 * @short_description: A FirtreeSampler which can sample from a buffer in memory.
 * @include: firtree/firtree-buffer-sampler.h
 *
 * A FirtreeBufferSampler is a FirtreeSampler which knows how to sample from
 * a buffer in memory. 
 */

/**
 * FirtreeBufferSampler:
 * 
 * A FirtreeBufferSampler is a FirtreeSampler which knows how to sample from
 * a buffer in memory. 
 */

/**
 * FirtreeBufferSamplerPrivate:
 * 
 * Internal data stored by a FirtreeBufferSampler instance.
 */

G_DEFINE_TYPE(FirtreeBufferSampler, firtree_buffer_sampler,
	      FIRTREE_TYPE_SAMPLER)

#define GET_PRIVATE(o) 		(G_TYPE_INSTANCE_GET_PRIVATE ((o), FIRTREE_TYPE_BUFFER_SAMPLER, FirtreeBufferSamplerPrivate))

struct _FirtreeBufferSamplerPrivate 
{
	gboolean do_interp;
	 llvm::Function * cached_function;
	gpointer cached_buffer;
	guint cached_buffer_len;
	guint cached_width;
	guint cached_height;
	guint cached_stride;
	FirtreeBufferFormat cached_format;
	gboolean free_cached_buffer;
};

llvm::Function *
firtree_buffer_sampler_get_sample_function(FirtreeSampler * self);

llvm::Function *
_firtree_buffer_sampler_create_sample_function(FirtreeBufferSampler * self);

FirtreeVec4 firtree_buffer_sampler_get_extent(FirtreeSampler * self)
{
	FirtreeBufferSamplerPrivate *p = GET_PRIVATE(self);
	if (!p || !p->cached_buffer) {
		/* return 'NULL' extent */
		FirtreeVec4 rv = { 0, 0, 0, 0 };
		return rv;
	}

	FirtreeVec4 rv = { 0, 0, p->cached_width, p->cached_height };
	return rv;
}

/* invalidate (and release) any cached LLVM modules/functions. This
 * will cause them to be re-generated when ..._get_function() is next
 * called. */
static void
_firtree_buffer_sampler_invalidate_llvm_cache(FirtreeBufferSampler * self)
{
	FirtreeBufferSamplerPrivate *p = GET_PRIVATE(self);
	if (p && p->cached_function) {
		delete p->cached_function->getParent();
		p->cached_function = NULL;
	}
	firtree_sampler_module_changed(FIRTREE_SAMPLER(self));
	firtree_sampler_contents_changed(FIRTREE_SAMPLER(self));
}

static void firtree_buffer_sampler_dispose(GObject * object)
{
	FirtreeBufferSamplerPrivate *p = GET_PRIVATE(object);

	/* dispose of any LLVM modules we might have. */
	if (p && p->cached_function) {
		delete p->cached_function->getParent();
		p->cached_function = NULL;
	}

	/* dispose of any cached buffer */
	if (p->cached_buffer) {
		if (p->free_cached_buffer) {
			g_slice_free1(p->cached_buffer_len, p->cached_buffer);
		}
		p->cached_buffer = NULL;
		p->cached_buffer_len = 0;
	}

	G_OBJECT_CLASS(firtree_buffer_sampler_parent_class)->dispose(object);
}

static FirtreeSamplerIntlVTable _firtree_buffer_sampler_class_vtable;

static void firtree_buffer_sampler_class_init(FirtreeBufferSamplerClass * klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS(klass);

	g_type_class_add_private(klass, sizeof(FirtreeBufferSamplerPrivate));

	object_class->dispose = firtree_buffer_sampler_dispose;

	/* override the sampler virtual functions with our own */
	FirtreeSamplerClass *sampler_class = FIRTREE_SAMPLER_CLASS(klass);

	sampler_class->get_extent = firtree_buffer_sampler_get_extent;

	sampler_class->intl_vtable = &_firtree_buffer_sampler_class_vtable;

	sampler_class->intl_vtable->get_sample_function =
	    firtree_buffer_sampler_get_sample_function;
}

static void firtree_buffer_sampler_init(FirtreeBufferSampler * self)
{
	FirtreeBufferSamplerPrivate *p = GET_PRIVATE(self);
	p->do_interp = FALSE;
	p->cached_function = NULL;
	p->cached_buffer = NULL;
	p->free_cached_buffer = FALSE;
	p->cached_buffer_len = 0;
}

/**
 * firtree_buffer_sampler_new:
 *
 * Construct an uninitialised buffer sampler. Until this has been associated
 * with a buffer via firtree_buffer_sampler_new_set_buffer(), the sampler is
 * invalid.
 *
 * Returns: A new FirtreeBufferSampler.
 */
FirtreeBufferSampler *firtree_buffer_sampler_new(void)
{
	return (FirtreeBufferSampler *)
	    g_object_new(FIRTREE_TYPE_BUFFER_SAMPLER, NULL);
}

/**
 * firtree_buffer_sampler_get_do_interpolation:
 * @self:  A FirtreeBufferSampler.
 *
 * Get a flag which indicates if the sampler should attempt linear interpolation
 * of the pixel values.
 *
 * Returns: A flag indicating if interpolation is performed.
 */
gboolean
firtree_buffer_sampler_get_do_interpolation(FirtreeBufferSampler * self)
{
	FirtreeBufferSamplerPrivate *p = GET_PRIVATE(self);
	return p->do_interp;
}

/**
 * firtree_buffer_sampler_set_do_interpolation:
 * @self:  A FirtreeBufferSampler.
 * @do_interp: A flag indicating if interpolation is performed.
 *
 * Set a flag which indicates if the sampler should attempt linear interpolation
 * of the pixel values.
 */
void
firtree_buffer_sampler_set_do_interpolation(FirtreeBufferSampler * self,
					    gboolean do_interp)
{
	FirtreeBufferSamplerPrivate *p = GET_PRIVATE(self);
	if (do_interp != p->do_interp) {
		p->do_interp = do_interp;
		_firtree_buffer_sampler_invalidate_llvm_cache(self);
	}
}

/**
 * firtree_buffer_sampler_set_buffer:
 * @self: A FirtreeBufferSampler
 * @buffer: The location of the buffer in memory or NULL to unset the buffer.
 * @width: The buffer width in pixels.
 * @height: The buffer height in rows.
 * @stride: The size of one row in bytes.
 * @format: The format of the buffer.
 *
 * Associate an in-memory buffer with this sampler. The buffer must be
 * valid for the lifetime of the sampler or until 
 * firtree_buffer_sampler_set_buffer() is called again. 
 */
void
firtree_buffer_sampler_set_buffer(FirtreeBufferSampler * self,
				  gpointer buffer, guint width, guint height,
				  guint stride, FirtreeBufferFormat format)
{
	FirtreeBufferSamplerPrivate *p = GET_PRIVATE(self);

	guint required_size = height * stride;

	if ((format == FIRTREE_FORMAT_I420_FOURCC) ||
	    (format == FIRTREE_FORMAT_YV12_FOURCC)) {
		required_size += (height * stride) >> 1;
	}

	if (!p->cached_buffer || (required_size != p->cached_buffer_len)) {
		/* dispose of any cached buffer */
		if (p->cached_buffer) {
			if (p->free_cached_buffer) {
				g_slice_free1(p->cached_buffer_len,
					      p->cached_buffer);
			}
			p->cached_buffer = NULL;
			p->cached_buffer_len = 0;
		}

		p->cached_buffer_len = required_size;
		p->cached_buffer = g_slice_alloc(required_size);

		_firtree_buffer_sampler_invalidate_llvm_cache(self);
	}

	/* Also invalidate if the width/height/stride/format has changed. */
	if ((p->cached_width != width) || (p->cached_height != height) ||
	    (p->cached_stride != stride) || (p->cached_format != format)) {
		_firtree_buffer_sampler_invalidate_llvm_cache(self);
	}

	p->cached_width = width;
	p->cached_height = height;
	p->cached_stride = stride;
	p->cached_format = format;
	p->free_cached_buffer = TRUE;

	/* copy the data */
	memcpy(p->cached_buffer, buffer, required_size);
}

/**
 * firtree_buffer_sampler_set_buffer_no_copy:
 * @self: A FirtreeBufferSampler
 * @buffer: The location of the buffer in memory or NULL to unset the buffer.
 * @width: The buffer width in pixels.
 * @height: The buffer height in rows.
 * @stride: The size of one row in bytes.
 * @format: The format of the buffer.
 *
 * Associate an in-memory buffer with this sampler like
 * firtree_buffer_sampler_set_buffer() except that no implicit copy is made.
 * The buffer must be valid for the lifetime of the sampler or until
 * firtree_buffer_sampler_set_buffer() or
 * firtree_buffer_sampler_set_buffer_no_copy()  is called again. 
 */
void
firtree_buffer_sampler_set_buffer_no_copy(FirtreeBufferSampler * self,
					  gpointer buffer, guint width,
					  guint height, guint stride,
					  FirtreeBufferFormat format)
{
	FirtreeBufferSamplerPrivate *p = GET_PRIVATE(self);

	guint required_size = height * stride;

	if ((format == FIRTREE_FORMAT_I420_FOURCC) ||
	    (format == FIRTREE_FORMAT_YV12_FOURCC)) {
		required_size += (height * stride) >> 1;
	}

	if (p->cached_buffer != buffer) {
		if (p->cached_buffer) {
			if (p->free_cached_buffer) {
				g_slice_free1(p->cached_buffer_len,
					      p->cached_buffer);
			}
			p->cached_buffer = NULL;
			p->cached_buffer_len = 0;
		}

		_firtree_buffer_sampler_invalidate_llvm_cache(self);
	}

	/* Also invalidate if the width/height/stride/format has changed. */
	if ((p->cached_width != width) || (p->cached_height != height) ||
	    (p->cached_stride != stride) || (p->cached_format != format)) {
		_firtree_buffer_sampler_invalidate_llvm_cache(self);
	}

	p->cached_buffer = buffer;
	p->cached_buffer_len = required_size;
	p->cached_width = width;
	p->cached_height = height;
	p->cached_stride = stride;
	p->cached_format = format;
	p->free_cached_buffer = FALSE;
}

/**
 * firtree_buffer_sampler_unset_buffer:
 * @self: A FirtreeBufferSampler
 *
 * Equivalent to calling firtree_buffer_sampler_set_buffer() with
 * NULL as the passed buffer.
 */
void firtree_buffer_sampler_unset_buffer(FirtreeBufferSampler * self)
{
	firtree_buffer_sampler_set_buffer(self, NULL, 0, 0, 0,
					  FIRTREE_FORMAT_LAST);
}

llvm::Function *
firtree_buffer_sampler_get_sample_function(FirtreeSampler * self)
{
	FirtreeBufferSamplerPrivate *p = GET_PRIVATE(self);

	if (p->cached_function) {
		return p->cached_function;
	}

	return
	    _firtree_buffer_sampler_create_sample_function
	    (FIRTREE_BUFFER_SAMPLER(self));
}

/* Create the sampler function */
llvm::Function *
_firtree_buffer_sampler_create_sample_function(FirtreeBufferSampler * self)
{
	FirtreeBufferSamplerPrivate *p = GET_PRIVATE(self);
	if (!p->cached_buffer) {
		return NULL;
	}

	unsigned char *data = (unsigned char *)(p->cached_buffer);

	/* Work out the width, height, stride, etc of the buffer. */
	int width = p->cached_width;
	int height = p->cached_height;
	int stride = p->cached_stride;
	FirtreeBufferFormat firtree_format = p->cached_format;

	_firtree_buffer_sampler_invalidate_llvm_cache(self);

	llvm::Module * m = new llvm::Module("buffer");

	/* declare the sample_image_buffer() function which will be implemented
	 * by the engine. */
	llvm::Function * sample_buffer_func =
	    firtree_engine_create_sample_image_buffer_prototype(m,
								p->do_interp);
	g_assert(sample_buffer_func);

	/* declare the sample() function which we shall implement. */
	llvm::Function * sample_func =
	    firtree_engine_create_sample_function_prototype(m);
	g_assert(sample_func);

	llvm::Value * llvm_width = llvm::ConstantInt::get(llvm::Type::Int32Ty,
							  (uint64_t) width,
							  false);
	llvm::Value * llvm_height =
	    llvm::ConstantInt::get(llvm::Type::Int32Ty, (uint64_t) height,
				   false);
	llvm::Value * llvm_stride =
	    llvm::ConstantInt::get(llvm::Type::Int32Ty, (uint64_t) stride,
				   false);
	llvm::Value * llvm_format =
	    llvm::ConstantInt::get(llvm::Type::Int32Ty,
				   (uint64_t) firtree_format, false);

	/* This looks dirty but is apparently valid.
	 *   See: http://www.nabble.com/Creating-Pointer-Constants-td22401381.html */
	llvm::Constant * llvm_data_int =
	    llvm::ConstantInt::get(llvm::Type::Int64Ty, (uint64_t) data, false);
	llvm::Value * llvm_data =
	    llvm::ConstantExpr::getIntToPtr(llvm_data_int,
					    llvm::PointerType::
					    getUnqual(llvm::Type::Int8Ty));

	/* Implement the sample function. */
	llvm::BasicBlock * bb = llvm::BasicBlock::Create("entry", sample_func);

	std::vector < llvm::Value * >func_args;
	func_args.push_back(llvm_data);
	func_args.push_back(llvm_format);
	func_args.push_back(llvm_width);
	func_args.push_back(llvm_height);
	func_args.push_back(llvm_stride);
	func_args.push_back(sample_func->arg_begin());

	llvm::Value * ret_val = llvm::CallInst::Create(sample_buffer_func,
						       func_args.begin(),
						       func_args.end(), "rv",
						       bb);

	llvm::ReturnInst::Create(ret_val, bb);

	p->cached_function = sample_func;
	return sample_func;
}

/* vim:sw=8:ts=8:tw=78:noet:cindent
 */
