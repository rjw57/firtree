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

#include <firtree/main.h>
#include <firtree/opengl.h>
#include <firtree/platform.h>
#include <internal/pbuffer.h>

#include <assert.h>

// ============================================================================
namespace Firtree { namespace Internal {
// ============================================================================

class PbufferPlatformImpl
{
    public:
        PbufferPlatformImpl() { }
        virtual ~PbufferPlatformImpl() { }

        virtual bool CreateContext(unsigned int width, unsigned int height,
                PixelFormat format, Pbuffer::Flags flags) = 0;
        virtual void SwapBuffers() = 0;
        virtual bool StartRendering() = 0;
        virtual void EndRendering() = 0;
        virtual void* GetProcAddress(const char* name) const = 0;
};

#if defined(FIRTREE_HAVE_OSMESA)
// ============================================================================
//  SOFTWARE MESA IMPLEMENTATION
// ============================================================================
class PbufferOSMesaImpl : public PbufferPlatformImpl
{
    public:
        PbufferOSMesaImpl() 
            : m_Context(NULL)
            , m_Width(0)
            , m_Height(0)
            , m_FrameBuffer(NULL)
        {
        }

        virtual ~PbufferOSMesaImpl() 
        {
            if(m_Context != NULL)
            {
                OSMesaDestroyContext(m_Context);
                m_Context = NULL;
            }

            if(m_FrameBuffer != NULL)
            {
                delete m_FrameBuffer;
            }
        }

        virtual bool CreateContext(unsigned int width, unsigned int height,
                PixelFormat format, Pbuffer::Flags flags)
        {
            if((format != R8G8B8A8) && (format != R8G8B8))
                return false;

            m_Context = OSMesaCreateContextExt(OSMESA_RGBA, 0, 0, 0, NULL);

            m_Width = width;
            m_Height = height;

            m_FrameBuffer = new uint8_t[width * height * 4];

            if(m_Context == NULL)
            {
                FIRTREE_WARNING("Error creating OSMesa context.");
            }

            return (m_Context != NULL);
        }

        virtual void SwapBuffers()
        {
            // NOP
        }

        virtual bool StartRendering() 
        {
            GLboolean retVal = OSMesaMakeCurrent(m_Context,
                    m_FrameBuffer, GL_UNSIGNED_BYTE,
                    m_Width, m_Height);

            if(retVal != GL_TRUE)
            {
                FIRTREE_WARNING("Error making OSMesa context current.");
            }

            return (retVal == GL_TRUE);
        }

        virtual void EndRendering() 
        {
        }

        virtual void* GetProcAddress(const char* name) const
        {
            return (void*)(OSMesaGetProcAddress(name));
        }

