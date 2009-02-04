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

#include <float.h>
#include <string.h>
#include <assert.h>

#include <firtree/main.h>
#include <firtree/kernel.h>
#include <firtree/cpu-runtime.h>
#include <firtree/linker/sampler_provider.h>

#include <firtree/internal/image-int.h>

using namespace llvm;

namespace Firtree { 

using namespace LLVM;

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
    llvm::Module* mod = linker.ReleaseModule();
    llvm::ModuleProvider* modprov = new llvm::ExistingModuleProvider(mod);

    std::string err_str;
    llvm::ExecutionEngine* engine = llvm::ExecutionEngine::createJIT(modprov,
            &err_str);
    if(!engine) {
        FIRTREE_ERROR("Error JIT-ing module: %s", err_str.c_str());
    }

    llvm::Function* kernel_F = mod->getFunction("kernel");

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
            "doit", mod );

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

    // mod->dump();

    KernelFunc kernel_fn = reinterpret_cast<KernelFunc>(
            engine->getPointerToFunction(doit_F));
    assert(kernel_fn != NULL);

    off_t row_start = row_stride * start_row;
    vec2 pos;

    for(size_t row=start_row; row<end_row; ++row, row_start += row_stride) {
        pos.y = row;
        vec4* outptr = reinterpret_cast<vec4*>(dest_img + row_start);
        for(size_t col=start_col; col<end_col; ++col, ++outptr) {
            pos.x = col;
            kernel_fn(&pos, outptr);
        }
    }
    // printf("%f,%f,%f,%f", result.x, result.y, result.z, result.w);

    if(engine) {
        delete engine;
    }
    FIRTREE_SAFE_RELEASE(im_samp_prov);
}

} // namespace Firtree

//=============================================================================
// vim:sw=4:ts=4:cindent:et
//
