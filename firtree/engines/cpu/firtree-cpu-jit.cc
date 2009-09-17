/* firtree-cpu-jit.cc */

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

#include "firtree-cpu-jit.hh"

#include <llvm/Module.h>
#include <llvm/ModuleProvider.h>
#include <llvm/Support/MemoryBuffer.h>
#include <llvm/Bitcode/ReaderWriter.h>
#include <llvm/Linker.h>
#include <llvm/ExecutionEngine/ExecutionEngine.h>

#include <llvm/Analysis/LoopPass.h>
#include <llvm/Target/TargetData.h>
#include <llvm/LinkAllPasses.h>

#include <llvm/DerivedTypes.h>
#include <llvm/Instructions.h>
#include <llvm/Constants.h>

#include <llvm/Transforms/Utils/Cloning.h>
#include <llvm/Support/CommandLine.h>

#include <firtree/internal/firtree-sampler-intl.hh>
#include <firtree/internal/firtree-kernel-intl.hh>

#if FIRTREE_HAVE_CLUTTER
#   include "clutter.hh"
#endif

#include <sstream>

G_DEFINE_TYPE (FirtreeCpuJit, firtree_cpu_jit, G_TYPE_OBJECT)

#define GET_PRIVATE(o) \
    (G_TYPE_INSTANCE_GET_PRIVATE ((o), FIRTREE_TYPE_CPU_JIT, FirtreeCpuJitPrivate))

/* 'Load' the compiled bitcode into a string. */
#define bindata _firtree_cpu_jit_render_buffer_mod
#include "llvm-cpu-support.bc.h"
#undef bindata

/* Indexed by FirtreeBufferFormat */
static const char* _firtree_cpu_jit_function_names[] = {
    "render_FIRTREE_FORMAT_ARGB32",
    "render_FIRTREE_FORMAT_ARGB32_PREMULTIPLIED",
    "render_FIRTREE_FORMAT_XRGB32",
    "render_FIRTREE_FORMAT_RGBA32",
    "render_FIRTREE_FORMAT_RGBA32_PREMULTIPLIED",

    "render_FIRTREE_FORMAT_ABGR32",
    "render_FIRTREE_FORMAT_ABGR32_PREMULTIPLIED",
    "render_FIRTREE_FORMAT_XBGR32",
    "render_FIRTREE_FORMAT_BGRA32",
    "render_FIRTREE_FORMAT_BGRA32_PREMULTIPLIED",

    "render_FIRTREE_FORMAT_RGB24",
    "render_FIRTREE_FORMAT_BGR24",

    "render_FIRTREE_FORMAT_RGBX32",
    "render_FIRTREE_FORMAT_BGRX32",

    /* Rendering to the following formats is not supported */

    NULL, /* FIRTREE_FORMAT_L8 */
    NULL, /* FIRTREE_FORMAT_I420_FOURCC */
    NULL, /* FIRTREE_FORMAT_YV12_FOURCC */

    NULL,
};

#define RENDER_FUNC_NAME(id) (_firtree_cpu_jit_function_names[(id)])

static void _firtree_cpu_jit_optimise_module(llvm::Module* m,
        std::vector<const char*>& export_list);

typedef struct _FirtreeCpuJitPrivate FirtreeCpuJitPrivate;

struct _FirtreeCpuJitPrivate {
    llvm::ExecutionEngine*      llvm_engine;
    llvm::ModuleProvider*       cached_llvm_module_provider;
    llvm::MemoryBuffer*         render_buffer_bitcode;
};

/* Internal function called by firtree_cpu_jit_get_render_function_for_sampler 
 * and firtree_cpu_jit_get_reduce_function_for_kernel to call the JIT. */
static void*
firtree_cpu_jit_get_compute_function (FirtreeCpuJit* self,
        const char* compute_function_name,
        llvm::Function* llvm_function,
        FirtreeKernelTarget target,
        FirtreeCpuJitLazyFunctionCreatorFunc lazy_creator_function);

static void
firtree_cpu_jit_dispose (GObject *object)
{
    G_OBJECT_CLASS (firtree_cpu_jit_parent_class)->dispose (object);
    FirtreeCpuJit* cpu_jit = FIRTREE_CPU_JIT(object);
    FirtreeCpuJitPrivate* p = GET_PRIVATE(cpu_jit); 

    if(p && p->cached_llvm_module_provider) {
        if(p->llvm_engine) {
            p->llvm_engine->deleteModuleProvider(p->cached_llvm_module_provider);
        }
        p->cached_llvm_module_provider = NULL;
    }

    if(p && p->render_buffer_bitcode) {
        delete p->render_buffer_bitcode;
        p->render_buffer_bitcode = NULL;
    }

    if(p && p->llvm_engine) {
        delete p->llvm_engine;
        p->llvm_engine = NULL;
    }
}

