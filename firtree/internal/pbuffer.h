// FIRTREE - A generic image processing library
// Copyright (C) 2007, 2008 Rich Wareham <richwareham@gmail.com>
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as published by
// the Free Software Foundation.
//
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License version 2 for more details.
//
// You should have received a copy of the GNU General Public License version 2
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA

/// \file pbuffer.h Off-screen Pbuffer support.

// ============================================================================
#ifndef FIRTREE_PBUFFER_H
#define FIRTREE_PBUFFER_H
// ============================================================================

// ============================================================================
namespace Firtree { namespace Internal {
// ============================================================================

// ============================================================================
/// How pixels are laid out in memory.  
enum PixelFormat {
    L8,             ///< One byte per-pixel luminance.
    L16F,           ///< One half-precision floating point value per-pixel.
    L32F,           ///< One float per-pixel luminance.
    R8G8B8,         ///< Three bytes per-pixel RGB triplet.
    B8G8R8,         ///< Three bytes per-pixel BGR triplet.
    R8G8B8A8,       ///< Four bytes per-pixel RGBA quartet.
    B8G8R8A8,       ///< Four bytes per-pixel BGRA quartet.
    R32G32B32A32F,  ///< Four floats per-pixel, RGBA quartet.
    R16G16B16A16F,  ///< Four half-precision floating point values per-pixel.
    L32A32F,        ///< Two floats per-pixel, Luminance/A pair.
    L32A32UI,       ///< Two unsigned 32-bit integers per-pixel, Luminance/A pair.
    DepthComponent, ///< Specialised depth buffer.
};

// ============================================================================
/// Manage an off-screen rendering context for non-windowed applications. If
/// you want to write an application which does entirely off-screen rendering
/// (e.g. if you want to integrate OpenGL into a compositing pipeline) then
/// a Pbuffer lets you create and manage an off-screen OpenGL context in
/// a platform agnostic way.
class Pbuffer {
    public:
        /// Enumeration representing the possible option flags for a 
        /// pbuffer.
        enum Flags {
            NoFlags         = 0x0,      ///< No flags
            DoubleBuffered  = 0x1,      ///< Context should be double-buffered.
        };

        ///             Default constructor
                        Pbuffer();

        ///             Destructor
                        ~Pbuffer();

        ///             Create an OpenGL context associated with this Pbuffer
        ///             requesting the passed width and height in pixels, 
        ///             pixel format and any flags.
        ///             Returns true if the context was created successfully,
        ///             false otherwise.
        bool            CreateContext(unsigned int width, unsigned int height,
                            PixelFormat format, Flags flags = NoFlags);

        ///             If the context was double-buffered, swap the front and
        ///             back buffer, otherwise do nothing.
        void            SwapBuffers();

        ///             Make the context encapsulated by this object current so
        ///             that OpenGL operations with act upon it. Returns true
        ///             if the context was set correctly.
        bool            StartRendering();

        ///             Should be called after StartRendering() and after you've
        ///             finished calling GL functions.
        void            EndRendering();

    private:
        class PbufferPlatformImpl*  m_pPlatformImpl;
};

// ============================================================================
} } // namespace Firtree::Internal
// ============================================================================

// ============================================================================
#endif // FIRTREE_PBUFFER_H
// ============================================================================

// So that VIM does the "right thing"
// vim:cindent:sw=4:ts=4:et
