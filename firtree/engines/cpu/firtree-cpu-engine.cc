/* firtree-cpu-engine.cc */

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

#include "firtree-cpu-engine.h"

#include <llvm/Module.h>
#include <llvm/ModuleProvider.h>
#include <llvm/Support/MemoryBuffer.h>
#include <llvm/Bitcode/ReaderWriter.h>
#include <llvm/Linker.h>
#include <llvm/ExecutionEngine/ExecutionEngine.h>
#include <llvm/ExecutionEngine/JIT.h>

#include <llvm/Analysis/Verifier.h>
#include <llvm/Analysis/LoopPass.h>
#include <llvm/Analysis/CallGraph.h>
#include <llvm/Target/TargetData.h>
#include <llvm/Transforms/Scalar.h>
#include <llvm/LinkAllPasses.h>

#include <llvm/DerivedTypes.h>
#include <llvm/Instructions.h>
#include <llvm/Constants.h>

#include <llvm/Transforms/Utils/Cloning.h>

#include <firtree/internal/firtree-sampler-intl.hh>

#define bindata _firtee_cpu_engine_render_buffer_mod
#include "render-buffer.llvm.bc.h"
#undef bindata

G_DEFINE_TYPE (FirtreeCpuEngine, firtree_cpu_engine, G_TYPE_OBJECT)

#define GET_PRIVATE(o) \
    (G_TYPE_INSTANCE_GET_PRIVATE ((o), FIRTREE_TYPE_CPU_ENGINE, FirtreeCpuEnginePrivate))

typedef void (* RenderFunc) (unsigned char* buffer,
    unsigned int width, unsigned int height,
    unsigned int row_stride, float* extents);

typedef struct _FirtreeCpuEnginePrivate FirtreeCpuEnginePrivate;

struct _FirtreeCpuEnginePrivate {
    FirtreeSampler*             sampler;
    RenderFunc                  cached_function;
    llvm::ExecutionEngine*      cached_engine;
};

/* invalidate (and release) any cached LLVM modules/functions. This
 * will cause them to be re-generated when ..._get_function() is next
 * called. */
static void
_firtree_cpu_engine_invalidate_llvm_cache(FirtreeCpuEngine* self)
{
    FirtreeCpuEnginePrivate* p = GET_PRIVATE(self);
    if(p && p->cached_engine) {
        delete p->cached_engine;
        p->cached_engine = NULL;
        p->cached_function = NULL;
    }
}

static void
firtree_cpu_engine_get_property (GObject *object, guint property_id,
        GValue *value, GParamSpec *pspec)
{
    switch (property_id) {
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    }
}

static void
firtree_cpu_engine_set_property (GObject *object, guint property_id,
        const GValue *value, GParamSpec *pspec)
{
    switch (property_id) {
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    }
}

static void
firtree_cpu_engine_dispose (GObject *object)
{
    /* FirtreeCpuEnginePrivate* p = GET_PRIVATE(object); */
    G_OBJECT_CLASS (firtree_cpu_engine_parent_class)->dispose (object);

    FirtreeCpuEngine* cpu_engine = FIRTREE_CPU_ENGINE(object);
    firtree_cpu_engine_set_sampler(cpu_engine, NULL);
    _firtree_cpu_engine_invalidate_llvm_cache(cpu_engine);
}

static void
firtree_cpu_engine_finalize (GObject *object)
{
    G_OBJECT_CLASS (firtree_cpu_engine_parent_class)->finalize (object);
}

static void
firtree_cpu_engine_class_init (FirtreeCpuEngineClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS (klass);

    g_type_class_add_private (klass, sizeof (FirtreeCpuEnginePrivate));

    object_class->get_property = firtree_cpu_engine_get_property;
    object_class->set_property = firtree_cpu_engine_set_property;
    object_class->dispose = firtree_cpu_engine_dispose;
    object_class->finalize = firtree_cpu_engine_finalize;
}

static void
firtree_cpu_engine_init (FirtreeCpuEngine *self)
{
    FirtreeCpuEnginePrivate* p = GET_PRIVATE(self); 
    p->sampler = NULL;
    p->cached_engine = NULL;
    p->cached_function = NULL;
}

FirtreeCpuEngine*
firtree_cpu_engine_new (void)
{
    return (FirtreeCpuEngine*)
        g_object_new (FIRTREE_TYPE_CPU_ENGINE, NULL);
}

