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

#include <firtree/include/main.h>
#include <firtree/include/math.h>
#include <firtree/include/blob.h>

namespace Firtree {

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
        Image(const Image& im);
        Image(Blob* blob,
                unsigned int width, unsigned int height,
                unsigned int stride, bool copy);
        virtual ~Image();
        ///@}

    public:
        // ====================================================================
        // CONSTRUCTION METHODS

        /// Construct a copy of the image passed.
        static Image* CreateFromImage(const Image* im);

        /// Construct an image from a 8-bit/channel RGBA image in memory.
        /// 
        /// \param width The width of the image in pixels.
        /// \param height The height of the image in pixels.
        /// \param stride The number of bytes in one row of the image.
        /// \param copyData If true, a deep copy of the data is made otherwise
        ///                 the reference count of the blob is merely incremented.
        static Image* CreateFromBitmapData(Blob* blob, 
                unsigned int width, unsigned int height, 
                unsigned int stride,
                bool copyData = true);

        /// Construct an image encapsulating the contents of a file. Returns
        /// NULL if the file could not be read.
        static Image* CreateFromFile(const char* pFilename);

        // ====================================================================
        // CONST METHODS

        /// Return a copy of this image.
        Image* Copy() const;

        // ====================================================================
        // MUTATING METHODS

    protected:
        // The image representation as a binary blob.
        Blob*           m_BinaryRep;

        // The image representation as an OpenGL texture.
        unsigned int    m_GLTexture;
};

}

//=============================================================================
#endif // FIRTREE_IMAGE_H
//=============================================================================

//=============================================================================
// vim:sw=4:ts=4:cindent:et