    private:
        OSMesaContext   m_Context;
        unsigned int    m_Width;
        unsigned int    m_Height;
        uint8_t*        m_FrameBuffer;
};
#endif

#if defined(FIRTREE_UNIX) && !defined(FIRTREE_APPLE)
// ============================================================================
//  PBUFFER GLX IMPLEMENTATION
// ============================================================================
class PbufferGLXImpl : public PbufferPlatformImpl
{
    public:
//      =======================================================================
        PbufferGLXImpl()
            : m_XDisplay(NULL)
            , m_Pbuffer(None)
            , m_Context(None)
            , m_IsCurrent(false)
        { }

//      =======================================================================
        virtual ~PbufferGLXImpl() 
        {
            if(m_Context != None)
            {
                glXDestroyContext(m_XDisplay, m_Context);
                m_Context = None;
            }

            if(m_Pbuffer != None)
            {
                glXDestroyPbuffer(m_XDisplay, m_Pbuffer);
                m_Pbuffer = None;
            }

            if(m_XDisplay != NULL)
            {
                XCloseDisplay(m_XDisplay);
                m_XDisplay = NULL;
            }
        }

//      =======================================================================
        virtual bool CreateContext(unsigned int width, unsigned int height,
                PixelFormat format, Pbuffer::Flags flags)
        {
            // GLX only supports RGBA visuals and, for the moment, we only
            // support 8-bit ones.
            if((format != R8G8B8A8) && (format != R8G8B8))
                return false;
            
            // Attempt to open a connection to the X display
            m_XDisplay = XOpenDisplay(NULL);
            if(m_XDisplay == NULL)
                return false;

            // Form the desired attributes...
            int desiredAttributes[] = {
                GLX_DOUBLEBUFFER, (flags & Pbuffer::DoubleBuffered) ? True : False,
                GLX_RED_SIZE, 1,
                GLX_GREEN_SIZE, 1,
                GLX_BLUE_SIZE, 1,
                GLX_ALPHA_SIZE, 1,
                GLX_RENDER_TYPE, GLX_RGBA_BIT,
                GLX_DRAWABLE_TYPE, GLX_PBUFFER_BIT,
                None
            };

            // And try to find a FBConfig...
            int numReturned = 0;
            GLXFBConfig* pFbConfigs = glXChooseFBConfig(m_XDisplay,
                    DefaultScreen(m_XDisplay), desiredAttributes,
                    &numReturned);
            if(pFbConfigs == NULL) 
                return false;

            if(numReturned == 0)
            {
                XFree(pFbConfigs);
                return false;
            }

            // We succeeded, attempt to make the context
            m_Context = glXCreateNewContext(m_XDisplay,
                    pFbConfigs[0], GLX_RGBA_TYPE, NULL, True);
            if(m_Context == None)
            {
                XFree(pFbConfigs);
                return false;
            }

            // We succeeded, try to create the Pbuffer
            int pbufferAttributes [] = {
                GLX_PBUFFER_WIDTH, width,
                GLX_PBUFFER_HEIGHT, height,
                None
            };

            m_Pbuffer = glXCreatePbuffer(m_XDisplay,
                    pFbConfigs[0], pbufferAttributes);
            if(m_Pbuffer == None)
            {
                glXDestroyContext(m_XDisplay, m_Context);
                m_Context = None;
                XFree(pFbConfigs);
                return false;
            }

            XFree(pFbConfigs);

            return true;
        }

//      =======================================================================
        virtual void SwapBuffers()
        {
            glXSwapBuffers(m_XDisplay, m_Pbuffer);
        }

//      =======================================================================
        virtual bool StartRendering()
        {
            if(m_IsCurrent)
                return true;
            m_IsCurrent = true;

            m_OldXDisplay = glXGetCurrentDisplay();
            m_OldDrawable = glXGetCurrentDrawable();
            m_OldContext = glXGetCurrentContext();

            Bool rv = glXMakeCurrent(m_XDisplay, m_Pbuffer, m_Context);
            if(rv != True)
            {
                FIRTREE_WARNING("Could not set Pbuffer to be current context!");
            }

            return (rv == True);
        }

//      =======================================================================
        virtual void EndRendering()
        {
            if(!m_IsCurrent)
                return;
            m_IsCurrent = false;

            if(m_OldXDisplay != NULL)
            {
                Bool rv = glXMakeCurrent(m_OldXDisplay, m_OldDrawable, m_OldContext);
                if(rv != True)
                {
                    FIRTREE_WARNING("Could not set glx state to be old context!");
                }
            }
        }
    
//      =======================================================================
        virtual void* GetProcAddress(const char* name) const
        {
            return (void*)(glXGetProcAddress((const GLubyte*)name));
        }

    private:
        Display*    m_XDisplay;
        GLXPbuffer  m_Pbuffer;
        GLXContext  m_Context;

        bool        m_IsCurrent;

        Display*    m_OldXDisplay;
        GLXDrawable m_OldDrawable;
        GLXContext  m_OldContext;
};
#endif

#if defined(FIRTREE_APPLE)
// ============================================================================
//  PBUFFER AGL IMPLEMENTATION
// ============================================================================
class PbufferAGLImpl : public PbufferPlatformImpl
{
    public:
//      =======================================================================
        PbufferAGLImpl()
            : m_IsCurrent(false)
            , m_AGLContext(NULL)  
            , m_AGLPbuffer(NULL)  
        { }

//      =======================================================================
        virtual ~PbufferAGLImpl() 
        {
            aglDestroyPBuffer(m_AGLPbuffer);
            aglDestroyContext(m_AGLContext);
        }

//      =======================================================================
        OSStatus AGLReportError(void)

