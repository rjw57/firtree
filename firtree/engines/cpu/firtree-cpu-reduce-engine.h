/* firtree-cpu-reduce-engine.h */

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

#ifndef _FIRTREE_CPU_REDUCE_ENGINE
#define _FIRTREE_CPU_REDUCE_ENGINE

#include <glib-object.h>

#include <firtree/firtree.h>

/**
 * SECTION:firtree-cpu-reduce-engine
 * @short_description: A reduce engine which uses a CPU JIT.
 * @include: firtree/engines/cpu/firtree-cpu-reduce-engine.h
 *
 * A reduce engine which uses a CPU JIT.
 */

G_BEGIN_DECLS

#define FIRTREE_TYPE_CPU_REDUCE_ENGINE firtree_cpu_reduce_engine_get_type()

#define FIRTREE_CPU_REDUCE_ENGINE(obj) \
    (G_TYPE_CHECK_INSTANCE_CAST ((obj), FIRTREE_TYPE_CPU_REDUCE_ENGINE, FirtreeCpuReduceEngine))

#define FIRTREE_CPU_REDUCE_ENGINE_CLASS(klass) \
    (G_TYPE_CHECK_CLASS_CAST ((klass), FIRTREE_TYPE_CPU_REDUCE_ENGINE, FirtreeCpuReduceEngineClass))

#define FIRTREE_IS_CPU_REDUCE_ENGINE(obj) \
    (G_TYPE_CHECK_INSTANCE_TYPE ((obj), FIRTREE_TYPE_CPU_REDUCE_ENGINE))

#define FIRTREE_IS_CPU_REDUCE_ENGINE_CLASS(klass) \
    (G_TYPE_CHECK_CLASS_TYPE ((klass), FIRTREE_TYPE_CPU_REDUCE_ENGINE))

#define FIRTREE_CPU_REDUCE_ENGINE_GET_CLASS(obj) \
    (G_TYPE_INSTANCE_GET_CLASS ((obj), FIRTREE_TYPE_CPU_REDUCE_ENGINE, FirtreeCpuReduceEngineClass))

/**
 * FirtreeCpuReduceEngine:
 * @parent: The parent GObject.
 *
 * A structure representing a FirtreeCpuReduceEngine object.
 */
typedef struct {
    GObject parent;
} FirtreeCpuReduceEngine;

typedef struct {
    GObjectClass parent_class;
} FirtreeCpuReduceEngineClass;

GType firtree_cpu_reduce_engine_get_type (void);

/**
 * firtree_cpu_reduce_engine_new:
 *
 * Construct an uninitialised reduce engine. The engine accepts an associated
 * reduce kernel via the firtree_cpu_reduce_engine_set_kernel() call.
 *
 * Returns: A new FirtreeCpuReduceEngine.
 */
FirtreeCpuReduceEngine* 
firtree_cpu_reduce_engine_new (void);

/**
 * firtree_cpu_reduce_engine_set_kernel:
 * @self: A FirtreeCpuReduceEngine object.
 * @kernel: A FirtreeKernel to associate with the engine.
 *
 * Associate the kernel @kernel with the engine.
 */
void
firtree_cpu_reduce_engine_set_kernel (FirtreeCpuReduceEngine* self, FirtreeKernel* kernel);

/**
 * firtree_cpu_reduce_engine_get_kernel:
 * @self: A FirtreeCpuReduceEngine object.
 *
 * Retrieve the FirtreeKernel associated with the engine. 
 *
 * Returns: The kernel related with the engine.
 */
FirtreeKernel*
firtree_cpu_reduce_engine_get_kernel (FirtreeCpuReduceEngine* self);

/**
 * firtree_cpu_reduce_engine_run:
 * @self: A FirtreeCpuReduceEngine object.
 * @set: The set to append emit()-ed elements to.
 * @extents: The extents of the sampler to render.
 * @width: The width in pixels.
 * @height: The height in rows.
 *
 * Excecute the reduce engine over the specified extents with the specified
 * width and height.
 */
void
firtree_cpu_reduce_engine_run (FirtreeCpuReduceEngine* self,
        FirtreeLockFreeSet* set,
        FirtreeVec4* extents,
        guint width, guint height);

/**
 * firtree_debug_dump_cpu_reduce_engine_function:
 * @engine: A FirtreeCpuReduceEngine.
 *
 * Dump the compiled LLVM associated with @engine into a string and
 * return it. The string must be released via g_string_free() after use.
 *
 * If the engine is invalid, or there is no LLVM function, this returns
 * NULL.
 *
 * Returns: NULL or a GString.
 */
GString*
firtree_debug_dump_cpu_reduce_engine_function(FirtreeCpuReduceEngine* engine);

/**
 * firtree_debug_dump_cpu_reduce_engine_asm:
 * @engine: A FirtreeCpuReduceEngine.
 *
 * Dump the target-specific assembler associated with @engine into a string and
 * return it. The string must be released via g_string_free() after use.
 *
 * If the engine is invalid, or there is no LLVM function, this returns NULL.
 *
 * Returns: NULL or a GString.
 */
GString* firtree_debug_dump_cpu_reduce_engine_asm(FirtreeCpuReduceEngine*
        engine);

G_END_DECLS

#endif /* _FIRTREE_CPU_REDUCE_ENGINE */

/* vim:sw=4:ts=4:et:cindent
 */