static void
firtree_cpu_jit_class_init (FirtreeCpuJitClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS (klass);

    g_type_class_add_private (klass, sizeof (FirtreeCpuJitPrivate));

    object_class->dispose = firtree_cpu_jit_dispose;
}

static void
firtree_cpu_jit_init (FirtreeCpuJit *self)
{
    FirtreeCpuJitPrivate* p = GET_PRIVATE(self); 

    p->llvm_engine = NULL;
    p->cached_llvm_module_provider = NULL;
    p->render_buffer_bitcode = 
        llvm::MemoryBuffer::getMemBuffer(
                (const char*)_firtree_cpu_jit_render_buffer_mod,
                (const char*)(_firtree_cpu_jit_render_buffer_mod + 
                    sizeof(_firtree_cpu_jit_render_buffer_mod) - 1));
}

FirtreeCpuJit*
firtree_cpu_jit_new (void)
{
    return (FirtreeCpuJit*)
        g_object_new (FIRTREE_TYPE_CPU_JIT, NULL);
}

void
firtree_cpu_jit_purge_cache(FirtreeCpuJit* self)
{
    FirtreeCpuJitPrivate* p = GET_PRIVATE(self); 

    if(p->cached_llvm_module_provider) {
        delete p->cached_llvm_module_provider;
        p->cached_llvm_module_provider = NULL;
    }
}

FirtreeCpuJitRenderFunc
firtree_cpu_jit_get_render_function_for_sampler(FirtreeCpuJit* self,
        FirtreeBufferFormat format,
        FirtreeSampler* sampler,
        FirtreeCpuJitLazyFunctionCreatorFunc lazy_creator_function)
{
    if((format < 0) || (format >= FIRTREE_FORMAT_LAST)) {
        g_error("Invalid format: %i", format);
        return NULL;
    }

    const char* func_name = RENDER_FUNC_NAME(format);

    return (FirtreeCpuJitRenderFunc)
        firtree_cpu_jit_get_compute_function(self,
            func_name, firtree_sampler_get_sample_function(sampler),
            FIRTREE_KERNEL_TARGET_RENDER,
            lazy_creator_function);
}

FirtreeCpuJitReduceFunc
firtree_cpu_jit_get_reduce_function_for_kernel(FirtreeCpuJit* self,
        FirtreeKernel* kernel,
        FirtreeCpuJitLazyFunctionCreatorFunc lazy_creator_function)
{
    if(!kernel || (firtree_kernel_get_target(kernel) != FIRTREE_KERNEL_TARGET_REDUCE)) {
        g_error("Invalid kernel passed.");
        return NULL;
    }

    const char* func_name = "reduce";

    return (FirtreeCpuJitReduceFunc)
        firtree_cpu_jit_get_compute_function(self,
            func_name, firtree_kernel_get_function(kernel),
            FIRTREE_KERNEL_TARGET_REDUCE,
            lazy_creator_function);
}

