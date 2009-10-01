/* firtree-kernel-sampler.h */

/* Firtree - A generic image processing library
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

#ifndef __FIRTREE_KERNEL_SAMPLER_H__
#define __FIRTREE_KERNEL_SAMPLER_H__

#include <glib-object.h>

#include <firtree/firtree-sampler.h>

G_BEGIN_DECLS

#define FIRTREE_TYPE_KERNEL_SAMPLER 		firtree_kernel_sampler_get_type()
#define FIRTREE_KERNEL_SAMPLER(obj) 		(G_TYPE_CHECK_INSTANCE_CAST ((obj), FIRTREE_TYPE_KERNEL_SAMPLER, FirtreeKernelSampler))
#define FIRTREE_KERNEL_SAMPLER_CLASS(klass) 	(G_TYPE_CHECK_CLASS_CAST ((klass), FIRTREE_TYPE_KERNEL_SAMPLER, FirtreeKernelSamplerClass))
#define FIRTREE_IS_KERNEL_SAMPLER(obj) 		(G_TYPE_CHECK_INSTANCE_TYPE ((obj), FIRTREE_TYPE_KERNEL_SAMPLER))
#define FIRTREE_IS_KERNEL_SAMPLER_CLASS(klass) 	(G_TYPE_CHECK_CLASS_TYPE ((klass), FIRTREE_TYPE_KERNEL_SAMPLER))
#define FIRTREE_KERNEL_SAMPLER_GET_CLASS(obj) 	(G_TYPE_INSTANCE_GET_CLASS ((obj), FIRTREE_TYPE_KERNEL_SAMPLER, FirtreeKernelSamplerClass))

typedef struct 	_FirtreeKernelSampler 		FirtreeKernelSampler;
typedef struct 	_FirtreeKernelSamplerClass 	FirtreeKernelSamplerClass;
typedef struct 	_FirtreeKernelSamplerPrivate 	FirtreeKernelSamplerPrivate;

struct _FirtreeKernelSampler 
{
	FirtreeSampler		 parent;
};

struct _FirtreeKernelSamplerClass
{
	FirtreeSamplerClass	 parent_class;
};

GType			 firtree_kernel_sampler_get_type	(void);

FirtreeKernelSampler	*firtree_kernel_sampler_new		(void);

void			 firtree_kernel_sampler_set_kernel	(FirtreeKernelSampler 	*self,
								 FirtreeKernel 		*kernel);

FirtreeKernel 		*firtree_kernel_sampler_get_kernel	(FirtreeKernelSampler 	*self);

G_END_DECLS

#endif				/* __FIRTREE_KERNEL_SAMPLER_H__ */

/* vim:sw=8:ts=8:noet:cindent
 */
