/* firtree-kernel-intl.hpp */

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

#ifndef _FIRTREE_KERNEL_INTL
#define _FIRTREE_KERNEL_INTL

#include "../firtree-kernel.h"

#include <llvm/Function.h>

G_BEGIN_DECLS

/**
 * SECTION:firtree-kernel-intl
 * @short_description: Internal kernel interface
 * @include: firtree/internal/firtree-kernel-intl.hh
 *
 * The private interface for kernels exists of a function which returns
 * a LLVM module associated with the kernel.
 */

/**
 * firtree_kernel_get_function:
 * @self: A FirtreeKernel instance.
 *
 * Get a pointer to the LLVM function resulting from the last call to
 * firtree_kernel_compile_from_source(). If the kernel has not been
 * successfully compiled, NULL is returned.
 *
 * Ownership of the function remains with the kernel. Callers should clone it
 * if they wish to maintain a long-lived version.
 *
 * Returns: NULL or an LLVM function.
 */
llvm::Function*
firtree_kernel_get_function(FirtreeKernel* self);

/**
 * firtree_kernel_create_overall_function:
 * @self: A FirtreeKernel instance.
 *
 * Get a pointer to the LLVM function resulting from the last call to
 * firtree_kernel_compile_from_source() appropriately linked into dependent samplers
 * and with static parameters interpolated in.
 *
 * This is a heavyweight function. Callers should cache the return value if possible.
 *
 * Ownership of the function passes to the caller. It should be deleteed after use along
 * with the module by calling delete function->getParent().
 *
 * Returns: NULL or an LLVM function.
 */
llvm::Function*
firtree_kernel_create_overall_function(FirtreeKernel* self);

G_END_DECLS

#endif /* _FIRTREE_KERNEL_INTL */

/* vim:sw=4:ts=4:et:cindent
 */
