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

#include <firtree/main.h>
#include <firtree/image.h>
#include <internal/image-int.h>

#include <wand/magick_wand.h>
#include <assert.h>

namespace Firtree {

using namespace Internal;

//=============================================================================
BitmapImageRep::BitmapImageRep(Blob* blob,
    unsigned int width, unsigned int height, unsigned int stride,
    PixelFormat format, bool copy)
    :   Width(width)
    ,   Height(height)
    ,   Stride(stride)
    ,   Format(format)
{
    if(blob == NULL)
        return;
    
    if(copy) 
    {
        ImageBlob = blob->Copy();
    } else {
        blob->Retain();
        ImageBlob = blob;
    }
}

//=============================================================================
BitmapImageRep::BitmapImageRep(const BitmapImageRep& rep, bool copy)
    :   Width(rep.Width)
    ,   Height(rep.Height)
    ,   Stride(rep.Stride)
    ,   Format(rep.Format)
{
    if(rep.ImageBlob == NULL)
        return;

    if(copy) 
    {
        ImageBlob = rep.ImageBlob->Copy();
    } else {
        rep.ImageBlob->Retain();
        ImageBlob = rep.ImageBlob;
    }
}

//=============================================================================
BitmapImageRep::~BitmapImageRep()
{
    FIRTREE_SAFE_RELEASE(ImageBlob);
}

//=============================================================================
Image::Image()
    :   ReferenceCounted()
{
}

//=============================================================================
Image::Image(const Image* im, AffineTransform* t)
    :   ReferenceCounted()
{
}

//=============================================================================
Image::Image(const BitmapImageRep& imageRep, bool copy)
    :   ReferenceCounted()
{
}

//=============================================================================
Image::Image(Kernel* k, ExtentProvider* extentProvider)
    :   ReferenceCounted()
{
}

//=============================================================================
Image::Image(ImageProvider* improv)
    :   ReferenceCounted()
{
}

//=============================================================================
Image::~Image()
{
}

//=============================================================================
Image* Image::CreateFromImage(const Image* im)
{
    if(im == NULL) { return NULL; }
    return new ImageImpl(im, AffineTransform::Identity());
}

//=============================================================================
Image* Image::CreateFromImageWithTransform(const Image* im, AffineTransform* t)
{
    if(im == NULL) { return NULL; }
    return new ImageImpl(im, t);
}

//=============================================================================
Image* Image::CreateFromBitmapData(const BitmapImageRep& imageRep,
                bool copyData)
{
    if(imageRep.ImageBlob == NULL) { return NULL; }
    if(imageRep.Stride < imageRep.Width) { return NULL; }
    if(imageRep.Stride*imageRep.Height > imageRep.ImageBlob->GetLength()) 
    {
        return NULL; 
    }
    return new ImageImpl(imageRep, copyData);
}

//=============================================================================
Image* Image::CreateFromFile(const char* pFileName)
{
    if(pFileName == NULL) { return NULL; }

    MagickWand* wand = NewMagickWand();
    MagickBooleanType status = MagickReadImage(wand, pFileName);
    if(status == MagickFalse)
    {
        FIRTREE_WARNING("Could not read image from %s.", pFileName);
        wand = DestroyMagickWand(wand);
        return false;
    }

    // MagickFlipImage(wand);

    unsigned int w = MagickGetImageWidth(wand);
    unsigned int h = MagickGetImageHeight(wand);

    Blob* imageBlob = Blob::CreateWithLength(w*h*4);

    MagickGetImagePixels(wand, 0, 0, w, h, "RGBA", CharPixel, 
            const_cast<uint8_t*>(imageBlob->GetBytes()));

    uint8_t* pixel = const_cast<uint8_t*>(imageBlob->GetBytes());
    for(int i=0; i<w*h; i++)
    {
        float alpha = ((float)(pixel[3]) / 255.0f);
        pixel[0] *= alpha;
        pixel[1] *= alpha;
        pixel[2] *= alpha;
        pixel += 4;
    }
    
    wand = DestroyMagickWand(wand);

    Image* rv = Image::CreateFromBitmapData(
        BitmapImageRep(imageBlob, w, h, w*4, BitmapImageRep::Byte, false), false);

    imageBlob->Release();

    return rv;
}

//=============================================================================
Image* Image::CreateFromKernel(Kernel* k, ExtentProvider* extentProvider)
{
    if(k == NULL) { return NULL; }

    if(extentProvider != NULL)
    {
        return new ImageImpl(k, extentProvider);
    }

    extentProvider = ExtentProvider::CreateStandardExtentProvider();
    Image* retVal = new ImageImpl(k, extentProvider);
    FIRTREE_SAFE_RELEASE(extentProvider);

    return retVal;
}

//=============================================================================
Image* Image::CreateFromImageProvider(ImageProvider* improv)
{
    if(improv == NULL) { return NULL; }
    return new ImageImpl(improv);
}

//=============================================================================
bool Image::WriteToFile(const char* pFileName) 
{
    if(pFileName == NULL) { return false; }

    Rect2D extent = GetExtent();

    if(Rect2D::IsInfinite(extent)) { return false; }

    MagickWand* wand = NewMagickWand();
    assert(wand != NULL);
    
    PixelWand* bg_pxl_wnd = NewPixelWand();
    PixelSetColor( bg_pxl_wnd, "black" );

    BitmapImageRep imageRep = WriteToBitmapData();

    if((imageRep.Width == 0) || (imageRep.Height == 0))
    {
        wand = DestroyMagickWand(wand);
        return false;
    }

    MagickBooleanType status = MagickNewImage(wand, imageRep.Width,
            imageRep.Height, bg_pxl_wnd);
    if(status == MagickFalse)
    {
        FIRTREE_WARNING("Could not create image for writing.");
        wand = DestroyMagickWand(wand);
        return false;
    }

    bg_pxl_wnd = DestroyPixelWand(bg_pxl_wnd);

    Blob* outputBufferBlob = 
        Blob::CreateWithLength(imageRep.Width * imageRep.Height * 4);

    uint8_t* outBuf = const_cast<uint8_t*>(outputBufferBlob->GetBytes());
    uint8_t* inBuf = const_cast<uint8_t*>(imageRep.ImageBlob->GetBytes());

    for(unsigned int row=0; row < imageRep.Height; row++)
    {
        uint8_t* outRow = outBuf + (row * imageRep.Width * 4);
        uint8_t* inRow = inBuf + (row * imageRep.Stride);

        float* inFloatRow = reinterpret_cast<float*>(inRow);

        for(unsigned int col=0; col < imageRep.Width; col++)
        {
            float r,g,b,a;

            if(imageRep.Format == BitmapImageRep::Float)
            {
                r = inFloatRow[(col<<2)];
                g = inFloatRow[(col<<2)+1];
                b = inFloatRow[(col<<2)+2];
                a = inFloatRow[(col<<2)+3];
            } else if(imageRep.Format == BitmapImageRep::Byte) {
                r = (1.f/255.f) * inRow[(col<<2)];
                g = (1.f/255.f) * inRow[(col<<2)+1];
                b = (1.f/255.f) * inRow[(col<<2)+2];
                a = (1.f/255.f) * inRow[(col<<2)+3];
            } else {
                FIRTREE_ERROR("Unknown bitmap image rep format: %i", imageRep.Format);
            }

            // Un pre-multiply
            if(a > 0.f)
            {
                r /= a; g /= a; b /= a;
            }

            uint8_t rc = static_cast<uint8_t>(255.f * r);
            uint8_t gc = static_cast<uint8_t>(255.f * g);
            uint8_t bc = static_cast<uint8_t>(255.f * b);
            uint8_t ac = static_cast<uint8_t>(255.f * a);

            outRow[(col<<2)] = rc;
            outRow[(col<<2)+1] = gc;
            outRow[(col<<2)+2] = bc;
            outRow[(col<<2)+3] = ac;
        }
    }

    MagickSetImagePixels(wand, 0, 0, imageRep.Width,
            imageRep.Height, "RGBA", CharPixel, 
            const_cast<uint8_t*>(outputBufferBlob->GetBytes()));

    FIRTREE_SAFE_RELEASE(outputBufferBlob);

    bool retVal = true;

    status = MagickWriteImage(wand, pFileName);
    if(status == MagickFalse)
    {
        FIRTREE_WARNING("Could not write image to '%s'.", pFileName);
        retVal = false;
    }
    
    wand = DestroyMagickWand(wand);

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
