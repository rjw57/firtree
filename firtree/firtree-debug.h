/* firtree-debug.h */

/* Firtree - A generic image processing library
 * Copyright (C) 2009 Rich Wareham <richwareham@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License verstion as published
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

#ifndef _FIRTREE_DEBUG
#define _FIRTREE_DEBUG

#include <glib-object.h>

#include "firtree-kernel.h"
#include "firtree-sampler.h"

/**
 * SECTION:firtree-debug
 * @short_description: Debugging utility functions.
 * @include: firtree/firtree-debug.h
 *
 * These are a selection of functions which are useful for debugging the
 * Firtree library.
 */

G_BEGIN_DECLS

/**
 * firtree_debug_dump_kernel_function:
 * @kernel: A FirtreeKernel.
 *
 * Dump the compiled LLVM associated with @kernel into a string and
 * return it. The string must be released via g_string_free() after use.
 *
 * If the kernel is invalid, or there is no LLVM function, this returns
 * NULL.
 *
 * Returns: NULL or a GString.
 */
GString*
firtree_debug_dump_kernel_function(FirtreeKernel* kernel);

/**
 * firtree_debug_dump_sampler_function:
 * @sampler: A FirtreeKernel.
 *
 * Dump the compiled LLVM associated with @sampler into a string and
 * return it. The string must be released via g_string_free() after use.
 *
 * If the sampler is invalid, or there is no LLVM function, this returns
 * NULL.
 *
 * Returns: NULL or a GString.
 */
GString*
firtree_debug_dump_sampler_function(FirtreeSampler* sampler);

G_END_DECLS

#endif /* _FIRTREE_DEBUG */

/* vim:sw=4:ts=4:et:cindent
 */
