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

#include <IL/il.h>
#include <assert.h>

namespace Firtree {

using namespace Internal;

//=============================================================================
BitmapImageRep* BitmapImageRep::Create(Blob* blob,
    unsigned int width, unsigned int height, unsigned int stride,
    PixelFormat format, bool copy, bool flipped)
{
    return new BitmapImageRep(blob, width, height, stride, format, copy, flipped);
}

//=============================================================================
BitmapImageRep* BitmapImageRep::CreateFromBitmapImageRep(
        const BitmapImageRep* rep, bool copyData, bool flipped)
{
    return new BitmapImageRep(rep, copyData, flipped);
}

//=============================================================================
BitmapImageRep::BitmapImageRep(Blob* blob,
    unsigned int width, unsigned int height, unsigned int stride,
    PixelFormat format, bool copy, bool flipped)
    :   Width(width)
    ,   Height(height)
    ,   Stride(stride)
    ,   Format(format)
    ,   Flipped(flipped)
    ,   IsDynamic(false)
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
BitmapImageRep::BitmapImageRep(const BitmapImageRep* rep, bool copy, 
        bool flipped)
    :   Width(rep->Width)
    ,   Height(rep->Height)
    ,   Stride(rep->Stride)
    ,   Format(rep->Format)
    ,   Flipped(flipped)
    ,   IsDynamic(false)
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
void FormatConversion::ExpandComponents(Blob* src,
        FormatConversion::SourcePixelFormat src_format,
        Blob* dest, BitmapImageRep::PixelFormat dest_format)
{
    if((src == NULL) || (dest == NULL))
    {
        FIRTREE_ERROR("Passed a NULL reference.");
        return;
    }

    bool dest_is_float = (dest_format == BitmapImageRep::Float);

    // Separate loops for each format.
    switch(src_format)
    {
        case Luminance8:
            {
                uint32_t num_pixels = src->GetLength();

                uint32_t dest_min_size = num_pixels * 4 *
                    (dest_is_float ? 4 : 1);
                if(dest->GetLength() < dest_min_size)
                {
                    FIRTREE_ERROR("Destination blob is too small.");
                    return;
                }

                if(dest_is_float)
                {
                    const uint8_t* inbuf = src->GetBytes();
                    float* outbuf = const_cast<float*>(
                            reinterpret_cast<const float*>(
                                dest->GetBytes()));
                    for(uint32_t pixel_idx=0; pixel_idx<num_pixels; pixel_idx++)
                    {
                        float pixel = (*inbuf / 255.0f);

                        outbuf[0] = pixel;
                        outbuf[1] = pixel;
                        outbuf[2] = pixel;
                        outbuf[3] = 1.0f;

                        inbuf ++;
                        outbuf += 4;
                    }
                } else {
                    const uint8_t* inbuf = src->GetBytes();
                    uint8_t* outbuf = const_cast<uint8_t*>(dest->GetBytes());
                    for(uint32_t pixel_idx=0; pixel_idx<num_pixels; pixel_idx++)
                    {
                        uint8_t pixel = *inbuf;

                        outbuf[0] = pixel;
                        outbuf[1] = pixel;
                        outbuf[2] = pixel;
                        outbuf[3] = 0xff;

                        inbuf ++;
                        outbuf += 4;
                    }
                }
            }
            break;
        case RGBA8:
            {
                uint32_t num_pixels = (src->GetLength()) / 4;

                uint32_t dest_min_size = num_pixels * 4 *
                    (dest_is_float ? 4 : 1);
                if(dest->GetLength() < dest_min_size)
                {
                    FIRTREE_ERROR("Destination blob is too small.");
                    return;
                }

                if(dest_is_float)
                {
                    const uint8_t* inbuf = src->GetBytes();
                    float* outbuf = const_cast<float*>(
                            reinterpret_cast<const float*>(
                                dest->GetBytes()));
                    for(uint32_t pixel_idx=0; pixel_idx<num_pixels; pixel_idx++)
                    {
                        float rpixel = (inbuf[0] / 255.0f);
                        float gpixel = (inbuf[1] / 255.0f);
                        float bpixel = (inbuf[2] / 255.0f);
                        float apixel = (inbuf[3] / 255.0f);

                        outbuf[0] = rpixel;
                        outbuf[1] = gpixel;
                        outbuf[2] = bpixel;
                        outbuf[3] = apixel;

                        inbuf += 4;
                        outbuf += 4;
                    }
                } else {
                    const uint8_t* inbuf = src->GetBytes();
                    uint8_t* outbuf = const_cast<uint8_t*>(dest->GetBytes());
                    for(uint32_t pixel_idx=0; pixel_idx<num_pixels; pixel_idx++)
                    {
                        float rpixel = (inbuf[0] / 255.0f);
                        float gpixel = (inbuf[1] / 255.0f);
                        float bpixel = (inbuf[2] / 255.0f);
                        float apixel = (inbuf[3] / 255.0f);

                        outbuf[0] = rpixel;
                        outbuf[1] = gpixel;
                        outbuf[2] = bpixel;
                        outbuf[3] = apixel;

                        inbuf += 4;
                        outbuf += 4;
                    }
                }
            }
            break;

        default:
            FIRTREE_ERROR("Unsupported conversion format: %i", src_format);
            return;
    }
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
    ilInit();

    ILuint image;
    ilGenImages(1, &image);

    // Ensure all images have their origin in the lower-left.
    ilEnable(IL_ORIGIN_SET);
    ilOriginFunc(IL_ORIGIN_LOWER_LEFT);

    if(pFileName == NULL) { return NULL; }

    ilBindImage(image);
    //ilSetInteger(IL_ORIGIN_MODE, IL_ORIGIN_LOWER_LEFT);
    if(!ilLoadImage(const_cast<char*>(pFileName)))
    {
        FIRTREE_WARNING("Could not read image from %s.", pFileName);
        ilBindImage(0);
        ilDeleteImages(1, &image);
        return false;
    }

    unsigned int w = ilGetInteger(IL_IMAGE_WIDTH);
    unsigned int h = ilGetInteger(IL_IMAGE_HEIGHT);

    Blob* imageBlob = Blob::CreateWithLength(w*h*4);

    ilCopyPixels(0, 0, 0, w, h, 1, IL_RGBA, IL_UNSIGNED_BYTE,
            (ILvoid*)(const_cast<uint8_t*>(imageBlob->GetBytes())));

    // Pre multiply
    uint8_t* pixel = const_cast<uint8_t*>(imageBlob->GetBytes());
    for(unsigned int i=0; i<w*h; i++)
    {
        float alpha = ((float)(pixel[3]) / 255.0f);
        pixel[0] *= alpha;
        pixel[1] *= alpha;
        pixel[2] *= alpha;
        pixel += 4;
    }

#if 0
    // Flip so that bottom pixels at beginning of buffer.
    pixel = const_cast<uint8_t*>(imageBlob->GetBytes());
    for(unsigned int r=0; r<(h>>1); r++)
    {
        for(unsigned int c=0; c<w*4; c++)
        {
            uint8_t pv1 = pixel[c + (r*w*4)];
            uint8_t pv2 = pixel[c + ((h-1-r)*w*4)];
            pixel[c + (r*w*4)] = pv2;
            pixel[c + ((h-1-r)*w*4)] = pv1;
        }
    }
#endif
    
    BitmapImageRep* bir = BitmapImageRep::Create(imageBlob, w, h, w*4, 
            BitmapImageRep::Byte, false);
    Image* rv = Image::CreateFromBitmapData(bir, false);
    FIRTREE_SAFE_RELEASE(bir);

    imageBlob->Release();

    ilBindImage(0);
    ilDeleteImages(1, &image);

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
Image* Image::CreateFromOpenGLTexture(unsigned int texObj, OpenGLContext* c,
        bool flipped)
{
    return new TextureImageImpl(texObj, c, flipped);
}

//=============================================================================
bool BitmapImageRep::WriteToFile(const char* pFileName) 
{
    ilInit();

    ILuint image;
    ilGenImages(1, &image);

    // Ensure all images have their origin in the lower-left.
    ilEnable(IL_ORIGIN_SET);
    ilOriginFunc(IL_ORIGIN_LOWER_LEFT);

    Blob* outputBufferBlob = 
        Blob::CreateWithLength(Width * Height * 4);

    uint8_t* outBuf = const_cast<uint8_t*>(outputBufferBlob->GetBytes());
    uint8_t* inBuf = const_cast<uint8_t*>(ImageBlob->GetBytes());

    for(unsigned int row=0; row < Height; row++)
    {
        //uint8_t* outRow = outBuf + ((Height - row - 1) * Width * 4);
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

    ilBindImage(image);
    //ilSetInteger(IL_ORIGIN_MODE, IL_ORIGIN_LOWER_LEFT);
    ilTexImage(Width, Height, 1, 4, IL_RGBA, IL_UNSIGNED_BYTE,
            (ILvoid*) const_cast<uint8_t*>(outputBufferBlob->GetBytes()));

    ilEnable(IL_FILE_OVERWRITE);
    bool retVal = ilSaveImage(const_cast<char*>(pFileName));

    ilBindImage(0);
    ilDeleteImages(1, &image);

    FIRTREE_SAFE_RELEASE(outputBufferBlob);

    return retVal;
}

//=============================================================================
Image* Image::Copy() const
{
    return Image::CreateFromImage(this);
}

// IMAGE ACCUMULATOR //////////////////////////////////////////////////////////

#if 0

//=============================================================================
ImageAccumulator::ImageAccumulator(Rect2D extent, OpenGLContext* context)
    : ReferenceCounted()
    , m_Extent(extent)
    , m_Context(context)
{
    if(m_Context == NULL)
    {
        m_Context = GetCurrentGLContext();
        if(m_Context == NULL)
        {
            FIRTREE_ERROR("ImageAccumulator asked to use current GL context. "
                    "No such context exists.");
        }
    }

    FIRTREE_SAFE_RETAIN(m_Context);

    m_TextureContext = RenderTextureContext::Create(m_Extent.Size.Width, 
            m_Extent.Size.Height, m_Context);
    m_Renderer = GLRenderer::Create(m_TextureContext);

    Image* untransImage = 
        Image::CreateFromOpenGLTexture(m_TextureContext->GetOpenGLTexture(), m_Context);
    AffineTransform* t = AffineTransform::Translation(m_Extent.Origin.X, m_Extent.Origin.Y);
    m_Image = Image::CreateFromImageWithTransform(untransImage, t);
    FIRTREE_SAFE_RELEASE(t);
    FIRTREE_SAFE_RELEASE(untransImage);

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

#endif

} // namespace Firtree 

//=============================================================================
// vim:sw=4:ts=4:cindent:et
//
