/* firtree-cpu-jit.h */

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

#ifndef _FIRTREE_CPU_JIT
#define _FIRTREE_CPU_JIT

#include <glib-object.h>

#include <firtree/firtree.h>
#include <firtree/firtree-sampler.h>

#include <string>

namespace llvm {
    class Function;
}

/**
 * SECTION:firtree-cpu-jit
 * @short_description: A CPU-based JIT for LLVM functions
 * @include: firtree/engines/cpu/firtree-cpu-jit.h
 *
 * A CPU JIT for LLVM functions. This JIT caches the JIT-ed function so that
 * it can be called efficiently.
 */

G_BEGIN_DECLS

#define FIRTREE_TYPE_CPU_JIT firtree_cpu_jit_get_type()

#define FIRTREE_CPU_JIT(obj) \
    (G_TYPE_CHECK_INSTANCE_CAST ((obj), FIRTREE_TYPE_CPU_JIT, FirtreeCpuJit))

#define FIRTREE_CPU_JIT_CLASS(klass) \
    (G_TYPE_CHECK_CLASS_CAST ((klass), FIRTREE_TYPE_CPU_JIT, FirtreeCpuJitClass))

#define FIRTREE_IS_CPU_JIT(obj) \
    (G_TYPE_CHECK_INSTANCE_TYPE ((obj), FIRTREE_TYPE_CPU_JIT))

#define FIRTREE_IS_CPU_JIT_CLASS(klass) \
    (G_TYPE_CHECK_CLASS_TYPE ((klass), FIRTREE_TYPE_CPU_JIT))

#define FIRTREE_CPU_JIT_GET_CLASS(obj) \
    (G_TYPE_INSTANCE_GET_CLASS ((obj), FIRTREE_TYPE_CPU_JIT, FirtreeCpuJitClass))

/**
 * FirtreeCpuJit:
 * @parent: The parent GObject.
 *
 * A structure representing a FirtreeCpuJit object.
 */
typedef struct {
    GObject parent;
} FirtreeCpuJit;

typedef struct {
    GObjectClass parent_class;
} FirtreeCpuJitClass;

GType firtree_cpu_jit_get_type (void);

typedef void* (*FirtreeCpuJitLazyFunctionCreatorFunc) (const std::string& name);

typedef void (*FirtreeCpuJitRenderFunc) (unsigned char* buffer,
    unsigned int row_width, unsigned int num_rows,
    unsigned int row_stride, float* extents);

typedef void (*FirtreeCpuJitReduceFunc) (gpointer output,
    unsigned int row_width, unsigned int num_rows,
    float* extents);

/**
 * firtree_cpu_jit_new:
 *
 * Construct an uninitialised JIT.
 *
 * Returns: A new FirtreeCpuJit.
 */
FirtreeCpuJit* 
firtree_cpu_jit_new (void);

/**
 * firtree_cpu_jit_get_render_function_for_sampler:
 * 
 * Compile the sampler function of the passed sampler and return a pointer to
 * a renderer.
 */
FirtreeCpuJitRenderFunc
firtree_cpu_jit_get_render_function_for_sampler(FirtreeCpuJit* self,
        FirtreeBufferFormat format,
        FirtreeSampler* sampler,
        FirtreeCpuJitLazyFunctionCreatorFunc lazy_creator_function);

/**
 * firtree_cpu_jit_get_reduce_function_for_kernel:
 * 
 * Compile the function of the passed kernel and return a pointer to
 * a reduce function.
 */
FirtreeCpuJitReduceFunc
firtree_cpu_jit_get_reduce_function_for_kernel(FirtreeCpuJit* self,
        FirtreeKernel* kernel,
        FirtreeCpuJitLazyFunctionCreatorFunc lazy_creator_function);

G_END_DECLS

#endif /* _FIRTREE_CPU_JIT */

/* vim:sw=4:ts=4:et:cindent
 */
