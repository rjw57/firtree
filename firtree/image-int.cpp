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
/// \file image-int.cpp The implementation of the internal FIRTREE image.
//=============================================================================

#include <firtree/main.h>
#include <firtree/math.h>
#include <firtree/image.h>
#include <firtree/opengl.h>
#include <firtree/glsl-runtime.h>
#include <internal/image-int.h>
#include <internal/render-to-texture.h>
#include <runtime/glsl/glsl-runtime-priv.h>

#include <assert.h>
#include <float.h>
#include <cmath>

namespace Firtree { namespace Internal {

#define CHECK_GL(a) do { \
    { a ; } \
    GLenum _err = glGetError(); \
    if(_err != GL_NO_ERROR) { \
        FIRTREE_ERROR("OpenGL error executing '%s': %s", \
            #a, gluErrorString(_err)); \
    } \
} while(0)

//=============================================================================
ImageImpl::ImageImpl()
    :   Image()
{
}

//=============================================================================
ImageImpl::~ImageImpl()
{
}

//=============================================================================
Kernel* ImageImpl::GetKernel() const
{
    return NULL;
}

//=============================================================================
TransformedImageImpl::TransformedImageImpl(const Image* inim, AffineTransform* t)
    :   ImageImpl()
{
    m_Transform = t->Copy();
    m_BaseImage = dynamic_cast<ImageImpl*>(const_cast<Image*>(inim));
    FIRTREE_SAFE_RETAIN(m_BaseImage);
}

//=============================================================================
TransformedImageImpl::~TransformedImageImpl()
{
    FIRTREE_SAFE_RELEASE(m_Transform);
    FIRTREE_SAFE_RELEASE(m_BaseImage);
}

//=============================================================================
ImageImpl::PreferredRepresentation 
TransformedImageImpl::GetPreferredRepresentation() const
{
    return m_BaseImage->GetPreferredRepresentation();
}

//=============================================================================
Image* TransformedImageImpl::GetBaseImage() const
{
    TransformedImageImpl* transImage = 
        dynamic_cast<TransformedImageImpl*>(m_BaseImage);
    if(transImage != NULL)
    {
        return transImage->GetBaseImage();
    }

    return m_BaseImage;
}

//=============================================================================
Rect2D TransformedImageImpl::GetExtent() const
{
    return Rect2D::Transform(m_BaseImage->GetExtent(), m_Transform);
}

//=============================================================================
Size2D TransformedImageImpl::GetUnderlyingPixelSize() const
{
    return m_BaseImage->GetUnderlyingPixelSize();
}

//=============================================================================
AffineTransform* TransformedImageImpl::GetTransformFromUnderlyingImage() const
{
    AffineTransform* t = m_BaseImage->GetTransformFromUnderlyingImage();
    t->AppendTransform(m_Transform);
    return t;
}

//=============================================================================
unsigned int TransformedImageImpl::GetAsOpenGLTexture(OpenGLContext* ctx)
{
    return m_BaseImage->GetAsOpenGLTexture(ctx);
}

//=============================================================================
Firtree::BitmapImageRep* TransformedImageImpl::GetAsBitmapImageRep()
{
    return m_BaseImage->GetAsBitmapImageRep();
}

//=============================================================================
// TEXTURE BACKED IMAGE
//=============================================================================

//=============================================================================
TextureBackedImageImpl::TextureBackedImageImpl()
    :   ImageImpl()
    ,   m_BitmapRep(NULL)
{
}

//=============================================================================
TextureBackedImageImpl::~TextureBackedImageImpl()
{
    FIRTREE_SAFE_RELEASE(m_BitmapRep);
}

//=============================================================================
ImageImpl::PreferredRepresentation 
TextureBackedImageImpl::GetPreferredRepresentation() const
{
    return ImageImpl::OpenGLTexture;
}

//=============================================================================
Firtree::BitmapImageRep* TextureBackedImageImpl::GetAsBitmapImageRep()
{
    OpenGLContext* c = GLSL::GetCurrentGLContext();

    if(c == NULL)
    {
        FIRTREE_ERROR("Attempt to render image to bitmap outside of an OpenGL "
                "context.");
    }

    unsigned int tex = GetAsOpenGLTexture(c);

    c->Begin();
    GLint w, h;
    CHECK_GL( glBindTexture(GL_TEXTURE_2D, tex) );
    CHECK_GL( glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &w) );
    CHECK_GL( glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_HEIGHT, &h) );