void
firtree_cpu_engine_set_sampler (FirtreeCpuEngine* self,
        FirtreeSampler* sampler)
{
    FirtreeCpuEnginePrivate* p = GET_PRIVATE(self); 

    if(p->sampler) {
        g_object_unref(p->sampler);
        p->sampler = NULL;
    }

    if(sampler) {
        p->sampler = sampler;
        g_object_ref(p->sampler);
    }

    _firtree_cpu_engine_invalidate_llvm_cache(self);
}

FirtreeSampler*
firtree_cpu_engine_get_sampler (FirtreeCpuEngine* self)
{
    FirtreeCpuEnginePrivate* p = GET_PRIVATE(self); 
    return p->sampler;
}

/* optimise a llvm module by internalising all but the
 * named function and agressively inlining. */
void optimise_module(llvm::Module* m, const char* func_name)
{
	if(m == NULL)
	{
		return;
	}

    llvm::PassManager PM;

	PM.add(new llvm::TargetData(m));

    std::vector<const char*> export_list;
    export_list.push_back(func_name);

    PM.add(llvm::createInternalizePass(export_list));
	PM.add(llvm::createFunctionInliningPass(32768)); 

	PM.add(llvm::createAggressiveDCEPass()); 

	PM.run(*m);
}

RenderFunc
get_renderer(FirtreeCpuEngine* self)
{
    FirtreeCpuEnginePrivate* p = GET_PRIVATE(self); 
    if(p->cached_function) {
        return p->cached_function;
    }

    llvm::Function* sampler_function = 
        firtree_sampler_get_function(p->sampler);
    if(sampler_function == NULL) {
        g_debug("No sampler function.\n");
        return NULL;
    }

    /* create an LLVM module from the bitcode in 
     * _firtee_cpu_engine_render_buffer_mod */
    llvm::MemoryBuffer* mem_buf = 
        llvm::MemoryBuffer::getMemBuffer(
                (const char*)_firtee_cpu_engine_render_buffer_mod,
                (const char*)(_firtee_cpu_engine_render_buffer_mod + 
                    sizeof(_firtee_cpu_engine_render_buffer_mod) - 1));
    llvm::Module* m = llvm::ParseBitcodeFile(mem_buf);
    delete mem_buf;
    
    llvm::Linker* linker = new llvm::Linker("sampler", m);

    llvm::Module* sampler_mod = sampler_function->getParent();
    std::string err_str;
    bool was_error = linker->LinkInModule(sampler_mod, &err_str);
    if(was_error) {
        g_error("Error linking in sampler: %s\n", err_str.c_str());
    }

    llvm::Module* linked_module = linker->releaseModule();
    delete linker;

    llvm::Function* new_sampler_func = linked_module->getFunction(
            sampler_function->getName());

    llvm::Function* existing_sample_func = linked_module->getFunction("sample");
    if(existing_sample_func) {
        existing_sample_func->replaceAllUsesWith(new_sampler_func);
        existing_sample_func->removeFromParent();
    }

    llvm::Function* render_function = linked_module->getFunction(
            "render_buffer_uc_4");

    m = render_function->getParent();

    optimise_module(m, render_function->getName().c_str());

    /* m->dump(); */

    llvm::ModuleProvider* mp = new llvm::ExistingModuleProvider(m);

    std::string err;
    llvm::ExecutionEngine* engine = llvm::ExecutionEngine::createJIT(mp, &err);
    p->cached_engine = engine;

    if(!engine) {
        g_debug("Error JIT-ing render: %s\n", err.c_str());
        delete mp;
        return NULL;
    }

    RenderFunc render = (RenderFunc)engine->getPointerToFunction(render_function);
    g_assert(render);

    p->cached_function = render;
    return p->cached_function;
}

gboolean
firtree_cpu_engine_render_into_pixbuf (FirtreeCpuEngine* self,
        FirtreeVec4* extents, GdkPixbuf* pixbuf)
{
    FirtreeCpuEnginePrivate* p = GET_PRIVATE(self); 

    if(!extents) { return FALSE; }
    if(!pixbuf) { return FALSE; }

    guchar* pixels = gdk_pixbuf_get_pixels(pixbuf);
    guint width = gdk_pixbuf_get_width(pixbuf);
    guint height = gdk_pixbuf_get_height(pixbuf);
    guint stride = gdk_pixbuf_get_rowstride(pixbuf);
    guint channels = gdk_pixbuf_get_n_channels(pixbuf);

    if(channels != 4) {
        return FALSE;
    }

    if(p->sampler == NULL) {
        return FALSE;
    }

    RenderFunc render = get_renderer(self);
    if(!render) {
        return FALSE;
    }

    render(pixels, width, height, stride, (float*)extents);

    return TRUE;
}

/* vim:sw=4:ts=4:et:cindent
 */
