/* firtree-debug.cc */

/* Firtree - A generic image processing library
 * Copyright (C) 2009 Rich Wareham <richwareham@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License verstion as published
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

#include "firtree-debug.h"

#include "internal/firtree-kernel-intl.hh"
#include "internal/firtree-sampler-intl.hh"

#include <sstream>

GString*
_firtree_debug_dump_module(llvm::Module* m)
{
    std::ostringstream out;

    m->print(out, NULL);

    /* This is non-optimal, invlving a copy as it does but
     * production code shouldn't be using this function anyway. */
    return g_string_new(out.str().c_str());
}

GString*
firtree_debug_dump_kernel_function(FirtreeKernel* kernel)
{
    if(!FIRTREE_IS_KERNEL(kernel)) {
        return NULL;
    }

    llvm::Function* f = firtree_kernel_get_function(kernel);
    if(!f) {
        return NULL;
    }

    llvm::Module* m = f->getParent();
    return _firtree_debug_dump_module(m);
}

GString*
firtree_debug_dump_sampler_function(FirtreeSampler* sampler)
{
    if(!FIRTREE_IS_SAMPLER(sampler)) {
        return NULL;
    }

    llvm::Function* f = firtree_sampler_get_function(sampler);
    if(!f) {
        return NULL;
    }

    llvm::Module* m = f->getParent();
    return _firtree_debug_dump_module(m);
}

/* vim:sw=4:ts=4:et:cindent
 */
