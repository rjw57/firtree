/* firtree-cpu-reduce-engine.cc */

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

#define __STDC_LIMIT_MACROS
#define __STDC_CONSTANT_MACROS

#include <firtree/firtree-debug.h>
#include <firtree/firtree-lock-free-set.h>
#include "firtree-cpu-reduce-engine.h"
#include "firtree-cpu-jit.hh"
#include "firtree-cpu-common.hh"

#include <firtree/internal/firtree-engine-intl.hh>
#include <firtree/internal/firtree-kernel-intl.hh>

#include <common/system-info.h>
#include <common/threading.h>

#include <sstream>

G_DEFINE_TYPE (FirtreeCpuReduceEngine, firtree_cpu_reduce_engine, G_TYPE_OBJECT)

#define GET_PRIVATE(o) \
    (G_TYPE_INSTANCE_GET_PRIVATE ((o), FIRTREE_TYPE_CPU_REDUCE_ENGINE, FirtreeCpuReduceEnginePrivate))

typedef struct _FirtreeCpuReduceEnginePrivate FirtreeCpuReduceEnginePrivate;

struct _FirtreeCpuReduceEnginePrivate {
    FirtreeKernel*              kernel;
    gulong                      kernel_handler_id;
    FirtreeCpuJit*              jit;

    FirtreeCpuJitReduceFunc     cached_reduce_func;
};

struct FirtreeCpuReduceEngineRequest {
    FirtreeCpuJitReduceFunc     func;
    gpointer                    output;
    unsigned int                row_width;
    unsigned int                num_rows;
    float                       extents[4];
};

/* invalidate (and release) any cached LLVM modules/functions. This
 * will cause them to be re-generated when ..._get_function() is next
 * called. */
static void
_firtree_cpu_reduce_engine_invalidate_llvm_cache(FirtreeCpuReduceEngine* self)
{
    FirtreeCpuReduceEnginePrivate* p = GET_PRIVATE(self);
    p->cached_reduce_func = NULL;
}

static void
firtree_cpu_reduce_engine_dispose (GObject *object)
{
    G_OBJECT_CLASS (firtree_cpu_reduce_engine_parent_class)->dispose (object);
    FirtreeCpuReduceEngine* cpu_reduce_engine = FIRTREE_CPU_REDUCE_ENGINE(object);
    FirtreeCpuReduceEnginePrivate* p = GET_PRIVATE(cpu_reduce_engine); 

    firtree_cpu_reduce_engine_set_kernel(cpu_reduce_engine, NULL);
    _firtree_cpu_reduce_engine_invalidate_llvm_cache(cpu_reduce_engine);

    if(p && p->jit) {
        g_object_unref(p->jit);
        p->jit = NULL;
    }

    p->cached_reduce_func = NULL;
}

static void
firtree_cpu_reduce_engine_class_init (FirtreeCpuReduceEngineClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS (klass);
    g_type_class_add_private (klass, sizeof (FirtreeCpuReduceEnginePrivate));
    object_class->dispose = firtree_cpu_reduce_engine_dispose;
}

static void
firtree_cpu_reduce_engine_init (FirtreeCpuReduceEngine *self)
{
    FirtreeCpuReduceEnginePrivate* p = GET_PRIVATE(self); 
    p->kernel = NULL;
    p->jit = firtree_cpu_jit_new();
    p->cached_reduce_func = NULL;
}

FirtreeCpuReduceEngine*
firtree_cpu_reduce_engine_new (void)
{
    return (FirtreeCpuReduceEngine*)
        g_object_new (FIRTREE_TYPE_CPU_REDUCE_ENGINE, NULL);
}

static void
firtree_cpu_reduce_engine_kernel_module_changed_cb(gpointer kernel,
        gpointer data)
{
    FirtreeCpuReduceEngine* self = FIRTREE_CPU_REDUCE_ENGINE(data);
    _firtree_cpu_reduce_engine_invalidate_llvm_cache(self);
}

