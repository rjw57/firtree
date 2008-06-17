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
/// \file image.cpp The implementation of the abstract FIRTREE image.
//=============================================================================

#include <public/include/main.h>
#include <public/include/image.h>

#ifdef FIRTREE_WIN32
#   include <wand/MagickWand.h>
#else
#   include <wand/magick-wand.h>
#endif

namespace Firtree {

//=============================================================================
Image::Image()
    :   ReferenceCounted()
    ,   m_BinaryRep(NULL)
    ,   m_GLTexture(0)
{
}

//=============================================================================
Image::Image(const Image& im)
{
    Image();
    if(im.m_BinaryRep != NULL)
    {
        m_BinaryRep = im.m_BinaryRep->Copy();
    }
    m_GLTexture = im.m_GLTexture;
}

//=============================================================================
Image::Image(Blob* blob,
        unsigned int width, unsigned int height, 
        unsigned int stride, bool copy)
{
    Image();
    if(stride < width) { return; }
    if(stride*height > blob->GetLength()) { return; }

    if(copy) {
        m_BinaryRep = blob->Copy();
    } else {
        blob->Retain();
        m_BinaryRep = blob;
    }
}

//=============================================================================
Image::~Image()
{
    FIRTREE_SAFE_RELEASE(m_BinaryRep);
}

//=============================================================================
Image* Image::CreateFromImage(const Image* im)
{
    if(im == NULL) { return NULL; }
    return new Image(*im);
}

//=============================================================================
Image* Image::CreateFromBitmapData(Blob* blob, 
                unsigned int width, unsigned int height, 
                unsigned int stride, bool copy)
{
    if(blob == NULL) { return NULL; }
    if(stride < width) { return NULL; }
    if(stride*height > blob->GetLength()) { return NULL; }
    return new Image(blob, width, height, stride, copy);
}

//=============================================================================
Image* Image::CreateFromFile(const char* pFilename)
{
    static bool calledGenesis = false;

    if(pFilename == NULL) { return NULL; }
    
    if(!calledGenesis)
    {
        // FIXME: How to know when to call Terminus?
        MagickWandGenesis();
        calledGenesis = true;
    }

    MagickWand* wand = NewMagickWand();
    MagickBooleanType status = MagickReadImage(wand, pFilename);
    if(status == MagickFalse)
    {
        return NULL;
    }

    MagickFlipImage(wand);

    unsigned int w = MagickGetImageWidth(wand);
    unsigned int h = MagickGetImageHeight(wand);

    Blob* image = Blob::CreateWithLength(w*h*4);

    PixelIterator* pixit = NewPixelIterator(wand);
    if(pixit == NULL)
    {
        wand = DestroyMagickWand(wand);
        FIRTREE_SAFE_RELEASE(image);
        return NULL;
    }

    uint8_t* curPixel = const_cast<uint8_t*>(image->GetBytes());
    for(unsigned int y=0; y<h; y++)
    {
        long unsigned int rowWidth;
        PixelWand** pixels = PixelGetNextIteratorRow(pixit, &rowWidth);
        if(pixels == NULL)
        {
            pixit = DestroyPixelIterator(pixit);
            wand = DestroyMagickWand(wand);
            FIRTREE_SAFE_RELEASE(image);
            return NULL;
        }
        if(rowWidth != w)
        {
            FIRTREE_ERROR("rowWidth != w");
            pixit = DestroyPixelIterator(pixit);
            wand = DestroyMagickWand(wand);
            FIRTREE_SAFE_RELEASE(image);
            return NULL;
        }

        for(unsigned int x=0; x<rowWidth; x++)
        {
            // Convert the image to pre-multiplied alpha.
            
            float alpha = PixelGetAlpha(pixels[x]);
            curPixel[0] = (unsigned char)(255.0 * alpha * PixelGetRed(pixels[x]));
            curPixel[1] = (unsigned char)(255.0 * alpha * PixelGetGreen(pixels[x]));
            curPixel[2] = (unsigned char)(255.0 * alpha * PixelGetBlue(pixels[x]));
            curPixel[3] = (unsigned char)(255.0 * alpha);

            curPixel += 4;
        }
    }
    
    pixit = DestroyPixelIterator(pixit);

    Image* retVal = CreateFromBitmapData(image, w, h, w*4, false);

    FIRTREE_SAFE_RELEASE(image);

    return retVal;
}

//=============================================================================
Image* Image::Copy() const
{
    return Image::CreateFromImage(this);
}

} // namespace Firtree 

//=============================================================================
// vim:sw=4:ts=4:cindent:et
//
