// FIRTREE - A generic image processing library
// Copyright (C) 2007, 2008, 2009 Rich Wareham <richwareham@gmail.com>
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
/// \file cpu-runtime.h CPU rendering methods for Images.
//=============================================================================

//=============================================================================
#ifndef FIRTREE_CPU_RUNTIME_H
#define FIRTREE_CPU_RUNTIME_H
//=============================================================================

#include <firtree/kernel.h>
#include <firtree/image.h>

namespace Firtree { 

//=============================================================================
/// A CPU rendering context encapsulates all the information Firtree needs
/// to render images into an image buffers.
class CPURenderer : public ReferenceCounted {
    protected:
        /// Protected contrution. Use Create*() methods.
        ///@{
        CPURenderer(const Rect2D& viewport);
        ///@}
        
    public:
        virtual ~CPURenderer();

        // ====================================================================
        // CONSTRUCTION METHODS

        /// Construct a new CPU rendering context. 
        static CPURenderer* Create(uint32_t width, uint32_t height);

        /// Construct a new CPU rendering context. 
        static CPURenderer* Create(const Rect2D& viewport);

        // ====================================================================
        // CONST METHODS

        /// Clear the context with the specified color
        void Clear(float r, float g, float b, float a);

        /// Create an image representation of the contents of the
        /// internal buffer.
        Image* CreateImage();

        /// Write the internal buffer to a file. Returns false if the 
        /// operation failed.
        inline bool WriteToFile(const char* pFileName) {
            Image* im = CreateImage();
            bool rv = WriteImageToFile(im, pFileName);
            FIRTREE_SAFE_RELEASE(im);
            return rv;
        }

        /// Render the passed image into a BitmapImageRep, returning
        /// NULL if the image cannot be rendered (e.g. if it has
        /// infinite extent).
        /// The BitmapImageRep should be Release()-ed afterwards.
        BitmapImageRep* CreateBitmapImageRepFromImage(Image* image,
                BitmapImageRep::PixelFormat format =
                    BitmapImageRep::Float);

        /// Convenience wrapper which renders an image into a bitmap
        /// and writes it to a file. Returns false if the operation failed.
        bool WriteImageToFile(Image* image, const char* pFileName);

        /// Render the passed image into the current OpenGL context
        /// in the rectangle destRect, using the pixels from srcRect in
        /// the image as a source. 
        void RenderInRect(Image* image, const Rect2D& destRect,
                const Rect2D& srcRect);

        /// Convenience wrapper around RenderInRect() which sets the
        /// destination and source rectangles to have identical sizes.
        /// The result us to render the pixels of the image from srcRect 
        /// with the lower-left pixel being at location.
        void RenderAtPoint(Image* image, const Point2D& location,
                const Rect2D& srcRect);

        /// Render an image at 1:1 size with the logical origin of the
        /// image at origin.
        void RenderWithOrigin(Image* image, const Point2D& origin);

        /// Return a const reference to the viewport.
        inline const Rect2D& GetViewport() const { return m_ViewportRect; }

    private:
        /// The rectangle which specifies the 'viewport' of this renderer.
        Rect2D          m_ViewportRect;

        /// A bitmap image rep which will hold the output.
        BitmapImageRep* m_OutputRep;
};

}

//=============================================================================
#endif // FIRTREE_CPU_RUNTIME_H
//=============================================================================

//=============================================================================
// vim:sw=4:ts=4:cindent:et

