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
/// \file image.h The interface to a FIRTREE image.
//=============================================================================

//=============================================================================
#ifndef FIRTREE_IMAGE_H
#define FIRTREE_IMAGE_H
//=============================================================================

#include <stdlib.h>

#include <firtree/main.h>
#include <firtree/math.h>
#include <firtree/blob.h>
#include <firtree/kernel.h>

namespace Firtree {

//=============================================================================
/// A structure encapsulating a bitmap image representation.
struct BitmapImageRep {
    Blob*           ImageBlob;  ///< A Blob containing the image data.
    unsigned int    Width;      ///< The width of the image in pixels.
    unsigned int    Height;     ///< The height of the image in pixels.
    unsigned int    Stride;     ///< The length of one image row in bytes.

    /// Constructor for a BitmapImageRep.
    ///
    /// \param blob The Blob object containing the image data.
    /// \param width The width of the image in pixels.
    /// \param height The height of the image in pixels.
    /// \param stride The number of bytes in one row of the image.
    /// \param copyData If true, a deep copy of the data is made otherwise
    ///                 the reference count of the blob is merely incremented.
    BitmapImageRep(Blob* blob,
                unsigned int width, unsigned int height,
                unsigned int stride, bool copyData = false);

    /// Construct a BitmapImageRep by copying an existing BitmapImageRep.
    ///
    /// \param rep A reference to the BitmapImageRep containing the 
    ///            bitmap image.
    /// \param copyData If true, a deep copy of the bitmap is made.
    BitmapImageRep(const BitmapImageRep& rep, bool copyData = false);

    ~BitmapImageRep();
};

//=============================================================================
/// An abstract base class which can provide a bitmap image representation.
class ImageProvider : public ReferenceCounted
{
    public:
        ImageProvider() : ReferenceCounted() { }
        virtual ~ImageProvider() { }

        /// Return the size of the image (in pixels) which would be
        /// returned from GetImageRep().
        virtual Size2D GetImageSize() const = 0;

        /// Return a BitmapImageRep structure containing the image.
        virtual const BitmapImageRep GetImageRep() const = 0;
};

//=============================================================================
/// An image encapsulates all the information FIRTREE needs to render an
/// image on screen.
class Image : public ReferenceCounted
{
    protected:
        ///@{
        /// Protected constructors/destructors. Use the various Image
        /// factory functions instead.
        Image();
        Image(const Image* im, AffineTransform* t);
        Image(const BitmapImageRep& imageRep, bool copy);
        Image(Kernel* kernel);
        Image(ImageProvider* imageProvider);
        ///@}

    public:
        virtual ~Image();

        // ====================================================================
        // CONSTRUCTION METHODS

        /// Construct an image which is a copy of the image passed.
        /// It is equivalent to calling CreateFromImageWithTransform()
        /// with an identify transform.
        static Image* CreateFromImage(const Image* im);

        /// Construct an image from a 8-bit/channel RGBA image in memory.
        /// 
        /// \param imageRep The BitmapImageRep encapsulating the image.
        /// \param copyData If true, a deep copy of the data is made otherwise
        ///                 the reference count of the blob is merely incremented.
        static Image* CreateFromBitmapData(const BitmapImageRep& imageRep,
                bool copyData = true);

        /// Construct an image encapsulating the contents of a file. Returns
        /// NULL if the file could not be read.
        static Image* CreateFromFile(const char* pFilename);

        /// Construct an image by applying a transformation to each pixel in
        /// a source image.
        static Image* CreateFromImageWithTransform(const Image* im,
                AffineTransform* t);

        /// Construct an image from the output of a kernel.
        static Image* CreateFromKernel(Kernel* k);

        /// Construct an image from an image provider.
        static Image* CreateFromImageProvider(ImageProvider* improv);

        // ====================================================================
        // CONST METHODS

        /// Return a copy of this image.
        Image* Copy() const;

        // ====================================================================
        // MUTATING METHODS

    protected:
};

}

//=============================================================================
#endif // FIRTREE_IMAGE_H
//=============================================================================

//=============================================================================
// vim:sw=4:ts=4:cindent:et