void*
firtree_cpu_jit_get_compute_function (FirtreeCpuJit* self,
        const char* compute_function_name,
        llvm::Function* llvm_function,
        FirtreeKernelTarget target,
        FirtreeCpuJitLazyFunctionCreatorFunc lazy_creator_function)
{
    FirtreeCpuJitPrivate* p = GET_PRIVATE(self); 

    if(!compute_function_name) {
        g_debug("No function name specified.");
        return NULL;
    }

    if(llvm_function == NULL) {
        g_debug("No LLVM function.\n");
        return NULL;
    }

    /* Nasty, nasty hack to set an option to disable MMX *
     * This is really horrible but is required by:       *
     *   http://llvm.org/bugs/show_bug.cgi?id=3287       */
    static bool set_opt = false;
    static const char* opts[] = {
        "progname",
    	"-disable-mmx",
    };
    if(!set_opt) {
        llvm::cl::ParseCommandLineOptions(2, const_cast<char**>(opts));
    	set_opt = true;
    }

    /* create an LLVM module from the bitcode */
    llvm::Module* m = llvm::ParseBitcodeFile(p->render_buffer_bitcode);
    
    llvm::Linker* linker = new llvm::Linker("jit_compute", m);

    llvm::Module* llvm_compute_module = llvm::CloneModule(llvm_function->getParent());
    std::string err_str;
    bool was_error = linker->LinkInModule(llvm_compute_module, &err_str);
    if(was_error) {
        g_error("Error linking in sampler: %s\n", err_str.c_str());
    }
    delete llvm_compute_module;
    llvm_compute_module = NULL;

    llvm::Module* linked_module = linker->releaseModule();
    delete linker;

    llvm::Function* new_compute_func = linked_module->getFunction(compute_function_name);

    llvm::Function* new_sampler_func = linked_module->getFunction(
            llvm_function->getName());

    if(target == FIRTREE_KERNEL_TARGET_RENDER) {
        /* Create the render version of the render function. */
        llvm::Function* existing_llvm_render_function = 
            linked_module->getFunction("sampler_render_function");
        if(existing_llvm_render_function) {
            llvm::BasicBlock* bb = llvm::BasicBlock::Create("entry", 
                    existing_llvm_render_function);
            std::vector<llvm::Value*> args;
            llvm::Function::arg_iterator AI = existing_llvm_render_function->arg_begin();
            args.push_back(AI);
            llvm::Value* sample_val = llvm::CallInst::Create(
                    new_sampler_func,
                    args.begin(), args.end(),
                    "rv", bb);
            llvm::ReturnInst::Create(sample_val, bb);
        }
    } else if(target == FIRTREE_KERNEL_TARGET_REDUCE) {
        /* Create the reduce version of the render function. */
        llvm::Function* existing_llvm_render_function = 
            linked_module->getFunction("sampler_reduce_function");
        if(existing_llvm_render_function) {
            llvm::BasicBlock* bb = llvm::BasicBlock::Create("entry", 
                    existing_llvm_render_function);
            std::vector<llvm::Value*> args;
            llvm::Function::arg_iterator AI = existing_llvm_render_function->arg_begin();
            args.push_back(AI);
            ++AI;
            args.push_back(AI);
            llvm::CallInst::Create(
                    new_sampler_func,
                    args.begin(), args.end(),
                    "rv", bb);
            llvm::ReturnInst::Create(NULL, bb);
        }
    } else {
        g_error("Unknown target.");
    }

    std::vector<const char*> compute_functions;
    compute_functions.push_back(compute_function_name);
    _firtree_cpu_jit_optimise_module(linked_module, compute_functions);

    if(p->cached_llvm_module_provider) {
        if(p->llvm_engine) {
            p->llvm_engine->deleteModuleProvider(p->cached_llvm_module_provider);
        }
        p->cached_llvm_module_provider = NULL;
    }
    p->cached_llvm_module_provider = new llvm::ExistingModuleProvider(linked_module);

    std::string err;
    if(!p->llvm_engine) {
        p->llvm_engine = llvm::ExecutionEngine::create(
                p->cached_llvm_module_provider, false, &err);
    } else {
        p->llvm_engine->addModuleProvider(p->cached_llvm_module_provider);
    }

    g_assert(p->llvm_engine);

    if(lazy_creator_function) {
        p->llvm_engine->InstallLazyFunctionCreator(lazy_creator_function);
    }

    void* compute_function = p->llvm_engine->getPointerToFunction(new_compute_func);
    g_assert(compute_function);

    return compute_function;
}

/* optimise a llvm module by internalising all but the
 * named function and agressively inlining. */
static void _firtree_cpu_jit_optimise_module(llvm::Module* m,
        std::vector<const char*>& export_list)
{
    if(m == NULL) {
    	return;
    }

    llvm::PassManager PM;

    PM.add(new llvm::TargetData(m));

#if FIRTREE_HAVE_CLUTTER
    PM.add(new CanonicaliseCoglCallsPass());
#endif

    PM.add(llvm::createInternalizePass(export_list));
    PM.add(llvm::createFunctionInliningPass(32768)); 

    PM.add(llvm::createAggressiveDCEPass()); 

    firtree_engine_create_standard_optimization_passes(&PM, 3, 
                            /*OptimizeSize=*/ false,
                            /*UnitAtATime=*/ true,
                            /*UnrollLoops=*/ true,
                            /*SimplifyLibCalls=*/ true,
                            /*HaveExceptions=*/ false,
                            llvm::createFunctionInliningPass(32768));

    PM.run(*m);
}

/* vim:sw=4:ts=4:et:cindent
 */
