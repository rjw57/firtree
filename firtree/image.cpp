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
BitmapImageRep* BitmapImageRep::Create(Blob* blob,
    unsigned int width, unsigned int height, unsigned int stride,
    PixelFormat format, bool copy)
{
    return new BitmapImageRep(blob, width, height, stride, format, copy);
}

//=============================================================================
BitmapImageRep* BitmapImageRep::CreateFromBitmapImageRep(
        const BitmapImageRep* rep, bool copyData)
{
    return new BitmapImageRep(rep, copyData);
}

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
BitmapImageRep::BitmapImageRep(const BitmapImageRep* rep, bool copy)
    :   Width(rep->Width)
    ,   Height(rep->Height)
    ,   Stride(rep->Stride)
    ,   Format(rep->Format)
{
    if(rep->ImageBlob == NULL)
        return;

    if(copy) 
    {
        ImageBlob = rep->ImageBlob->Copy();
    } else {
        FIRTREE_SAFE_RETAIN(rep->ImageBlob);
        ImageBlob = rep->ImageBlob;
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
Image::~Image()
{
}

//=============================================================================
Image* Image::CreateFromImage(const Image* im)
{
    if(im == NULL) { return NULL; }
    return new TransformedImageImpl(im, AffineTransform::Identity(),
                Rect2D::MakeInfinite());
}

//=============================================================================
Image* Image::CreateFromImageWithTransform(const Image* im, AffineTransform* t)
{
    if(im == NULL) { return NULL; }
    return new TransformedImageImpl(im, t, Rect2D::MakeInfinite());
}

//=============================================================================
Image* Image::CreateFromImageCroppedTo(const Image* im, Rect2D cropRect)
{
    if(im == NULL) { return NULL; }
    return new TransformedImageImpl(im, NULL, cropRect);
}

//=============================================================================
Image* Image::CreateFromBitmapData(const BitmapImageRep* imageRep,
                bool copyData)
{
    if(imageRep->ImageBlob == NULL) { return NULL; }
    if(imageRep->Stride < imageRep->Width) { return NULL; }
    if(imageRep->Stride*imageRep->Height > imageRep->ImageBlob->GetLength()) 
    {
        return NULL; 
    }
    return new BitmapImageImpl(imageRep, copyData);
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

    // So that row 0 is the bottom-most row.
    MagickFlipImage(wand);

    unsigned int w = MagickGetImageWidth(wand);
    unsigned int h = MagickGetImageHeight(wand);

    Blob* imageBlob = Blob::CreateWithLength(w*h*4);

    MagickGetImagePixels(wand, 0, 0, w, h, "RGBA", CharPixel, 
            const_cast<uint8_t*>(imageBlob->GetBytes()));

    uint8_t* pixel = const_cast<uint8_t*>(imageBlob->GetBytes());
    for(unsigned int i=0; i<w*h; i++)
    {
        float alpha = ((float)(pixel[3]) / 255.0f);
        pixel[0] *= alpha;
        pixel[1] *= alpha;
        pixel[2] *= alpha;
        pixel += 4;
    }
    
    wand = DestroyMagickWand(wand);

    BitmapImageRep* bir = BitmapImageRep::Create(imageBlob, w, h, w*4, 
            BitmapImageRep::Byte, false);
    Image* rv = Image::CreateFromBitmapData(bir, false);
    FIRTREE_SAFE_RELEASE(bir);

    imageBlob->Release();

    return rv;
}

//=============================================================================
Image* Image::CreateFromKernel(Kernel* k, ExtentProvider* extentProvider)
{
    if(k == NULL) { return NULL; }

    if(extentProvider != NULL)
    {
        return new KernelImageImpl(k, extentProvider);
    }

    extentProvider = ExtentProvider::CreateStandardExtentProvider();
    Image* retVal = new KernelImageImpl(k, extentProvider);
    FIRTREE_SAFE_RELEASE(extentProvider);

    return retVal;
}

//=============================================================================
Image* Image::CreateFromImageProvider(ImageProvider* improv)
{
    if(improv == NULL) { return NULL; }
    return new ImageProviderImageImpl(improv);
}

//=============================================================================
Image* Image::CreateFromOpenGLTexture(unsigned int texObj)
{
    return new TextureImageImpl(texObj);
}

//=============================================================================
bool BitmapImageRep::WriteToFile(const char* pFileName) 
{
    if(pFileName == NULL) { return false; }

    MagickWand* wand = NewMagickWand();
    assert(wand != NULL);
    
    PixelWand* bg_pxl_wnd = NewPixelWand();
    PixelSetColor( bg_pxl_wnd, "black" );

    if((Width == 0) || (Height == 0))
    {
        wand = DestroyMagickWand(wand);
        return false;
    }

    MagickBooleanType status = MagickNewImage(wand, Width,
            Height, bg_pxl_wnd);
    if(status == MagickFalse)
    {
        FIRTREE_WARNING("Could not create image for writing.");
        wand = DestroyMagickWand(wand);
        return false;
    }

    bg_pxl_wnd = DestroyPixelWand(bg_pxl_wnd);

    Blob* outputBufferBlob = 
        Blob::CreateWithLength(Width * Height * 4);

    uint8_t* outBuf = const_cast<uint8_t*>(outputBufferBlob->GetBytes());
    uint8_t* inBuf = const_cast<uint8_t*>(ImageBlob->GetBytes());

    for(unsigned int row=0; row < Height; row++)
    {
        uint8_t* outRow = outBuf + (row * Width * 4);
        uint8_t* inRow = inBuf + (row * Stride);

        float* inFloatRow = reinterpret_cast<float*>(inRow);

        for(unsigned int col=0; col < Width; col++)
        {
            float r,g,b,a;

            if(Format == BitmapImageRep::Float)
            {
                r = inFloatRow[(col<<2)];
                g = inFloatRow[(col<<2)+1];
                b = inFloatRow[(col<<2)+2];
                a = inFloatRow[(col<<2)+3];
            } else if(Format == BitmapImageRep::Byte) {
                r = (1.f/255.f) * inRow[(col<<2)];
                g = (1.f/255.f) * inRow[(col<<2)+1];
                b = (1.f/255.f) * inRow[(col<<2)+2];
                a = (1.f/255.f) * inRow[(col<<2)+3];
            } else {
                FIRTREE_ERROR("Unknown bitmap image rep format: %i", Format);
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

    MagickSetImagePixels(wand, 0, 0, Width,
            Height, "RGBA", CharPixel, 
            const_cast<uint8_t*>(outputBufferBlob->GetBytes()));

    // A bitmap image rep stores the bottom-most row as row 0. Flip
    // the image so that row 0 is the top-most row.
    MagickFlipImage(wand);

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

// IMAGE ACCUMULATOR //////////////////////////////////////////////////////////

//=============================================================================
ImageAccumulator::ImageAccumulator(Rect2D extent, OpenGLContext* context)
    : ReferenceCounted()
    , m_Extent(extent)
    , m_Context(context)
{
    if(m_Context != NULL)
    {
        FIRTREE_SAFE_RETAIN(m_Context);
    } else {
        m_Context = OpenGLContext::CreateNullContext();
    }

    m_TextureContext = RenderTextureContext::Create(m_Extent.Size.Width, 
            m_Extent.Size.Height, m_Context);
    m_Renderer = GLRenderer::Create(m_TextureContext);

    m_Image = Image::CreateFromOpenGLTexture(m_TextureContext->GetOpenGLTexture());

    Clear();
}

//=============================================================================
ImageAccumulator::~ImageAccumulator()
{
    FIRTREE_SAFE_RELEASE(m_Context);
    FIRTREE_SAFE_RELEASE(m_TextureContext);
    FIRTREE_SAFE_RELEASE(m_Renderer);
    FIRTREE_SAFE_RELEASE(m_Image);
}

//=============================================================================
ImageAccumulator* ImageAccumulator::Create(Rect2D extent, OpenGLContext* context)
{
    return new ImageAccumulator(extent, context);
}

//=============================================================================
Image* ImageAccumulator::GetImage() const
{
    return m_Image;
}

//=============================================================================
void ImageAccumulator::Clear()
{
    m_Renderer->Clear(0,0,0,0);
}

//=============================================================================
void ImageAccumulator::RenderImage(Image* im)
{
    m_Renderer->RenderInRect(im, Rect2D(Point2D(0,0), m_Extent.Size), m_Extent);
}

} // namespace Firtree 

//=============================================================================
// vim:sw=4:ts=4:cindent:et
//
