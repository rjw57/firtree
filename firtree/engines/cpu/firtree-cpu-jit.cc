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

/* See http://markmail.org/message/7fuma5ghjxtnk5mz fpr why
 * we need to include these headers and not ExecutionEngine.h.
 * This makes me sad :(. */
#include <llvm/ExecutionEngine/JIT.h>
#include <llvm/ExecutionEngine/Interpreter.h>

#include <llvm/Analysis/LoopPass.h>
#include <llvm/Target/TargetData.h>
#include <llvm/LinkAllPasses.h>

#include <llvm/DerivedTypes.h>
#include <llvm/Instructions.h>
#include <llvm/Constants.h>

#include <llvm/Transforms/Utils/Cloning.h>
#include <llvm/Support/CommandLine.h>

#include <llvm/ADT/Triple.h>
#include <llvm/System/Host.h>
#include <llvm/Target/TargetRegistry.h>
#include <llvm/Target/TargetMachine.h>
#include <llvm/Support/FormattedStream.h>

#include <firtree/internal/firtree-engine-intl.hh>
#include <firtree/internal/firtree-sampler-intl.hh>
#include <firtree/internal/firtree-kernel-intl.hh>

#if FIRTREE_LLVM_AT_LEAST_2_6
#   include <llvm/Target/TargetSelect.h>
#endif

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

    "render_FIRTREE_FORMAT_RGBA_F32_PREMULTIPLIED",

    NULL,
};

#define RENDER_FUNC_NAME(id) (_firtree_cpu_jit_function_names[(id)])

static void _firtree_cpu_jit_optimise_module(llvm::Module* m,
        std::vector<const char*>& export_list);

typedef struct _FirtreeCpuJitPrivate FirtreeCpuJitPrivate;

static llvm::ExecutionEngine*   _firtree_cpu_jit_global_llvm_engine = NULL;

struct _FirtreeCpuJitPrivate {
    llvm::ModuleProvider*       cached_llvm_module_provider;
    llvm::Function*             cached_llvm_function;
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
        if(_firtree_cpu_jit_global_llvm_engine) {
            _firtree_cpu_jit_global_llvm_engine->
                deleteModuleProvider(p->cached_llvm_module_provider);
        }
        p->cached_llvm_module_provider = NULL;
    }

    if(p && p->render_buffer_bitcode) {
        delete p->render_buffer_bitcode;
        p->render_buffer_bitcode = NULL;
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

    p->cached_llvm_module_provider = NULL;
    p->cached_llvm_function = NULL;
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

    llvm::Function* f = firtree_kernel_create_overall_function(kernel);
    if(!f) {
        return NULL;
    }

    FirtreeCpuJitReduceFunc rv = (FirtreeCpuJitReduceFunc)
        firtree_cpu_jit_get_compute_function(self,
            func_name, f, FIRTREE_KERNEL_TARGET_REDUCE,
            lazy_creator_function);

    delete f->getParent();

    return rv;
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

    /* Since we moved to using 4-way vectors throughout, this
     * is no-longer required! Yay! */
#if 0
    /* Nasty, nasty hack to set an option to disable MMX *
     * This is really horrible but is required by:       *
     *   http://llvm.org/bugs/show_bug.cgi?id=3287       */
    static bool set_opt = false;
    static const char* opts[] = {
        "progname",
#if FIRTREE_LLVM_AT_LEAST_2_6
        "-mattr=-mmx",
#else
    	"-disable-mmx",
#endif
    };
    if(!set_opt) {
        llvm::cl::ParseCommandLineOptions(sizeof(opts) / sizeof(const char*),
                const_cast<char**>(opts));
    	set_opt = true;
    }
#endif

    /* create an LLVM module from the bitcode */
#if FIRTREE_LLVM_AT_LEAST_2_6
    llvm::Module* m = llvm::ParseBitcodeFile(p->render_buffer_bitcode,
            llvm::getGlobalContext());
#else
    llvm::Module* m = llvm::ParseBitcodeFile(p->render_buffer_bitcode);
#endif
    
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
            llvm::BasicBlock* bb = llvm::BasicBlock::Create(FIRTREE_LLVM_CONTEXT
                    "entry", 
                    existing_llvm_render_function);
            std::vector<llvm::Value*> args;
            llvm::Function::arg_iterator AI = existing_llvm_render_function->arg_begin();
            args.push_back(AI);
            llvm::Value* sample_val = llvm::CallInst::Create(
                    new_sampler_func,
                    args.begin(), args.end(),
                    "rv", bb);
            llvm::ReturnInst::Create(FIRTREE_LLVM_CONTEXT sample_val, bb);
        }
    } else if(target == FIRTREE_KERNEL_TARGET_REDUCE) {
        /* Create the reduce version of the render function. */
        llvm::Function* existing_llvm_render_function = 
            linked_module->getFunction("sampler_reduce_function");
        if(existing_llvm_render_function) {
            llvm::BasicBlock* bb = llvm::BasicBlock::Create(FIRTREE_LLVM_CONTEXT
                    "entry", 
                    existing_llvm_render_function);
            std::vector<llvm::Value*> args;
            llvm::Function::arg_iterator AI = existing_llvm_render_function->arg_begin();
            args.push_back(AI);
            llvm::CallInst::Create(
                    new_sampler_func,
                    args.begin(), args.end(),
                    "", bb);
            llvm::ReturnInst::Create(FIRTREE_LLVM_CONTEXT NULL, bb);
        }
    } else {
        g_error("Unknown target.");
    }

    std::vector<const char*> compute_functions;
    compute_functions.push_back(compute_function_name);
    _firtree_cpu_jit_optimise_module(linked_module, compute_functions);

    if(p->cached_llvm_module_provider) {
        if(_firtree_cpu_jit_global_llvm_engine) {
            _firtree_cpu_jit_global_llvm_engine->
                deleteModuleProvider(p->cached_llvm_module_provider);
        }
        p->cached_llvm_module_provider = NULL;
    }
    p->cached_llvm_function = NULL;
    p->cached_llvm_module_provider = new llvm::ExistingModuleProvider(linked_module);

    if(!_firtree_cpu_jit_global_llvm_engine) {
        std::string err;

#if FIRTREE_LLVM_AT_LEAST_2_6
        bool init_native = llvm::InitializeNativeTarget();
        if(init_native) { // <- quite why the return flag is this way around, I don't know.
            g_error("No native target compiled in!");
        }
        _firtree_cpu_jit_global_llvm_engine = llvm::ExecutionEngine::create(
                p->cached_llvm_module_provider, false, &err,
                llvm::CodeGenOpt::Aggressive, false);
#else
        _firtree_cpu_jit_global_llvm_engine = llvm::ExecutionEngine::create(
                p->cached_llvm_module_provider, false, &err);
#endif

        if(!_firtree_cpu_jit_global_llvm_engine) 
        {
            g_error("Error creating JIT: %s", err.c_str());
        }
    } else {
        _firtree_cpu_jit_global_llvm_engine->addModuleProvider(p->cached_llvm_module_provider);
    }

    g_assert(_firtree_cpu_jit_global_llvm_engine);

    if(lazy_creator_function) {
        _firtree_cpu_jit_global_llvm_engine->InstallLazyFunctionCreator(lazy_creator_function);
    }

    void* compute_function = _firtree_cpu_jit_global_llvm_engine->
        getPointerToFunction(new_compute_func);
    p->cached_llvm_function = new_compute_func;
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

