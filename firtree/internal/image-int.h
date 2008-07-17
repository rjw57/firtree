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

namespace Firtree { namespace Internal {

//=============================================================================
/// The internal implementation of the public Image class.
class ImageImpl : public Image
{
    protected:
        ///@{
        /// Protected constructors/destructors. Use the various Image
        /// factory functions instead.
        ImageImpl();
        ImageImpl(const Image* im, AffineTransform* t);
        ImageImpl(const BitmapImageRep& imageRep, bool copyData);
        ImageImpl(Kernel* k, ExtentProvider* extentProvider);
        ImageImpl(ImageProvider* imageProvider);
        virtual ~ImageImpl();
        ///@}

    public:
        // ====================================================================
        // CONSTRUCTION METHODS

        // ====================================================================
        // CONST METHODS

        virtual Rect2D GetExtent() const;

        /// Return the size of image that would be returned by 
        /// GetAsOpenGLTexture() or GetAsBitmapImageRep().
        Size2D GetUnderlyingPixelSize() const;

        /// Return true if there is a cached OpenGL texture ready to use.
        bool HasOpenGLTexture() const;

        /// Return true if there is a cached BitmapImageRep ready to use.
        bool HasBitmapImageRep() const;

        /// Return true if this image directly represents a kernel.
        bool HasKernel() const;

        /// Returns the kernel represented by this image or NULL if there
        /// is none.
        Kernel* GetKernel() const;

        /// Return a pointer to an AffineTransform which represents the
        /// transfrom from the underlying pixel representation to this image.
        /// \note THIS POINTER MUST HAVE RELEASE CALLED ON IT AFTERWARDS.
        AffineTransform* GetTransformFromUnderlyingImage() const;

        // ====================================================================
        // MUTATING METHODS

        virtual BitmapImageRep WriteToBitmapData();

        /// These functions can potentially mutate the class since they will
        /// convert the image to the requested type 'on the fly'.
        ///@{
        
        /// Return an OpenGL texture object handle containing this image.
        unsigned int GetAsOpenGLTexture();

        /// Return a pointer to a BitmapImageRep structure containing this
        /// image.
        BitmapImageRep* GetAsBitmapImageRep();
        
        ///@}

    protected:
        /// The image representation as a binary blob or NULL if there is 
        /// none yet.
        BitmapImageRep*     m_BitmapRep;

        /// The image representation as an OpenGL texture or 0 if there is 
        /// none yet.
        unsigned int        m_GLTexture;

        ///@{
        /// If this image is actually the result of transforming another
        /// image, record the original image and the transform which maps
        /// it's pixels to this image's pixels.
        ImageImpl*          m_BaseImage;
        AffineTransform*    m_BaseTransform;
        ///@}
        
        /// The kernel encapsulated by this image.
        Kernel*             m_Kernel;

        /// The image provider for this image.
        ImageProvider*      m_ImageProvider;

        ExtentProvider*     m_ExtentProvider;

        friend class Firtree::Image;
};

} }

//=============================================================================
#endif // FIRTREE_IMAGE_INT_H
//=============================================================================

//=============================================================================
// vim:sw=4:ts=4:cindent:et

