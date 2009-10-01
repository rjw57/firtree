/* firtree-kernel.h */

/* Firtree - A generic image processing library
 * Copyright (C) 2009 Rich Wareham <richwareham@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as published
 * by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program; if not, write to the Free Software Foundation, Inc., 51
 * Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#ifndef __FIRTREE_KERNEL_H__
#define __FIRTREE_KERNEL_H__

#include <glib-object.h>

G_BEGIN_DECLS

#define FIRTREE_TYPE_KERNEL firtree_kernel_get_type()
#define FIRTREE_KERNEL(obj) \
    (G_TYPE_CHECK_INSTANCE_CAST ((obj), FIRTREE_TYPE_KERNEL, FirtreeKernel))
#define FIRTREE_KERNEL_CLASS(klass) \
    (G_TYPE_CHECK_CLASS_CAST ((klass), FIRTREE_TYPE_KERNEL, FirtreeKernelClass))
#define FIRTREE_IS_KERNEL(obj) \
    (G_TYPE_CHECK_INSTANCE_TYPE ((obj), FIRTREE_TYPE_KERNEL))
#define FIRTREE_IS_KERNEL_CLASS(klass) \
    (G_TYPE_CHECK_CLASS_TYPE ((klass), FIRTREE_TYPE_KERNEL))
#define FIRTREE_KERNEL_GET_CLASS(obj) \
    (G_TYPE_INSTANCE_GET_CLASS ((obj), FIRTREE_TYPE_KERNEL, FirtreeKernelClass))

typedef struct 	_FirtreeKernel 			FirtreeKernel;
typedef struct 	_FirtreeKernelClass 		FirtreeKernelClass;
typedef struct 	_FirtreeKernelPrivate 		FirtreeKernelPrivate;
typedef struct 	_FirtreeKernelArgumentSpec	FirtreeKernelArgumentSpec;

struct _FirtreeKernelArgumentSpec
{
	GQuark		  name_quark;
	GType		  type;
	gboolean	  is_static;
};

typedef enum
{
	FIRTREE_KERNEL_TARGET_RENDER,
	FIRTREE_KERNEL_TARGET_REDUCE,

	FIRTREE_KERNEL_TARGET_INVALID = -1
} FirtreeKernelTarget;

struct _FirtreeKernel 
{
	GObject		  parent;
};

struct _FirtreeKernelClass
{
	GObjectClass	  parent_class;

	void 		(*argument_changed) 	(FirtreeKernel 	*kernel, 
						 gchar 		*arg_name);

	void 		(*module_changed) 	(FirtreeKernel 	*kernel);

	void 		(*contents_changed) 	(FirtreeKernel 	*kernel);
};

GType 		  firtree_kernel_get_type		(void);

FirtreeKernel 	 *firtree_kernel_new			(void);

gboolean	  firtree_kernel_compile_from_source	(FirtreeKernel 	 *self,
							 gchar 		**lines,
							 gint		  n_lines,
							 gchar 		 *kernel_name);

gchar		**firtree_kernel_get_compile_log	(FirtreeKernel 	 *self,
					 		 guint 		 *n_log_lines);

gboolean	  firtree_kernel_get_compile_status	(FirtreeKernel	 *self);

GQuark		 *firtree_kernel_list_arguments		(FirtreeKernel	 *self,
							 guint		 *n_arguments);

gboolean 	  firtree_kernel_has_argument_named	(FirtreeKernel	 *self,
							 gchar		 *arg_name);

FirtreeKernelArgumentSpec 	*firtree_kernel_get_argument_spec
							(FirtreeKernel	 *self,
							 GQuark		  arg_name);

GValue		 *firtree_kernel_get_argument_value	(FirtreeKernel	 *self,
							 GQuark		  arg_name);

gboolean 	  firtree_kernel_set_argument_value	(FirtreeKernel	 *self,
							 GQuark		  arg_name,
							 GValue		 *value);

void		  firtree_kernel_argument_changed	(FirtreeKernel	 *self,
							 GQuark		  arg_name);

gboolean	  firtree_kernel_is_valid		(FirtreeKernel	 *self);

void		  firtree_kernel_module_changed		(FirtreeKernel	 *self);

void		  firtree_kernel_contents_changed	(FirtreeKernel	 *self);

GType		  firtree_kernel_get_return_type	(FirtreeKernel	 *self);

FirtreeKernelTarget  	firtree_kernel_get_target	(FirtreeKernel	 *self);

G_END_DECLS

#endif				/* __FIRTREE_KERNEL_H__ */

/* vim:sw=8:ts=8:noet:cindent
 */
