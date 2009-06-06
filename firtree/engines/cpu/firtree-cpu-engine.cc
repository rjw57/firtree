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

#define bindata _firtee_cpu_engine_render_buffer_mod
#include "render-buffer.llvm.bc.h"
#undef bindata

G_DEFINE_TYPE (FirtreeCpuEngine, firtree_cpu_engine, G_TYPE_OBJECT)

#define GET_PRIVATE(o) \
    (G_TYPE_INSTANCE_GET_PRIVATE ((o), FIRTREE_TYPE_CPU_ENGINE, FirtreeCpuEnginePrivate))

typedef struct _FirtreeCpuEnginePrivate FirtreeCpuEnginePrivate;

struct _FirtreeCpuEnginePrivate {
    FirtreeSampler* sampler;
};

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
}

FirtreeSampler*
firtree_cpu_engine_get_sampler (FirtreeCpuEngine* self)
{
    FirtreeCpuEnginePrivate* p = GET_PRIVATE(self); 
    return p->sampler;
}

llvm::Function*
generate_renderer(FirtreeCpuEngine* self)
{
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
    llvm::Function* sample_function = 
        linker->releaseModule()->getFunction("render_buffer_uc_4");
    delete linker;

    return sample_function;
}

typedef void (* RenderFunc) (unsigned char* buffer,
    unsigned int width, unsigned int height,
    unsigned int row_stride, float* extents);

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

gboolean
firtree_cpu_engine_render_into_pixbuf (FirtreeCpuEngine* self,
        FirtreeVec4* extents, GdkPixbuf* pixbuf)
{
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

    llvm::Function* render_f = generate_renderer(self);
    g_assert(render_f);

    llvm::Module* m = render_f->getParent();

    /* implement the function */
    llvm::Function* f = m->getFunction("sample");

    /* FIXME: this is a stub */
    llvm::BasicBlock* bb = llvm::BasicBlock::Create("entry", f);

    std::vector<llvm::Constant*> elements;
    elements.push_back(llvm::ConstantFP::get(llvm::Type::FloatTy, 1.0));
    elements.push_back(llvm::ConstantFP::get(llvm::Type::FloatTy, 0.0));
    elements.push_back(llvm::ConstantFP::get(llvm::Type::FloatTy, 1.0));
    elements.push_back(llvm::ConstantFP::get(llvm::Type::FloatTy, 1.0));
    llvm::Constant* rv = llvm::ConstantVector::get(
            llvm::VectorType::get(llvm::Type::FloatTy, 4),
            elements);

    llvm::ReturnInst::Create(rv, bb);

    optimise_module(m, render_f->getName().c_str());

    llvm::ModuleProvider* mp = 
        new llvm::ExistingModuleProvider(m);

    std::string err;
    llvm::ExecutionEngine* engine = llvm::ExecutionEngine::createJIT(mp, &err);
    if(!engine) {
        g_debug("Error JIT-ing render: %s\n", err.c_str());
        delete mp;
        return FALSE;
    }

    RenderFunc render = (RenderFunc)
        engine->getPointerToFunction(render_f);
    g_assert(render);

    render(pixels, width, height, stride, (float*)extents);

    delete engine;

    return TRUE;
}

/* vim:sw=4:ts=4:et:cindent
 */