    Blob* imageBlob = Blob::CreateWithLength(w*h*4*sizeof(float));
    CHECK_GL( glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_FLOAT, 
                (void*)(imageBlob->GetBytes())) );
    c->End();

    FIRTREE_SAFE_RELEASE(m_BitmapRep);
    m_BitmapRep = BitmapImageRep::Create(imageBlob,
            w, h, w*4*sizeof(float), Firtree::BitmapImageRep::Float, false);
    FIRTREE_SAFE_RELEASE(imageBlob);

    return m_BitmapRep;
}

//=============================================================================
// BITMAP BACKED IMAGE
//=============================================================================

//=============================================================================
BitmapBackedImageImpl::BitmapBackedImageImpl()
    :   ImageImpl()
    ,   m_GLTexture(0)
    ,   m_GLContext(NULL)
{
}

//=============================================================================
BitmapBackedImageImpl::~BitmapBackedImageImpl()
{
    if(m_GLTexture != 0)
    {
        m_GLContext->DeleteTexture(m_GLTexture);
        m_GLTexture = 0;
    }
    FIRTREE_SAFE_RELEASE(m_GLContext);
}

//=============================================================================
ImageImpl::PreferredRepresentation 
BitmapBackedImageImpl::GetPreferredRepresentation() const
{
    return ImageImpl::BitmapImageRep;
}

//=============================================================================
unsigned int BitmapBackedImageImpl::GetAsOpenGLTexture(OpenGLContext* ctx)
{
    // If the context we used to create the texture doesn't match
    // the one asked for, purge our cache.
    if((m_GLTexture != 0) && (ctx != m_GLContext))
    {
        m_GLContext->DeleteTexture(m_GLTexture);
        m_GLTexture = 0;
        FIRTREE_SAFE_RELEASE(m_GLContext);
    }

    Firtree::BitmapImageRep* bir = GetAsBitmapImageRep();

    ctx->Begin();

    if(m_GLTexture == 0)
    {
        FIRTREE_SAFE_RETAIN(ctx);
        FIRTREE_SAFE_RELEASE(m_GLContext);
        m_GLContext = ctx;

        m_GLTexture = m_GLContext->GenTexture();
    }

    m_TexSize = Size2DU32(bir->Width, bir->Height);
    CHECK_GL( glBindTexture(GL_TEXTURE_2D, m_GLTexture) );

    if(bir->Format == Firtree::BitmapImageRep::Float)
    { 
        assert(bir->Stride == bir->Width*4*4);
        CHECK_GL( glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 
                    bir->Width, bir->Height, 0,
                    GL_RGBA, GL_FLOAT,
                    bir->ImageBlob->GetBytes()) );
    } else if(bir->Format == Firtree::BitmapImageRep::Byte) {
        assert(bir->Stride == bir->Width*4);
        CHECK_GL( glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 
                    bir->Width, bir->Height, 0,
                    GL_RGBA, GL_UNSIGNED_BYTE,
                    bir->ImageBlob->GetBytes()) );
    } else {
        FIRTREE_ERROR("Unknown bitmap image rep format: %i", bir->Format);
    }

    ctx->End();

    return m_GLTexture;
}

//=============================================================================
// KERNEL IMAGE
//=============================================================================