void
firtree_cpu_reduce_engine_set_kernel (FirtreeCpuReduceEngine* self,
        FirtreeKernel* kernel)
{
    FirtreeCpuReduceEnginePrivate* p = GET_PRIVATE(self); 

    if(p->kernel) {
        /* disconnect the original handler */
        g_signal_handler_disconnect(p->kernel, p->kernel_handler_id);

        g_object_unref(p->kernel);
        p->kernel = NULL;
    }

    if(kernel) {
        if(firtree_kernel_get_target(kernel) != FIRTREE_KERNEL_TARGET_REDUCE) {
            g_warning("Attempt to set a non-reduce kernel ignored.");
        } else {
            p->kernel = kernel;
            g_object_ref(p->kernel);

            p->kernel_handler_id = g_signal_connect(p->kernel, 
                    "module-changed",
                    G_CALLBACK(firtree_cpu_reduce_engine_kernel_module_changed_cb), self);
        }
    }

    _firtree_cpu_reduce_engine_invalidate_llvm_cache(self);
}

FirtreeKernel*
firtree_cpu_reduce_engine_get_kernel (FirtreeCpuReduceEngine* self)
{
    FirtreeCpuReduceEnginePrivate* p = GET_PRIVATE(self); 
    return p->kernel;
}

static FirtreeCpuJitReduceFunc
firtree_cpu_reduce_engine_get_reduce_engine_func(FirtreeCpuReduceEngine* self)
{
    FirtreeCpuReduceEnginePrivate* p = GET_PRIVATE(self); 

    if(!p->kernel) {
        g_warning("No kernel");
        return NULL;
    }

    if(p->cached_reduce_func) {
        return p->cached_reduce_func;
    }

    p->cached_reduce_func = firtree_cpu_jit_get_reduce_function_for_kernel(p->jit,
            p->kernel, firtree_cpu_common_lazy_function_creator);

    return p->cached_reduce_func;
}

static void
_call_reduce_func(guint slice, FirtreeCpuReduceEngineRequest* request)
{
    guint start_row = slice << 3;
    guint n_rows = MIN(start_row+8, request->num_rows) - start_row;

    float dy = request->extents[3] / (float)(request->num_rows);
    float extents[] = { 
        request->extents[0], request->extents[1] + (dy * (float)start_row),
        request->extents[2], dy * (float)n_rows };

    request->func(request->output, request->row_width, n_rows, extents);
}

static void
firtree_cpu_reduce_engine_perform_reduce(FirtreeCpuReduceEngine* self,
        FirtreeLockFreeSet* set,
        FirtreeCpuJitReduceFunc func,  
        unsigned int row_width, unsigned int num_rows,
        float* extents) 
{
    if(!func) {
        return;
    }

    FirtreeCpuReduceEngineRequest request = {
        func, 
        set,
        row_width, num_rows,
        { extents[0], extents[1], extents[2], extents[3] },
    };

    threading_apply(((num_rows+7)>>3), (ThreadingApplyFunc) _call_reduce_func, &request);
}

void
firtree_cpu_reduce_engine_run (FirtreeCpuReduceEngine* self,
        FirtreeLockFreeSet* set,
        FirtreeVec4* extents,
        guint width, guint height)
{
    FirtreeCpuJitReduceFunc reduce_func = 
        firtree_cpu_reduce_engine_get_reduce_engine_func(self);

    if(!reduce_func) {
        return;
    }

    firtree_cpu_reduce_engine_perform_reduce(self, set, reduce_func, width, height,
            (float*)extents);
}

GString*
firtree_debug_dump_cpu_reduce_engine_function(FirtreeCpuReduceEngine* self)
{
    FirtreeCpuReduceEnginePrivate* p = GET_PRIVATE(self); 

    if(!p->kernel) {
        return NULL;
    }

    llvm::Function* f = firtree_kernel_create_overall_function(p->kernel);
    if(!f) {
        return NULL;
    }
    llvm::Module* m = f->getParent();
    
    std::ostringstream out;

    m->print(out, NULL);

    delete m;

    /* This is non-optimal, invlving a copy as it does but
     * production code shouldn't be using this function anyway. */
    return g_string_new(out.str().c_str());
}

/* vim:sw=4:ts=4:et:cindent
 */
