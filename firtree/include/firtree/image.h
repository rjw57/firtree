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
/// A structure encapsulating a bitmap image representation. A bitmap is an
/// array of component values in red, green, blue and alpha order. 
/// Components are arranged in the array one after another forming a 
/// four component 'pixel block'. Blocks are packed in the array in 
/// left-to-right scanlines. The scanlines are packed in bottom-to-top order.
/// This mirrors the format taken by glTexImage2D() and it's ilk.
struct BitmapImageRep {
    /// An enumeration for specifying the pixel format of a BitmapImageRep.
    enum PixelFormat { 
        Byte,
        Float,
    };

    Blob*           ImageBlob;  ///< A Blob containing the image data.
    unsigned int    Width;      ///< The width of the image in pixels.
    unsigned int    Height;     ///< The height of the image in pixels.
    unsigned int    Stride;     ///< The length of one image row in bytes.
    PixelFormat     Format;     ///< The image representation uses elements
                                ///< of this type to encode pixel component values.

    /// Constructor for a BitmapImageRep.
    ///
    /// \param blob The Blob object containing the image data.
    /// \param width The width of the image in pixels.
    /// \param height The height of the image in pixels.
    /// \param stride The number of bytes in one row of the image.
    /// \param format The format of one component of one pixel.
    /// \param copyData If true, a deep copy of the data is made otherwise
    ///                 the reference count of the blob is merely incremented.
    BitmapImageRep(Blob* blob,
                unsigned int width, unsigned int height,
                unsigned int stride,
                PixelFormat format = Byte,
                bool copyData = false);

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
        Image(Kernel* kernel, ExtentProvider* extentProvider);
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

        /// Construct an image from the output of a kernel. If extentProvider
        /// is non-NULL, the specified ExtentProvider will be used to calculate
        /// the extent of the kernel. Otherwise, the extent provider
        /// returned from CreateStandardExtentProvider() is used.
        static Image* CreateFromKernel(Firtree::Kernel* k,
                Firtree::ExtentProvider* extentProvider = NULL);

        /// Construct an image from an image provider.
        static Image* CreateFromImageProvider(ImageProvider* improv);

        // ====================================================================
        // CONST METHODS

        /// Return a copy of this image.
        Image* Copy() const;

        /// Return the extent of this image (i.e. a rectangle covering all non
        /// transparent pixels).
        virtual Rect2D GetExtent() const = 0;

        // ====================================================================
        // MUTATING METHODS

        /// Write a copy of this image to a bitmap and return a BitmapImageRep
        /// structure describing this data. Returns an empty 0x0 image should
        /// this image be infinite in extent.
        /// Non-const because implementations may wish to cache the result.
        virtual BitmapImageRep WriteToBitmapData() = 0;

        /// Write a copy of this image a file. If this image is infinite or
        /// the file could not be writte, this returns false. Otherwise it
        /// returns true.
        /// Non-const because implementations may wish to cache the result.
        bool WriteToFile(const char* pFileName);


    protected:
};

}

//=============================================================================
#endif // FIRTREE_IMAGE_H
//=============================================================================

//=============================================================================
// vim:sw=4:ts=4:cindent:et