//=============================================================================
KernelImageImpl::KernelImageImpl(Firtree::Kernel* k, ExtentProvider* extentProvider)
    :   TextureBackedImageImpl()
    ,   m_Kernel(k)
    ,   m_ExtentProvider(extentProvider)
    ,   m_TextureRenderer(NULL)
    ,   m_GLRenderer(NULL)
{
    FIRTREE_SAFE_RETAIN(m_Kernel);
    FIRTREE_SAFE_RETAIN(m_ExtentProvider);
}

//=============================================================================
KernelImageImpl::~KernelImageImpl()
{
    FIRTREE_SAFE_RELEASE(m_GLRenderer);
    FIRTREE_SAFE_RELEASE(m_TextureRenderer);

    FIRTREE_SAFE_RELEASE(m_Kernel);
    FIRTREE_SAFE_RELEASE(m_ExtentProvider);
}

//=============================================================================
ImageImpl::PreferredRepresentation KernelImageImpl::GetPreferredRepresentation() const
{
    return ImageImpl::Kernel;
}

//=============================================================================
Rect2D KernelImageImpl::GetExtent() const
{
    if(m_ExtentProvider == NULL)
    {
        FIRTREE_WARNING("Kernel backed image lacks an extent provider.");
        return Rect2D::MakeInfinite();
    }

    return m_ExtentProvider->ComputeExtentForKernel(m_Kernel);
}

//=============================================================================
Size2D KernelImageImpl::GetUnderlyingPixelSize() const
{
    // Use the extent provider.
    return GetExtent().Size;
}

//=============================================================================
AffineTransform* KernelImageImpl::GetTransformFromUnderlyingImage() const
{
    return AffineTransform::Identity();
}

//=============================================================================
Firtree::Kernel* KernelImageImpl::GetKernel() const
{
    return m_Kernel;
}

//=============================================================================
unsigned int KernelImageImpl::GetAsOpenGLTexture(OpenGLContext* ctx)
{
    Rect2D extent = GetExtent();
    if(Rect2D::IsInfinite(extent))
    {
        FIRTREE_ERROR("Cannot render infinite extent image to texture.");
        return 0;
    }

    /*
    FIRTREE_DEBUG("Extent: %f,%f+%f+%f.",
            extent.Origin.X, extent.Origin.Y,
            extent.Size.Width, extent.Size.Height);
    */

    Size2DU32 imageSize(ceil(extent.Size.Width), ceil(extent.Size.Height));

    if(m_TextureRenderer != NULL)
    {
        Size2DU32 cacheSize = m_TextureRenderer->GetSize();
        if((cacheSize.Width != imageSize.Width) || 
                (cacheSize.Height != imageSize.Height))
        {
            FIRTREE_SAFE_RELEASE(m_TextureRenderer);
            m_TextureRenderer = NULL;
            FIRTREE_SAFE_RELEASE(m_GLRenderer);
            m_GLRenderer = NULL;
        } else if(m_TextureRenderer->GetParentContext() != ctx) {
            FIRTREE_SAFE_RELEASE(m_TextureRenderer);
            m_TextureRenderer = NULL;
            FIRTREE_SAFE_RELEASE(m_GLRenderer);
            m_GLRenderer = NULL;
        }
    }

    ctx->Begin();

    if(m_TextureRenderer == NULL)
    {
        FIRTREE_SAFE_RELEASE(m_GLRenderer);
        m_TextureRenderer = RenderTextureContext::Create(imageSize.Width,
                imageSize.Height, ctx);
    }

    m_GLRenderer = GLRenderer::Create(m_TextureRenderer);
    m_TextureRenderer->Begin();
    m_GLRenderer->Clear(0,0,0,0);
    m_GLRenderer->RenderAtPoint(this, Point2D(0,0), extent);
    m_TextureRenderer->End();

    ctx->End();

    FIRTREE_SAFE_RELEASE(m_GLRenderer);

    return m_TextureRenderer->GetOpenGLTexture();
}

//=============================================================================
// BITMAP IMAGE
//=============================================================================