GString* 
firtree_cpu_jit_dump_asm(FirtreeCpuJit* self)
{
    std::string err;

    FirtreeCpuJitPrivate* p = GET_PRIVATE(self); 

    llvm::ModuleProvider* mp = p->cached_llvm_module_provider;
    if(!mp) 
        return NULL;

    llvm::Module* m = mp->getModule();

    // A lot of this code is inspired by lli.cpp.

    // Initialise targets and asm printers
    llvm::InitializeAllTargets();
    llvm::InitializeAllAsmPrinters();

    // Work out the target triple.
    llvm::Triple the_triple(m->getTargetTriple());
    if(the_triple.getTriple().empty())
        the_triple.setTriple(llvm::sys::getHostTriple());

    // Allocate the target machine
    const llvm::Target* the_target =
        llvm::TargetRegistry::lookupTarget(the_triple.getTriple(), err);
    if(!the_target) 
    {
        g_warning("LLVM error selecting target: %s", err.c_str());
        return NULL;
    }

    std::auto_ptr<llvm::TargetMachine> target_machine(
            the_target->createTargetMachine(the_triple.getTriple(), ""));
    g_assert(target_machine.get() && "Could not allocate target machine!");
    llvm::TargetMachine &target = *target_machine.get();

    std::string out_str;
    llvm::raw_string_ostream out_string_stream(out_str);
	llvm::formatted_raw_ostream out(out_string_stream);

    if(target.WantsWholeFile()) 
    {
        llvm::PassManager PM;

        if(const llvm::TargetData* TD = target.getTargetData()) {
            PM.add(new llvm::TargetData(*TD));
        } else {
            PM.add(new llvm::TargetData(m));
        }

        if(target.addPassesToEmitWholeFile(PM, out, 
                    llvm::TargetMachine::AssemblyFile, llvm::CodeGenOpt::Aggressive))
        {
            g_warning("Host target does not support generation of assembly.");
            return NULL;
        }

        PM.run(*m);
    } else {
        llvm::FunctionPassManager Passes(p->cached_llvm_module_provider);

        if(const llvm::TargetData* TD = target.getTargetData()) {
            Passes.add(new llvm::TargetData(*TD));
        } else {
            Passes.add(new llvm::TargetData(m));
        }

        llvm::ObjectCodeEmitter* OCE = NULL;
        target.setAsmVerbosityDefault(true);

        switch(target.addPassesToEmitFile(Passes, out,
                    llvm::TargetMachine::AssemblyFile, llvm::CodeGenOpt::Aggressive)) 
        {
            default:
                g_assert(0 && "Invalid file model!");
                return NULL;
            case llvm::FileModel::Error:
                g_warning("Host target does not support generation of assembly.");
                return NULL;
            case llvm::FileModel::AsmFile:
                break;
            case llvm::FileModel::MachOFile:
            case llvm::FileModel::ElfFile:
                g_error("Unexpected return code from addPassesToEmitFile().");
                break;
        }

        if(target.addPassesToEmitFileFinish(Passes, OCE, llvm::CodeGenOpt::Aggressive)) {
            g_warning("Host target does not support generation of assembly.");
            return NULL;
        }

        Passes.doInitialization();

        for(llvm::Module::iterator I = m->begin(), E = m->end(); I != E; ++I)
        {
            if (!I->isDeclaration()) {
                Passes.run(*I);
            }
        }

        Passes.doFinalization();
    }

    return g_string_new(out_str.c_str());
}

/* vim:sw=4:ts=4:et:cindent
 */
