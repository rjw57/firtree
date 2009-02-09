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

struct vec2 { float x, y; };
struct vec4 { float x,y,z,w; };
typedef void (*KernelFunc) (vec2* coord, vec4* output);

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

#if 0
    uint8_t digest[20];
    glslSampler->ComputeDigest(digest);
    for(int i=0; i<20; i++)
    {
        printf("%02X", digest[i]);
    }
    printf("\n");
#endif

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

#if 0
    FIRTREE_DEBUG(" clip dst: %f,%f+%f+%f.", renderRect.Origin.X, renderRect.Origin.Y,
            renderRect.Size.Width, renderRect.Size.Height);
    FIRTREE_DEBUG(" clip src: %f,%f+%f+%f.", clipSrcRect.Origin.X, clipSrcRect.Origin.Y,
            clipSrcRect.Size.Width, clipSrcRect.Size.Height);
#endif

    FIRTREE_DEBUG(" rdr rect: %f,%f+%f+%f.", 
            renderRect.Origin.X, renderRect.Origin.Y,
            renderRect.Size.Width, renderRect.Size.Height);

    size_t start_row = renderRect.Origin.Y;
    size_t end_row = renderRect.Origin.Y + renderRect.Size.Height;
    size_t start_col = renderRect.Origin.X;
    size_t end_col = renderRect.Origin.X + renderRect.Size.Width;

    uint8_t* dest_img = const_cast<uint8_t*>(
            m_OutputRep->ImageBlob->GetBytes());
    off_t row_stride = m_OutputRep->Stride;

    SamplerProvider* im_samp_prov = SamplerProvider::CreateFromImage(image);
    SamplerLinker linker;

    linker.LinkSampler(im_samp_prov);
    llvm::Module* kernel_module = linker.ReleaseModule();

#if 0
    llvm::Function* kernel_F = kernel_module->getFunction("kernel");

    // We want to add our 'doit' function which will actually do the 
    // work.
    std::vector<const Type*> params;
    params.push_back(PointerType::get( 
                VectorType::get( Type::FloatTy, 2 ), 0 ));
    params.push_back(PointerType::get( 
                VectorType::get( Type::FloatTy, 4 ), 0 ));
    FunctionType* doit_FT = FunctionType::get( 
            Type::VoidTy, params, false );
    Function* doit_F = Function::Create( doit_FT,
            Function::ExternalLinkage,
            "doit", kernel_module );

    BasicBlock* entry_BB = BasicBlock::Create( "entry", doit_F );

    llvm::Function::arg_iterator ai = doit_F->arg_begin();
    llvm::Value* coord = new llvm::LoadInst( ai, "coord", entry_BB );
    std::vector<llvm::Value*> kernel_params;
    kernel_params.push_back(coord);

    llvm::Value* rv = CallInst::Create( kernel_F, 
            kernel_params.begin(), kernel_params.end(),
            "result", entry_BB );

    ++ai;
    new StoreInst( rv, ai, entry_BB );

    ReturnInst::Create( entry_BB );
#endif

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

    /*
#   define STR(x) #x
    lib_module->setTargetTriple( STR(TARGET_TRIPLE) );
    */

    // Register some passes with a PassManager, and run them.
    std::vector<const char*> exportList;
    exportList.push_back("doit");

    PassManager Passes;
    Passes.add(new TargetData(lib_module));
#if 1
    Passes.add(createInternalizePass(exportList));
    Passes.add(createGlobalDCEPass());
    Passes.add(createGlobalOptimizerPass());
    Passes.add(createFunctionInliningPass(32768));
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

    KernelFunc kernel_fn = reinterpret_cast<KernelFunc>(
            engine->getPointerToFunction(lib_module->getFunction("doit")));
    assert(kernel_fn != NULL);

    off_t row_start = (row_stride * (end_row-1)) + (start_col * sizeof(vec4));
    vec2 pos;

    for(size_t row=start_row; row<end_row; ++row, row_start -= row_stride) {
        pos.y = row;
        vec4* outptr = reinterpret_cast<vec4*>(dest_img + row_start);
        for(size_t col=start_col; col<end_col; ++col, ++outptr) {
            pos.x = col;
            kernel_fn(&pos, outptr);
        }
    }

    if(engine) {
        delete engine;
    }

    FIRTREE_SAFE_RELEASE(im_samp_prov);
}

} // namespace Firtree

//=============================================================================
// vim:sw=4:ts=4:cindent:et
//