//=============================================================================
BitmapImageImpl::BitmapImageImpl(const Firtree::BitmapImageRep* imageRep, bool copy)
    :   BitmapBackedImageImpl()
{
    if(imageRep->ImageBlob == NULL) { return; }
    if(imageRep->Stride < imageRep->Width) { return; }
    if(imageRep->Stride*imageRep->Height > imageRep->ImageBlob->GetLength()) {
        return; 
    }

    m_BitmapRep = BitmapImageRep::CreateFromBitmapImageRep(imageRep, copy);
}

//=============================================================================
BitmapImageImpl::~BitmapImageImpl()
{
    FIRTREE_SAFE_RELEASE(m_BitmapRep);
}

//=============================================================================
Rect2D BitmapImageImpl::GetExtent() const
{
    // Get the size of the underlying pixel representation.
    Size2D pixelSize = GetUnderlyingPixelSize();

    // Is this image 'infinite' in extent?
    if(Size2D::IsInfinite(pixelSize))
    {
        // Return an 'infinite' extent
        return Rect2D::MakeInfinite();
    }

    // Form a rectangle which covers the underlying pixel
    // representation
    Rect2D pixelRect = Rect2D(Point2D(0,0), pixelSize);

    AffineTransform* transform = GetTransformFromUnderlyingImage();

    Rect2D extentRect = Rect2D::Transform(pixelRect, transform);

    FIRTREE_SAFE_RELEASE(transform);

    return extentRect;
}

//=============================================================================
Size2D BitmapImageImpl::GetUnderlyingPixelSize() const
{
    return Size2D(m_BitmapRep->Width, m_BitmapRep->Height);
}

//=============================================================================
AffineTransform* BitmapImageImpl::GetTransformFromUnderlyingImage() const
{
    return AffineTransform::Identity();
}

//=============================================================================
Firtree::BitmapImageRep* BitmapImageImpl::GetAsBitmapImageRep()
{
    return m_BitmapRep;
}

//=============================================================================
// IMAGE PROVIDER IMAGE
//=============================================================================

//=============================================================================
ImageProviderImageImpl::ImageProviderImageImpl(ImageProvider* improv)
    :   BitmapBackedImageImpl()
    ,   m_ImageProvider(improv)
{
    FIRTREE_SAFE_RETAIN(m_ImageProvider);
}

//=============================================================================
ImageProviderImageImpl::~ImageProviderImageImpl()
{
    FIRTREE_SAFE_RELEASE(m_ImageProvider);
}

//=============================================================================
Rect2D ImageProviderImageImpl::GetExtent() const
{
    // Get the size of the underlying pixel representation.
    Size2D pixelSize = GetUnderlyingPixelSize();

    // Is this image 'infinite' in extent?
    if(Size2D::IsInfinite(pixelSize))
    {
        // Return an 'infinite' extent
        return Rect2D::MakeInfinite();
    }

    // Form a rectangle which covers the underlying pixel
    // representation
    Rect2D pixelRect = Rect2D(Point2D(0,0), pixelSize);

    AffineTransform* transform = GetTransformFromUnderlyingImage();

    Rect2D extentRect = Rect2D::Transform(pixelRect, transform);

    FIRTREE_SAFE_RELEASE(transform);

    return extentRect;
}

//=============================================================================
Size2D ImageProviderImageImpl::GetUnderlyingPixelSize() const
{
    return m_ImageProvider->GetImageSize();
}

//=============================================================================
AffineTransform* ImageProviderImageImpl::GetTransformFromUnderlyingImage() const
{
    return AffineTransform::Identity();
}

//=============================================================================
Firtree::BitmapImageRep* ImageProviderImageImpl::GetAsBitmapImageRep()
{
    return const_cast<Firtree::BitmapImageRep*>(m_ImageProvider->GetImageRep());
}

} } // namespace Firtree::Internal

//=============================================================================
// vim:sw=4:ts=4:cindent:et
//
