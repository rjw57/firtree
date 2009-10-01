/* firtree-debug.h */

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

#ifndef __FIRTREE_DEBUG_H__
#define __FIRTREE_DEBUG_H__

#include <glib-object.h>

#include <firtree/firtree-kernel.h>
#include <firtree/firtree-sampler.h>

G_BEGIN_DECLS

GString 	*firtree_debug_dump_kernel_function	(FirtreeKernel 	*kernel);

GString 	*firtree_debug_dump_sampler_function	(FirtreeSampler	*sampler);

G_END_DECLS

#endif				/* __FIRTREE_DEBUG_H__ */
/* vim:sw=8:ts=8:noet:cindent
 */
