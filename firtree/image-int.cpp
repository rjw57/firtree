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
    ,   m_PreferredRepresentation(NoRepresentation)
    ,   m_BitmapRep(NULL)
    ,   m_GLTexture(0)
    ,   m_GLTextureCtx(NULL)
    ,   m_BaseImage(NULL)
    ,   m_BaseTransform(NULL)
    ,   m_Kernel(NULL)
    ,   m_ImageProvider(NULL)
    ,   m_ExtentProvider(NULL)
    ,   m_TextureRenderer(NULL)
{   
}

//=============================================================================
ImageImpl::ImageImpl(const Image* inim, AffineTransform* t)
    :   Image(inim, t)
    ,   m_PreferredRepresentation(NoRepresentation)
    ,   m_BitmapRep(NULL)
    ,   m_GLTexture(0)
    ,   m_GLTextureCtx(NULL)
    ,   m_BaseImage(NULL)
    ,   m_BaseTransform(NULL)
    ,   m_Kernel(NULL)
    ,   m_ImageProvider(NULL)
    ,   m_ExtentProvider(NULL)
    ,   m_TextureRenderer(NULL)
{
    const ImageImpl* im = dynamic_cast<const ImageImpl*>(inim);

    if(im == NULL)
    {
        return;
    }

    m_BaseImage = const_cast<ImageImpl*>(im);
    m_BaseImage->Retain();

    m_PreferredRepresentation = m_BaseImage->GetPreferredRepresentation();

    m_BaseTransform = t->Copy();
}

//=============================================================================
ImageImpl::ImageImpl(const Firtree::BitmapImageRep* imageRep, bool copy)
    :   Image(imageRep, copy)
    ,   m_PreferredRepresentation(BitmapImageRep)
    ,   m_BitmapRep(NULL)
    ,   m_GLTexture(0)
    ,   m_GLTextureCtx(NULL)
    ,   m_BaseImage(NULL)
    ,   m_BaseTransform(NULL)
    ,   m_Kernel(NULL)
    ,   m_ImageProvider(NULL)
    ,   m_ExtentProvider(NULL)
    ,   m_TextureRenderer(NULL)
{
    if(imageRep->ImageBlob == NULL) { return; }
    if(imageRep->Stride < imageRep->Width) { return; }
    if(imageRep->Stride*imageRep->Height > imageRep->ImageBlob->GetLength()) {
        return; 
    }

    m_BitmapRep = BitmapImageRep::CreateFromBitmapImageRep(imageRep, copy);
}

//=============================================================================
ImageImpl::ImageImpl(Kernel* k, ExtentProvider* extentProvider)
    :   Image(k, extentProvider)
    ,   m_PreferredRepresentation(OpenGLTexture)
    ,   m_BitmapRep(NULL)
    ,   m_GLTexture(0)
    ,   m_GLTextureCtx(NULL)
    ,   m_BaseImage(NULL)
    ,   m_BaseTransform(NULL)
    ,   m_Kernel(k)
    ,   m_ImageProvider(NULL)
    ,   m_ExtentProvider(extentProvider)
    ,   m_TextureRenderer(NULL)
{
    if(m_Kernel != NULL)
    {
        m_Kernel->Retain();
    }

    FIRTREE_SAFE_RETAIN(m_ExtentProvider);
}

//=============================================================================
ImageImpl::ImageImpl(ImageProvider* improv)
    :   Image(improv)
    ,   m_PreferredRepresentation(BitmapImageRep)
    ,   m_BitmapRep(NULL)
    ,   m_GLTexture(0)
    ,   m_GLTextureCtx(NULL)
    ,   m_BaseImage(NULL)
    ,   m_BaseTransform(NULL)
    ,   m_Kernel(NULL)
    ,   m_ImageProvider(improv)
    ,   m_ExtentProvider(NULL)
    ,   m_TextureRenderer(NULL)
{
    FIRTREE_SAFE_RETAIN(m_ImageProvider);
}


//=============================================================================
ImageImpl::~ImageImpl()
{
    if(m_GLTexture != 0)
    {
        m_GLTextureCtx->DeleteTexture(m_GLTexture);
        m_GLTexture = 0;
    }
    
    FIRTREE_SAFE_RELEASE(m_GLTextureCtx);

    FIRTREE_SAFE_RELEASE(m_BitmapRep);
    FIRTREE_SAFE_RELEASE(m_ImageProvider);
    FIRTREE_SAFE_RELEASE(m_BaseImage);
    FIRTREE_SAFE_RELEASE(m_BaseTransform);
    FIRTREE_SAFE_RELEASE(m_Kernel);
    FIRTREE_SAFE_RELEASE(m_ExtentProvider);
    FIRTREE_SAFE_RELEASE(m_TextureRenderer);
}