        {
            GLenum err = aglGetError();

            if (AGL_NO_ERROR != err) {
                FIRTREE_ERROR("AGL: %s",(char *) aglErrorString(err));
            }

            if (err == AGL_NO_ERROR)
                return noErr;
            else
                return (OSStatus) err;
        }
        

//      =======================================================================
        virtual bool CreateContext(unsigned int width, unsigned int height,
                PixelFormat format, Pbuffer::Flags flags)
        {
            // AGL only supports RGBA visuals and, for the moment, we only
            // support 8-bit ones.
            if((format != R8G8B8A8) && (format != R8G8B8))
                return false;

            GLint layout = 0;
            switch(format)
            {
                case R8G8B8A8:
                    layout = AGL_RGBA;
                    break;
                case R8G8B8:
                    layout = AGL_RGBA;
                    break;
                default:
                    FIRTREE_ERROR("Format not supported.");
                    break;
            };

            OSStatus err = noErr;
            GLint attributes[] =  { 
                layout,
                AGL_DOUBLEBUFFER,
                AGL_DEPTH_SIZE, 24,
                AGL_NONE 
            };

            AGLPixelFormat myAGLPixelFormat;

            myAGLPixelFormat = aglChoosePixelFormat (NULL, 0, attributes);

            err = AGLReportError ();

            if (myAGLPixelFormat) {
                m_AGLContext = aglCreateContext (myAGLPixelFormat, NULL);

                err = AGLReportError ();
            }

            if(!aglCreatePBuffer(width, height, GL_TEXTURE_RECTANGLE_EXT,
                        GL_RGBA, 0, &m_AGLPbuffer))
            {
                err = AGLReportError();
                return false;
            }

            if(!aglSetPBuffer(m_AGLContext, m_AGLPbuffer,
                        0, 0, 0))
            {
                err = AGLReportError();
                return false;
            }

            return true;
        }

//      =======================================================================
        virtual void SwapBuffers()
        {
            aglSwapBuffers(m_AGLContext);
        }

//      =======================================================================
        virtual bool StartRendering()
        {
            if(m_IsCurrent)
                return true;
            m_IsCurrent = true;

            m_OldContext = aglGetCurrentContext();
            if(!aglSetCurrentContext(m_AGLContext))
            {
                AGLReportError();
                return false;
            }

            return true;
        }

//      =======================================================================
        virtual void EndRendering()
        {
            if(!m_IsCurrent)
                return;
            m_IsCurrent = false;

            aglSetCurrentContext(m_OldContext);
        }
     
//      =======================================================================
        virtual void* GetProcAddress(const char* name) const
        {
#           error FIXME: Missing GetProcAddress implementation.
            return NULL;
        }

    private:
        bool        m_IsCurrent;

        AGLContext  m_AGLContext;
        AGLPbuffer  m_AGLPbuffer;

