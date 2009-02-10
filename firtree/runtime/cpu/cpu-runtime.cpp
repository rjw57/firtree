// FIRTREE - A generic image processing library
// Copyright (C) 2007, 2008 Rich Wareham <richwareham@gmail.com>
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License verstion as published
// by the Free Software Foundation.
//
// This program is distributed in the hope that it will be useful, but WITHOUT
// ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
// FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
// more details.
//
// You should have received a copy of the GNU General Public License along with
// this program; if not, write to the Free Software Foundation, Inc., 51
// Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA

//=============================================================================
// This file implements the FIRTREE compiler utility functions.
//=============================================================================

#define __STDC_LIMIT_MACROS
#include <stdint.h>
#include <llvm/Module.h>
#include <llvm/ModuleProvider.h>
#include <llvm/Function.h>
#include <llvm/DerivedTypes.h>
#include <llvm/Type.h>
#include <llvm/BasicBlock.h>
#include <llvm/Instructions.h>
#include <llvm/ExecutionEngine/GenericValue.h>
#include <llvm/ExecutionEngine/ExecutionEngine.h>
#include <llvm/Bitcode/ReaderWriter.h>
#include <llvm/Linker.h>
#include <llvm/Support/MemoryBuffer.h>
#include <llvm/Assembly/PrintModulePass.h>
#include <llvm/System/DynamicLibrary.h>

#include "llvm/PassManager.h"
#include "llvm/Analysis/CallGraph.h"
#include "llvm/Target/TargetData.h"
#include "llvm/Transforms/Scalar.h"
#include "llvm/LinkAllPasses.h"

#include <float.h>
#include <string.h>
#include <assert.h>

#include <cmath>

#include <firtree/main.h>
#include <firtree/kernel.h>
#include <firtree/cpu-runtime.h>
#include <firtree/linker/sampler_provider.h>

#include <firtree/internal/image-int.h>

#include "builtins_bc.h"

using namespace llvm;