//=============================================================================
ImageImpl::PreferredRepresentation ImageImpl::GetPreferredRepresentation() const
{
    return m_PreferredRepresentation;
}

//=============================================================================
Size2D ImageImpl::GetUnderlyingPixelSize() const
{
    if(m_ImageProvider != NULL)
    {
        return m_ImageProvider->GetImageSize();
    }

    if(m_BaseImage != NULL)
    {
        return m_BaseImage->GetUnderlyingPixelSize();
    }

    // Do we have a bitmap representation?
    if(HasBitmapImageRep())
    {
        return Size2D(m_BitmapRep->Width, m_BitmapRep->Height);
    }

    // Do we have an OpenGL texture representation?
    if(HasOpenGLTexture())
    {
        GLint w, h;
        glBindTexture(GL_TEXTURE_2D, m_GLTexture);
        glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &w);
        glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_HEIGHT, &h);

        return Size2D(w,h);
    }

    return Size2D::MakeInfinite();
}


//=============================================================================
AffineTransform* ImageImpl::GetTransformFromUnderlyingImage() const
{
    if(m_BaseImage != NULL)
    {
        AffineTransform* baseTransform = 
            m_BaseImage->GetTransformFromUnderlyingImage();
        baseTransform->AppendTransform(m_BaseTransform);
        return baseTransform;
    }

    return AffineTransform::Identity();
}