        AGLContext  m_OldContext;
};
#endif

#ifdef FIRTREE_WIN32
// wGL extensions.
#include "../../third-party/include/wglext.h"

// ============================================================================
//  PBUFFER WGL IMPLEMENTATION
// ============================================================================

const char g_szClassName[] = "FIRTREEWindowClass";

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch(msg)
    {
        case WM_CLOSE:
            DestroyWindow(hwnd);
        break;
        case WM_DESTROY:
            PostQuitMessage(0);
        break;
        default:
            return DefWindowProc(hwnd, msg, wParam, lParam);
    }
    return 0;
}

static PFNWGLGETEXTENSIONSSTRINGARBPROC wglGetExtensionsStringARB = NULL;
static PFNWGLCREATEPBUFFERARBPROC wglCreatePbufferARB = NULL;
static PFNWGLGETPBUFFERDCARBPROC wglGetPbufferDCARB = NULL;
static PFNWGLRELEASEPBUFFERDCARBPROC wglReleasePbufferDCARB = NULL;
static PFNWGLDESTROYPBUFFERARBPROC wglDestroyPbufferARB = NULL;
static PFNWGLQUERYPBUFFERARBPROC wglQueryPbufferARB = NULL;

static bool EnsureWGLFunction(const char* name, void** dest)
{
    void* ptr = (void*)wglGetProcAddress(name);
    if(ptr == NULL)
        return false;

    *dest = ptr;
    return true;
}

class PbufferWGLImpl : public PbufferPlatformImpl
{
    public:
//      =======================================================================
        PbufferWGLImpl()
            :   m_hWindowDC(NULL)
            ,   m_hWindow(NULL)
            ,   m_hWindowContext(NULL)
            ,   m_hPbuffer(NULL)
            ,   m_hContext(NULL)
            ,   m_hDC(NULL)
        { }

//      =======================================================================
        virtual ~PbufferWGLImpl() 
        {
            if(m_hContext != NULL)
                wglDeleteContext(m_hContext);
            if((m_hDC != NULL) && (wglReleasePbufferDCARB != NULL))
                wglReleasePbufferDCARB(m_hPbuffer, m_hDC);
            if((m_hPbuffer != NULL) && (wglDestroyPbufferARB != NULL))
                wglDestroyPbufferARB(m_hPbuffer);
            if(m_hWindowContext != NULL)
                wglDeleteContext(m_hWindowContext);
            if(m_hWindowDC != NULL)
                ReleaseDC(m_hWindow, m_hWindowDC);
            if(m_hWindow != NULL)
                DestroyWindow(m_hWindow);
        }

//      =======================================================================
        virtual bool CreateContext(unsigned int width, unsigned int height,
                PixelFormat format, Pbuffer::Flags flags)
        {
            // We only support RGBA visuals and, for the moment, we only
            // support 8-bit ones.
            if((format != R8G8B8A8) && (format != R8G8B8))
            {
                FIRTREE_DEBUG("Off-screen format not supported.");
                return false;
            }

            // Because *$*&"£(&"-ing Vista fucked over OpenGL, we need to
            // jump through hoops to get an off-screen OpenGL context which is
            // accelerated properly.

            WNDCLASSEX wc;
            HINSTANCE hInstance = GetModuleHandle(NULL);

            wc.cbSize        = sizeof(WNDCLASSEX);
            wc.style         = 0;
            wc.lpfnWndProc   = WndProc;
            wc.cbClsExtra    = 0;
            wc.cbWndExtra    = 0;
            wc.hInstance     = hInstance;
            wc.hIcon         = LoadIcon(NULL, IDI_APPLICATION);
            wc.hCursor       = LoadCursor(NULL, IDC_ARROW);
            wc.hbrBackground = (HBRUSH)(COLOR_WINDOW+1);
            wc.lpszMenuName  = NULL;
            wc.lpszClassName = g_szClassName;
            wc.hIconSm       = LoadIcon(NULL, IDI_APPLICATION);

            if(!RegisterClassEx(&wc))
            {
                FIRTREE_DEBUG("Could not register class.");
                return false;
            }

            m_hWindow = CreateWindowEx(
                    WS_EX_CLIENTEDGE,
                    g_szClassName,
                    "Untitled",
                    WS_OVERLAPPEDWINDOW,
                    CW_USEDEFAULT, CW_USEDEFAULT, 100, 100,
                    NULL, NULL, hInstance, NULL);

            if(m_hWindow == NULL)
            {
                FIRTREE_DEBUG("Window creation failed!");
                return false;
            }

            m_hWindowDC = GetDC(m_hWindow);
            if(m_hWindowDC == NULL)
            {
                FIRTREE_DEBUG("Error getting window DC.");
                return false;
            }

            // Set the pixel format
            PIXELFORMATDESCRIPTOR pfd;
            ZeroMemory( &pfd, sizeof( pfd ) );
            pfd.nSize = sizeof( pfd );
            pfd.nVersion = 1;
            pfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL |
                ((flags & Pbuffer::DoubleBuffered) ? PFD_DOUBLEBUFFER : 0),
            pfd.iPixelType = PFD_TYPE_RGBA;
            pfd.cColorBits = 24;
            pfd.cDepthBits = 16;
            pfd.iLayerType = PFD_MAIN_PLANE;

            int pixelFormat = ChoosePixelFormat( m_hWindowDC, &pfd );
            if(pixelFormat == 0)
            {
                FIRTREE_DEBUG("ChoosePixelFormat failed.");
                return false;
            }

            BOOL rv = SetPixelFormat( m_hWindowDC, pixelFormat, &pfd );
            if(rv != TRUE)
            {
                FIRTREE_DEBUG("SetPixelFormat on Desktop Window failed.");
                return false;
            }

            // create the render context (RC) for the window
            m_hWindowContext = wglCreateContext( m_hWindowDC );
            if(m_hWindowContext == NULL)
            {
                FIRTREE_DEBUG("Could not create WGL context.");
                return false;
            }

            // make it the current render context
            GdiFlush();
            rv = wglMakeCurrent( m_hWindowDC, m_hWindowContext );
            if(rv != TRUE)
            {
                FIRTREE_DEBUG("Error making context current in createContext().");
                return false;
            }

            // At *this point* the wgl stuff works.

#define     ENSURE_FUNC(name) \
            { if(!EnsureWGLFunction(#name, (void**)&(name))) { \
                FIRTREE_DEBUG("Function %s not found!", #name); \
                return false; \
            } }

            ENSURE_FUNC(wglGetExtensionsStringARB);
            ENSURE_FUNC(wglCreatePbufferARB);
            ENSURE_FUNC(wglGetPbufferDCARB);
            ENSURE_FUNC(wglReleasePbufferDCARB);
            ENSURE_FUNC(wglDestroyPbufferARB);
            ENSURE_FUNC(wglQueryPbufferARB);
#undef      ENSURE_FUNC

            int pbufAttributes[] = {
                0,
            };

            m_hPbuffer = wglCreatePbufferARB(m_hWindowDC, pixelFormat,
                   width, height, pbufAttributes);
            if(m_hPbuffer == NULL)
            {
                FIRTREE_DEBUG("Error creating Pbuffer.");
            }

            m_hDC = wglGetPbufferDCARB(m_hPbuffer);
            if(m_hDC == NULL)
            {
                FIRTREE_DEBUG("Error getting DC for Pbuffer.");
            }
            
            m_hContext = wglCreateContext( m_hDC );
            if(m_hContext == NULL)
            {
                FIRTREE_DEBUG("Could not create WGL context for Pbuffer.");
                return false;
            }
            FIRTREE_CHECK_GL();

            return true;
        }

//      =======================================================================
        virtual void SwapBuffers()
        {
        }

//      =======================================================================
        virtual bool StartRendering()
        {
            GdiFlush();
            BOOL rv = wglMakeCurrent( m_hDC, m_hContext );
            if(rv != TRUE)
            {
                FIRTREE_DEBUG("Error making context current in MakeCurrent().");
                return false;
            }
            FIRTREE_CHECK_GL();

            return true;
        }

//      =======================================================================
        virtual void EndRendering()
        {
            glFinish();
        }
     
//      =======================================================================
        virtual void* GetProcAddress(const char* name) const
        {
#           error FIXME: Missing GetProcAddress implementation.
            return NULL;
        }
    
    private:
        HDC         m_hWindowDC;
        HWND        m_hWindow;
        HGLRC       m_hWindowContext;

        HPBUFFERARB m_hPbuffer;
        HGLRC       m_hContext;
        HDC         m_hDC;
};
#endif

// ============================================================================
//  PBUFFER METHODS
// ============================================================================

// ============================================================================
Pbuffer::Pbuffer()
    :   m_pPlatformImpl(NULL)
{
//#if defined(FIRTREE_HAVE_OSMESA)
//    m_pPlatformImpl = new PbufferOSMesaImpl();
#if defined(FIRTREE_UNIX) && !defined(FIRTREE_APPLE)
    m_pPlatformImpl = new PbufferGLXImpl();
#elif defined(FIRTREE_APPLE)
    m_pPlatformImpl = new PbufferAGLImpl();
#elif defined(FIRTREE_WIN32)
    m_pPlatformImpl = new PbufferWGLImpl();
#else
    FIRTREE_WARNING("No Pbuffer implementation available");
#endif
}

// ============================================================================
Pbuffer::~Pbuffer()
{
    if(m_pPlatformImpl != NULL)
    {
        delete m_pPlatformImpl;
        m_pPlatformImpl = NULL;
    }
}

// ============================================================================
bool Pbuffer::CreateContext(unsigned int width, unsigned int height,
                            PixelFormat format, Flags flags)
{
    if(!m_pPlatformImpl)
        return false;
    return m_pPlatformImpl->CreateContext(width, height, format, flags);
}

// ============================================================================
void Pbuffer::SwapBuffers()
{
    if(!m_pPlatformImpl)
        return;
    m_pPlatformImpl->SwapBuffers();
}

// ============================================================================
bool Pbuffer::StartRendering()
{
    if(!m_pPlatformImpl)
        return false;
    return m_pPlatformImpl->StartRendering();
}

// ============================================================================
void Pbuffer::EndRendering()
{
    if(!m_pPlatformImpl)
        return;
    return m_pPlatformImpl->EndRendering();
}

// ============================================================================
void* Pbuffer::GetProcAddress(const char* name) const
{
    return m_pPlatformImpl->GetProcAddress(name);
}

// ============================================================================
} } // namespace Firtree::Internal 
// ============================================================================

// So that VIM does the "right thing"
// vim:cindent:sw=4:ts=4:et
