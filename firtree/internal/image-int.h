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
/// \file image-int.h The internal interface to a FIRTREE image.
//=============================================================================

//=============================================================================
#ifndef FIRTREE_IMAGE_INT_H
#define FIRTREE_IMAGE_INT_H
//=============================================================================

#include <stdlib.h>

#include <firtree/main.h>
#include <firtree/math.h>
#include <firtree/blob.h>
#include <firtree/image.h>
#include <internal/render-to-texture.h>

namespace Firtree { namespace Internal {

//=============================================================================
/// The internal implementation of the public Image class. Abstract base class.
class ImageImpl : public Image
{
    public:
        /// The possible 'preferred' representations for an image.
        enum PreferredRepresentation {
            NoRepresentation,
            OpenGLTexture,
            BitmapImageRep,
            Kernel,
        };

    protected:
        ImageImpl();

    public:
        // ====================================================================
        // CONSTRUCTION METHODS

        virtual ~ImageImpl();

        // ====================================================================
        // CONST METHODS
        
        /// Return the 'preferred' representation for an image.
        virtual PreferredRepresentation GetPreferredRepresentation() const = 0;

        /// Return the extent of the image.
        virtual Rect2D GetExtent() const = 0;

        /// Return the size of image that would be returned by 
        /// GetAsOpenGLTexture() or GetAsBitmapImageRep().
        virtual Size2D GetUnderlyingPixelSize() const = 0;

        /// Return a pointer to an AffineTransform which represents the
        /// transfrom from the underlying pixel representation to this image.
        /// \note THIS POINTER MUST HAVE RELEASE CALLED ON IT AFTERWARDS.
        virtual AffineTransform* GetTransformFromUnderlyingImage() const = 0;
        
        /// Return a pointer to the kernel which generates this image or
        /// NULL if theres is none.
        virtual Firtree::Kernel* GetKernel() const;

        // ====================================================================
        // MUTATING METHODS

        /// These functions can potentially mutate the class since they will
        /// convert the image to the requested type 'on the fly'.
        ///@{
        
        /// Return an OpenGL texture object handle containing this image for
        /// the specified OpenGL context.
        virtual unsigned int GetAsOpenGLTexture(OpenGLContext* ctx) = 0;

        /// Return a pointer to a BitmapImageRep structure containing this
        /// image.
        virtual Firtree::BitmapImageRep* GetAsBitmapImageRep() = 0;

        ///@}

        friend class Firtree::Image;
};

//=============================================================================
/// The an image which is simply a transformed version of another image.
class TransformedImageImpl : public ImageImpl
{
    public:
        // ====================================================================
        // CONSTRUCTION METHODS

        TransformedImageImpl(const Image* baseImage, AffineTransform* transform);
        virtual ~TransformedImageImpl();

        // ====================================================================
        // CONST METHODS
        
        virtual ImageImpl::PreferredRepresentation GetPreferredRepresentation() const;
        virtual Rect2D GetExtent() const;
        virtual Size2D GetUnderlyingPixelSize() const;
        virtual AffineTransform* GetTransformFromUnderlyingImage() const;

        /// Get the underlying non-transformed image.
        Image*  GetBaseImage() const;

        // ====================================================================
        // MUTATING METHODS

        virtual unsigned int GetAsOpenGLTexture(OpenGLContext* ctx);
        virtual Firtree::BitmapImageRep* GetAsBitmapImageRep();

    private:
        AffineTransform*        m_Transform;
        ImageImpl*              m_BaseImage;
};

//=============================================================================
/// The an image which is backed by a texture.
class TextureBackedImageImpl : public ImageImpl
{
     public:
        // ====================================================================
        // CONSTRUCTION METHODS

        TextureBackedImageImpl();
        virtual ~TextureBackedImageImpl();

        // ====================================================================
        // CONST METHODS
        
        virtual ImageImpl::PreferredRepresentation GetPreferredRepresentation() const;

        // ====================================================================
        // MUTATING METHODS

        virtual Firtree::BitmapImageRep* GetAsBitmapImageRep();

    private:
        Firtree::BitmapImageRep*     m_BitmapRep;
};

//=============================================================================
/// The an image which is backed by a bitmap image rep.
class BitmapBackedImageImpl : public ImageImpl
{
     public:
        // ====================================================================
        // CONSTRUCTION METHODS

        BitmapBackedImageImpl();
        virtual ~BitmapBackedImageImpl();

        // ====================================================================
        // CONST METHODS
        
        virtual ImageImpl::PreferredRepresentation GetPreferredRepresentation() const;

        // ====================================================================
        // MUTATING METHODS

        virtual unsigned int GetAsOpenGLTexture(OpenGLContext* ctx);

    private:
        unsigned int        m_GLTexture;
        OpenGLContext*      m_GLContext;
        Size2DU32           m_TexSize;
};

//=============================================================================
/// The an image which is specified by a kernel.
class KernelImageImpl : public TextureBackedImageImpl
{
    public:
        // ====================================================================
        // CONSTRUCTION METHODS

        KernelImageImpl(Firtree::Kernel* k, ExtentProvider* extentProvider);
        virtual ~KernelImageImpl();

        // ====================================================================
        // CONST METHODS
        
        virtual ImageImpl::PreferredRepresentation GetPreferredRepresentation() const;
        virtual Rect2D GetExtent() const;
        virtual Size2D GetUnderlyingPixelSize() const;
        virtual AffineTransform* GetTransformFromUnderlyingImage() const;

        virtual Firtree::Kernel* GetKernel() const;

        // ====================================================================
        // MUTATING METHODS

        virtual unsigned int GetAsOpenGLTexture(OpenGLContext* ctx);

    private:
        Firtree::Kernel*    m_Kernel;
        ExtentProvider*     m_ExtentProvider;

        RenderTextureContext*   m_TextureRenderer;
        GLRenderer*             m_GLRenderer;
};

//=============================================================================
/// The an image which is specified by a bitmap image.
class BitmapImageImpl : public BitmapBackedImageImpl
{
    public:
        // ====================================================================
        // CONSTRUCTION METHODS

        BitmapImageImpl(const Firtree::BitmapImageRep* imageRep, bool copy);
        virtual ~BitmapImageImpl();

        // ====================================================================
        // CONST METHODS
        
        virtual Rect2D GetExtent() const;
        virtual Size2D GetUnderlyingPixelSize() const;
        virtual AffineTransform* GetTransformFromUnderlyingImage() const;

        // ====================================================================
        // MUTATING METHODS

        virtual Firtree::BitmapImageRep* GetAsBitmapImageRep();

    private:
        Firtree::BitmapImageRep*     m_BitmapRep;
};

//=============================================================================
/// The an image which is specified by a bitmap image provider
class ImageProviderImageImpl : public BitmapBackedImageImpl
{
    public:
        // ====================================================================
        // CONSTRUCTION METHODS

        ImageProviderImageImpl(ImageProvider* improv);
        virtual ~ImageProviderImageImpl();

        // ====================================================================
        // CONST METHODS
        
        virtual Rect2D GetExtent() const;
        virtual Size2D GetUnderlyingPixelSize() const;
        virtual AffineTransform* GetTransformFromUnderlyingImage() const;

        // ====================================================================
        // MUTATING METHODS

        virtual Firtree::BitmapImageRep* GetAsBitmapImageRep();

    private:
        ImageProvider*     m_ImageProvider;
};

} }

//=============================================================================
#endif // FIRTREE_IMAGE_INT_H
//=============================================================================

//=============================================================================
// vim:sw=4:ts=4:cindent:et