//=============================================================================
Rect2D ImageImpl::GetExtent() const
{
    if(m_BaseImage != NULL)
    {
        Rect2D baseExtent = m_BaseImage->GetExtent();
        Rect2D extentRect = Rect2D::Transform(baseExtent, m_BaseTransform);
        return extentRect;
    }

    // Is this a kernel image?
    if(HasKernel())
    {
        if(m_ExtentProvider == NULL)
        {
            return Rect2D::MakeInfinite();
        }

        return m_ExtentProvider->ComputeExtentForKernel(m_Kernel);
    }
    
    // Get the size of the underlying pixel representation.
    Size2D pixelSize = GetUnderlyingPixelSize();

    // Is this image 'infinite' in extent?
    if(pixelSize.Width == -1)
    {
        // Return an 'infinite' extent
        return Rect2D(-0.5*FLT_MAX, -0.5*FLT_MAX, FLT_MAX, FLT_MAX);
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
bool ImageImpl::HasOpenGLTexture() const
{
    if(m_BaseImage != NULL)
    {
        return m_BaseImage->HasOpenGLTexture();
    }

    return m_GLTexture != 0;
}

//=============================================================================
bool ImageImpl::HasBitmapImageRep() const
{
    if(m_BaseImage != NULL)
    {
        return m_BaseImage->HasBitmapImageRep();
    }

    if(m_ImageProvider != NULL)
    {
        return true;
    }

    return (m_BitmapRep != NULL) && (m_BitmapRep->ImageBlob != NULL) &&
        (m_BitmapRep->ImageBlob->GetLength() > 0);
}

//=============================================================================
bool ImageImpl::HasKernel() const
{
    if(m_BaseImage != NULL)
    {
        return m_BaseImage->HasKernel();
    }

    return (m_Kernel != NULL);
}

//=============================================================================
Kernel* ImageImpl::GetKernel() const
{
    if(m_BaseImage != NULL)
    {
        return m_BaseImage->GetKernel();
    }

    return m_Kernel;
}

//=============================================================================
unsigned int ImageImpl::GetAsOpenGLTexture(OpenGLContext* ctx)
{
    if(m_BaseImage != NULL)
    {
        return m_BaseImage->GetAsOpenGLTexture(ctx);
    }

    // If the context we used to create the texture doesn't match
    // the one asked for, purge our cache.
    if((m_GLTexture != 0) && (ctx != m_GLTextureCtx))
    {
        m_GLTextureCtx->DeleteTexture(m_GLTexture);
        m_GLTexture = 0;
        FIRTREE_SAFE_RELEASE(m_GLTextureCtx);
    }

    // If we have already created a texture but are driven by an
    // image provider, update the texture.
    if(HasOpenGLTexture() && (m_ImageProvider != NULL))
    {
        Firtree::BitmapImageRep* bir = GetAsBitmapImageRep();

        CHECK_GL( glBindTexture(GL_TEXTURE_2D, m_GLTexture) );
        assert(bir->Stride == bir->Width*4);
        CHECK_GL( glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0,
                    bir->Width, bir->Height, 
                    GL_RGBA, GL_UNSIGNED_BYTE,
                    bir->ImageBlob->GetBytes()) );

        return m_GLTexture;
    }

    // Trivial case: we already have a GL representation.
    if(HasOpenGLTexture())
    {
        return m_GLTexture;
    }

    // We have to construct one. Try the binary rep first.
    if(HasBitmapImageRep())
    {
        Firtree::BitmapImageRep* bir = GetAsBitmapImageRep();

        m_GLTextureCtx = ctx;
        FIRTREE_SAFE_RETAIN(m_GLTextureCtx);

        m_GLTextureCtx->Begin();

        m_GLTexture = m_GLTextureCtx->GenTexture();

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

        m_GLTextureCtx->End();

        return m_GLTexture;
    }
    
    // If we get here, we have no way of constructing the texture.
    // Return 0 to signal this.
    return 0;
}

//=============================================================================
Firtree::BitmapImageRep* ImageImpl::GetAsBitmapImageRep()
{
    if(m_ImageProvider != NULL)
    {
        FIRTREE_SAFE_RELEASE(m_BitmapRep);
        m_BitmapRep = BitmapImageRep::CreateFromBitmapImageRep(
                m_ImageProvider->GetImageRep(), false);
        return m_BitmapRep;
    }

    if(m_BaseImage != NULL)
    {
        return m_BaseImage->GetAsBitmapImageRep();
    }

    // Trivial case where a bitmap rep exists
    if(HasBitmapImageRep())
    {
        return m_BitmapRep;
    }

    // If we have an OpenGL texture, use that.
    if(HasOpenGLTexture())
    {
        GLint w, h;
        glBindTexture(GL_TEXTURE_2D, m_GLTexture);
        glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &w);
        glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_HEIGHT, &h);

        Blob* imageBlob = Blob::CreateWithLength(w*h*4*sizeof(float));
        glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_FLOAT, 
                (void*)(imageBlob->GetBytes()));

        if(m_BitmapRep != NULL) { delete m_BitmapRep; }
        FIRTREE_SAFE_RELEASE(m_BitmapRep);
        m_BitmapRep = BitmapImageRep::Create(imageBlob,
                w, h, w*4*sizeof(float), Firtree::BitmapImageRep::Float, false);
        FIRTREE_SAFE_RELEASE(imageBlob);
        
        return m_BitmapRep;
    }

    if(HasKernel())
    {
        Rect2D extent = GetExtent();
        if(Rect2D::IsInfinite(extent))
        {
            FIRTREE_ERROR("Cannot render infinite extent image to texture.");
            return NULL;
        }

        Size2DU32 imageSize(ceil(extent.Size.Width), ceil(extent.Size.Height));

        OpenGLContext* c = GLSL::GetCurrentGLContext();

        if(c == NULL)
        {
            FIRTREE_ERROR("Attempt to render image to bitmap outside of an OpenGL "
                    "context.");
        }

        FIRTREE_SAFE_RETAIN(c);

        c->Begin();

        if(m_TextureRenderer == NULL)
        {
            m_TextureRenderer = RenderTextureContext::Create(imageSize.Width,
                    imageSize.Height, c);
        } else {
            Size2DU32 existingRendererSize = m_TextureRenderer->GetSize();
            if((existingRendererSize.Width != imageSize.Width) ||
                    (existingRendererSize.Height != imageSize.Height))
            {
                FIRTREE_SAFE_RELEASE(m_TextureRenderer);
                m_TextureRenderer = RenderTextureContext::Create(imageSize.Width,
                        imageSize.Height, c);
            }
        }

        m_TextureRenderer->Begin();
        GLRenderer* renderer = GLRenderer::Create(m_TextureRenderer);
        renderer->Clear(0,0,0,0);
        renderer->RenderAtPoint(this, Point2D(0,0), extent);
        FIRTREE_SAFE_RELEASE(renderer);
        m_TextureRenderer->End();

        GLint w, h;
        CHECK_GL( glBindTexture(GL_TEXTURE_2D, m_TextureRenderer->GetOpenGLTexture()) );
        CHECK_GL( glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &w) );
        CHECK_GL( glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_HEIGHT, &h) );

        Blob* imageBlob = Blob::CreateWithLength(w*h*4*sizeof(float));
        CHECK_GL( glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_FLOAT, 
                (void*)(imageBlob->GetBytes())) );

        c->End();
        FIRTREE_SAFE_RELEASE(c);

        FIRTREE_SAFE_RELEASE(m_BitmapRep);
        m_BitmapRep = BitmapImageRep::Create(imageBlob,
                w, h, w*4*sizeof(float), Firtree::BitmapImageRep::Float, false);
        FIRTREE_SAFE_RELEASE(imageBlob);
        
        return m_BitmapRep;
    }

    return NULL;
}

} } // namespace Firtree::Internal

//=============================================================================
// vim:sw=4:ts=4:cindent:et
//