namespace Firtree { 

using namespace LLVM;

struct BuiltinDef {
    const char* name;
    void*       func;
};

#define BUILTIN_FROM_CMATH( funcname ) \
    { "_ft_" #funcname , reinterpret_cast<void*>(funcname) }

static BuiltinDef ft_builtins[] = {
    BUILTIN_FROM_CMATH( expf ),
    BUILTIN_FROM_CMATH( exp2f ),
    BUILTIN_FROM_CMATH( logf ),
    BUILTIN_FROM_CMATH( log2f ),
    BUILTIN_FROM_CMATH( tanf ),
    BUILTIN_FROM_CMATH( asinf ),
    BUILTIN_FROM_CMATH( acosf ),
    BUILTIN_FROM_CMATH( atanf ),
    BUILTIN_FROM_CMATH( atan2f ),
    { NULL, NULL },
};
static bool ft_haveRegisteredBuiltins = false;

typedef float vec2[2];
typedef float vec4[4];
typedef void (*KernelFunc) (vec2* coord, vec4* output);

struct FlattenedSampler {
	vec4*   PixelData;
	int32_t	Width;			// In pixels.
	int32_t	Height;			// In pixels.
};

//===========================================================================
/// This pass takes a vector of FlattenedSampler structs and replaces calls
/// to texSample() with appropriate calls to sampling functions.
struct FlatSamplerCallsPass : public llvm::BasicBlockPass {
    static char ID;

    FlatSamplerCallsPass(const FlattenedSampler* list,
            llvm::Function* nn_sample_func,
            llvm::Function* lin_sample_func)
        :   llvm::BasicBlockPass((intptr_t)&ID)
        ,   m_NearestNeighbourSampleFunc(nn_sample_func)
        ,   m_LinearSampleFunc(lin_sample_func)
        ,   m_List(list)
    {
    }

    ~FlatSamplerCallsPass()
    {
    }

    virtual bool runOnBasicBlock(llvm::BasicBlock &BB)
    {
        bool bModified = false;
        std::vector<llvm::Instruction*> to_erase;

        for(BasicBlock::iterator i=BB.begin(); i!=BB.end(); ++i) {
            if(llvm::isa<CallInst>(i)) {
                CallInst* ci = llvm::cast<CallInst>(i);
                Function* f = ci->getCalledFunction();
            
                if(f->getName() == "texSample") {
                    std::vector<llvm::Value*> params;
                    params.push_back(ci->getOperand(1));
                    params.push_back(ci->getOperand(2));
                    llvm::Value* new_inst = llvm::CallInst::Create(
                        //m_NearestNeighbourSampleFunc,
                        m_LinearSampleFunc,
                        params.begin(), params.end(),
                        "tmp", ci);
                    ci->replaceAllUsesWith(new_inst);
                    to_erase.push_back(ci);

                    bModified = true;
                }
            }
        }

        for(std::vector<llvm::Instruction*>::iterator i=to_erase.begin();
                i!=to_erase.end(); ++i) {
            (*i)->eraseFromParent();
        }

        return bModified;
    }

    private:

    llvm::Function* m_NearestNeighbourSampleFunc;
    llvm::Function* m_LinearSampleFunc;

    const FlattenedSampler* m_List;
};

char FlatSamplerCallsPass::ID = 0;

//=============================================================================
CPURenderer::CPURenderer(const Rect2D& viewport)
    :   ReferenceCounted()
    ,   m_ViewportRect(viewport)
    ,   m_OutputRep(NULL)
{
    uint32_t w = viewport.Size.Width;
    uint32_t h = viewport.Size.Height;

    Blob* image_blob = Blob::CreateWithLength(w*h*sizeof(float)*4);

    m_OutputRep = BitmapImageRep::Create(image_blob,
            w, h, w*4*sizeof(float),
            BitmapImageRep::Float);

    FIRTREE_SAFE_RELEASE(image_blob);
}

//=============================================================================
CPURenderer::~CPURenderer()
{
    FIRTREE_SAFE_RELEASE(m_OutputRep);
}

//=============================================================================
CPURenderer* CPURenderer::Create(uint32_t width, uint32_t height)
{
    Rect2D vp(0,0,width,height);
    return CPURenderer::Create(vp);
}

//=============================================================================
CPURenderer* CPURenderer::Create(const Rect2D& viewport)
{
    return new CPURenderer(viewport);
}

//=============================================================================
void CPURenderer::RenderWithOrigin(Image* image, 
        const Point2D& origin)
{
    // Firstly, extract the image size and make sure it isn't infinite
    // in extent.
    Rect2D extent = image->GetExtent();

    if(Rect2D::IsInfinite(extent))
    {
        FIRTREE_ERROR("An infinite extent image has no origin.");
        return;
    }

    Point2D renderPoint = Point2D(origin.X + extent.Origin.X,
            origin.Y + extent.Origin.Y);
    RenderAtPoint(image, renderPoint, extent);
}

//=============================================================================
void CPURenderer::RenderAtPoint(Image* image, const Point2D& location,
        const Rect2D& srcRect)
{
    RenderInRect(image, Rect2D(location, srcRect.Size), srcRect);
}

//=============================================================================
BitmapImageRep* CPURenderer::CreateBitmapImageRepFromImage(Image* image,
        Firtree::BitmapImageRep::PixelFormat format)
{
    // NOTE: This might accidently call some OpenGL.
    BitmapImageRep* rv = 
        dynamic_cast<Internal::ImageImpl*>(image)->CreateBitmapImageRep(format);

    return rv;
}

//=============================================================================
bool CPURenderer::WriteImageToFile(Image* image, const char* pFileName)
{
    BitmapImageRep* bir = CreateBitmapImageRepFromImage(image);
    if(bir == NULL)
    {
        FIRTREE_WARNING("Failed to create bitmap of image %p.", image);
        return false;
    }

    bool rv = bir->WriteToFile(pFileName);
    FIRTREE_SAFE_RELEASE(bir);

    return rv;
}

//=============================================================================
void CPURenderer::Clear(float r, float g, float b, float a)
{
    float to_set[] = {r,g,b,a};
    uint8_t* to_set_bytes = reinterpret_cast<uint8_t*>(to_set);
    uint8_t* dest = const_cast<uint8_t*>(m_OutputRep->ImageBlob->GetBytes());
    size_t num_bytes = m_OutputRep->ImageBlob->GetLength();

    for(size_t i=0; i<num_bytes; ++i)
    {
        dest[i] = to_set_bytes[i % sizeof(to_set)];
    }
}

//=============================================================================
Image* CPURenderer::CreateImage()
{
    return Image::CreateFromBitmapData(m_OutputRep, false);
}

//=============================================================================
void CPURenderer::RenderInRect(Image* image, const Rect2D& destRect, 
        const Rect2D& srcRect)
{
    // If we've not yet registered the builtins, do so.
    if(!ft_haveRegisteredBuiltins) {
        uint32_t idx = 0;
        while(ft_builtins[idx].name) {
            llvm::sys::DynamicLibrary::AddSymbol(
                    ft_builtins[idx].name,
                    ft_builtins[idx].func);
            ++idx;
        }
        ft_haveRegisteredBuiltins = true;
    }

#if 1
    FIRTREE_DEBUG("RenderInRect image %p:", image);
    FIRTREE_DEBUG(" dst rect: %f,%f+%f+%f.", destRect.Origin.X, destRect.Origin.Y,
            destRect.Size.Width, destRect.Size.Height);
    FIRTREE_DEBUG(" src rect: %f,%f+%f+%f.", srcRect.Origin.X, srcRect.Origin.Y,
            srcRect.Size.Width, srcRect.Size.Height);
#endif

    FIRTREE_DEBUG("  vp rect: %f,%f+%f+%f.", 
            m_ViewportRect.Origin.X, m_ViewportRect.Origin.Y,
            m_ViewportRect.Size.Width, m_ViewportRect.Size.Height);

    Rect2D clipSrcRect = srcRect;
    Rect2D renderRect = destRect;

    // Firstly clip srcRect by extent
    Rect2D extent = image->GetExtent();

    // if extent is non-infinite, clip
    if(!Rect2D::IsInfinite(extent))
    {
        AffineTransform* srcToDestTrans = 
            Rect2D::ComputeTransform(srcRect, destRect);

        clipSrcRect = Rect2D::Intersect(srcRect, extent);

        // Transform clipped source to destination
        renderRect = Rect2D::Transform(clipSrcRect, srcToDestTrans);

        // Clip the render rectangle by the viewport
        renderRect = Rect2D::Intersect(renderRect, m_ViewportRect);

        // Transform clipped destination rect back to source
        srcToDestTrans->Invert();
        clipSrcRect = Rect2D::Transform(renderRect, srcToDestTrans);

        FIRTREE_SAFE_RELEASE(srcToDestTrans);

        // Do nothing if we've clipped everything away.
        if(Rect2D::IsZero(renderRect))
        {
            return;
        }
    }

#if 1
    FIRTREE_DEBUG(" rdr rect: %f,%f+%f+%f.", 
            renderRect.Origin.X, renderRect.Origin.Y,
            renderRect.Size.Width, renderRect.Size.Height);
    FIRTREE_DEBUG("csrc rect: %f,%f+%f+%f.", 
            clipSrcRect.Origin.X, clipSrcRect.Origin.Y,
            clipSrcRect.Size.Width, clipSrcRect.Size.Height);
#else
    FIRTREE_DEBUG(" rdr rect: %f,%f+%f+%f.", 
            renderRect.Origin.X, renderRect.Origin.Y,
            renderRect.Size.Width, renderRect.Size.Height);
#endif

    SamplerProvider* im_samp_prov = SamplerProvider::CreateFromImage(image);
    SamplerLinker linker;

    linker.LinkSampler(im_samp_prov);
    
    // Get the sampler table.
    const std::vector<SamplerProvider*>& sampler_table = 
        linker.GetSamplerTable();

    std::vector<BitmapImageRep*> to_release;
    FlattenedSampler* flat_sampler_list = NULL;
    if(sampler_table.size() > 0 ) {
        flat_sampler_list = new FlattenedSampler[sampler_table.size()];
        size_t idx = 0;
        for(std::vector<SamplerProvider*>::
                const_iterator i = sampler_table.begin();
                i != sampler_table.end(); ++i, ++idx) {
            FlattenedSampler fs;
            const Image* base_im = reinterpret_cast<
                const Internal::ImageImpl*>((*i)->GetImage())->GetBaseImage();
            const Internal::ImageImpl* im = reinterpret_cast<
                const Internal::ImageImpl*>(base_im);

            //printf("PR: %i\n", im->GetPreferredRepresentation());
            if(im->GetPreferredRepresentation() != 
                    Internal::ImageImpl::Kernel) {
                BitmapImageRep* bir = CreateBitmapImageRepFromImage(
                        const_cast<Image*>(base_im),
                        BitmapImageRep::Float);
                fs.PixelData = (vec4*)(bir->ImageBlob->GetBytes());
                fs.Width = bir->Width;
                fs.Height = bir->Height;
                to_release.push_back(bir);
            } else {
                fs.PixelData = NULL;
                fs.Width = fs.Height = 0;
            }
            //printf("B: %p,%i,%i\n", fs.PixelData, fs.Width, fs.Height);
            flat_sampler_list[idx] = fs;
        }
    }

    llvm::Module* kernel_module = linker.ReleaseModule();

    // Now load our builtins library.
    llvm::MemoryBuffer* lib_buf = llvm::MemoryBuffer::getMemBuffer(
            reinterpret_cast<char*>(builtins_bc),
            reinterpret_cast<char*>(builtins_bc + sizeof(builtins_bc) - 1));

#if 0
    llvm::ModuleProvider* lib_mod_prov = 
        llvm::getBitcodeModuleProvider(lib_buf);
    assert(lib_mod_prov && "Error parsing builtin bitcode!");
    llvm::Module* lib_module = lib_mod_prov->getModule();
#else
    llvm::Module* lib_module = ParseBitcodeFile(lib_buf);
    if(!lib_module) {
        FIRTREE_ERROR("Error parsing bitcode file.");
    }
    llvm::ModuleProvider* lib_mod_prov = 
        new llvm::ExistingModuleProvider(lib_module);
    delete lib_buf;
#endif

    // Link in our kernel module
    // Linking kernel into library is better.
    std::string link_err_str;
    if(llvm::Linker::LinkModules(lib_module, kernel_module, &link_err_str)) {
        FIRTREE_ERROR("Error linking bultins: %s.", link_err_str.c_str());
    }
    
    // Delete the kernel module.
    if(kernel_module) {
        delete kernel_module;
    }

    llvm::Function* nn_samp_func = lib_module->getFunction("nearestSample");
    assert(nn_samp_func && "No nearest neighbour sample function!");

    llvm::Function* lin_samp_func = lib_module->getFunction("linearSample");
    assert(lin_samp_func && "No linear sample function!");

    // Register some passes with a PassManager, and run them.
    std::vector<const char*> exportList;
    exportList.push_back("doit");

    PassManager Passes;
    Passes.add(new TargetData(lib_module));
#if 1
    if(flat_sampler_list) {
        Passes.add(new FlatSamplerCallsPass(flat_sampler_list,
                    nn_samp_func, lin_samp_func));
    }
    Passes.add(createInternalizePass(exportList));
    Passes.add(createGlobalDCEPass());
    Passes.add(createGlobalOptimizerPass());
    Passes.add(createFunctionInliningPass(32768)); // Aggressively inline.
    Passes.add(createScalarReplAggregatesPass());
    Passes.add(createInstructionCombiningPass());
#endif
    Passes.run(*lib_module);

#if 0
    PassManager foo;
    foo.add(new PrintFunctionPass());
    foo.run(*lib_module);
#endif

    std::string err_str;
    llvm::ExecutionEngine* engine = 
        llvm::ExecutionEngine::createJIT(lib_mod_prov, &err_str);
    if(!engine) {
        FIRTREE_ERROR("Error JIT-ing module: %s", err_str.c_str());
    }

    if(flat_sampler_list) {
        GlobalVariable* ft_sampler_table_gv = 
            lib_module->getGlobalVariable("_ft_sampler_table", true);
        // The sampler table global may well have been optimised away.
        if(ft_sampler_table_gv) {
            engine->updateGlobalMapping(ft_sampler_table_gv,
                    reinterpret_cast<void*>(flat_sampler_list));
        }
    }

    KernelFunc kernel_fn = reinterpret_cast<KernelFunc>(
            engine->getPointerToFunction(lib_module->getFunction("doit")));
    assert(kernel_fn != NULL);

    int32_t start_row = renderRect.Origin.Y;
    int32_t end_row = renderRect.Origin.Y + renderRect.Size.Height;
    int32_t start_col = renderRect.Origin.X;
    int32_t end_col = renderRect.Origin.X + renderRect.Size.Width;

/*
    printf("%i, %i\n", start_row, end_row);
    printf("%i, %i\n", start_col, end_col);
    printf("%f, %f\n", clipSrcRect.Origin.Y, clipSrcRect.Origin.X);
*/

    uint8_t* dest_img = const_cast<uint8_t*>(
            m_OutputRep->ImageBlob->GetBytes());
    off_t row_stride = m_OutputRep->Stride;

    off_t row_start = (row_stride * start_row) + 
        (start_col * sizeof(vec4));
    vec2 pos;

    pos[1] = clipSrcRect.Origin.Y;
    for(int32_t row=start_row; row<end_row; ++row, row_start += row_stride) {
        vec4* outptr = reinterpret_cast<vec4*>(dest_img + row_start);
        pos[0] = clipSrcRect.Origin.X;
        for(int32_t col=start_col; col<end_col; ++col, ++outptr, ++pos[0]) {
            kernel_fn(&pos, outptr);
#if 0
            if((col == start_col) || (col == end_col-1) ||
                    (row == start_row) || (row == end_row-1)) {
                (*outptr)[1] = 1.f;
                (*outptr)[3] = 1.f;
            }
#endif
        }
        ++pos[1];
    }

    if(engine) {
        delete engine;
    }

    FIRTREE_SAFE_RELEASE(im_samp_prov);

    for(std::vector<BitmapImageRep*>::iterator ri=to_release.begin();
            ri!=to_release.end(); ++ri) {
        BitmapImageRep* bir = *ri;
        FIRTREE_SAFE_RELEASE(bir);
    }

    if(flat_sampler_list) {
        delete flat_sampler_list;
    }
}

} // namespace Firtree

//=============================================================================
// vim:sw=4:ts=4:cindent:et
//
