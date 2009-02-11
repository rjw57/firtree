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
/// \file glsl-runtime.h OpenGL rendering methods for Images.
//=============================================================================

//=============================================================================
#ifndef FIRTREE_GLSL_RUNTIME_H
#define FIRTREE_GLSL_RUNTIME_H
//=============================================================================

#include <firtree/kernel.h>
#include <firtree/image.h>
#include <firtree/opengl.h>

#include <stack>

namespace Firtree { 

// We have our own glext.h - style function defs here so we don't have to rely
// on them being provided.
///@{
/// A set of types representing various OpenGL entry points.
typedef void (*PFNGLBINDTEXTUREPROC) (GLenum, GLuint);
typedef void (*PFNGLGETTEXLEVELPARAMETERIVPROC) (GLenum, GLint, GLenum, GLint*);
typedef void (*PFNGLGETTEXIMAGEPROC) (GLenum, GLint, GLenum, GLenum, GLvoid*);
typedef void (*PFNGLTEXIMAGE2DPROC) (GLenum, GLint, GLint, GLsizei, GLsizei,
        GLint, GLenum, GLenum, const GLvoid*);
typedef GLenum (*PFNGLGETERRORPROC) ( void );
typedef void (*PFNGLGETINTEGERVPROC) (GLenum, GLint*);
typedef void (*PFNGLGETFLOATVPROC) (GLenum, GLfloat*);
typedef void (*PFNGLVIEWPORTPROC) (GLint, GLint, GLsizei, GLsizei);
typedef void (*PFNGLPOPMATRIXPROC) ( void );
typedef void (*PFNGLPUSHMATRIXPROC) ( void );
typedef void (*PFNGLMATRIXMODEPROC) ( GLenum );
typedef void (*PFNGLBEGINPROC) ( GLenum );
typedef void (*PFNGLENDPROC) ( void );
typedef void (*PFNGLCLEARPROC) ( GLenum );
typedef void (*PFNGLLOADIDENTITYPROC) ( void );
typedef void (*PFNGLCLEARCOLORPROC) ( GLfloat, GLfloat, GLfloat, GLfloat );
typedef void (*PFNGLDISABLEPROC) ( GLenum );
typedef void (*PFNGLENABLEPROC) ( GLenum );
typedef void (*PFNGLVERTEX2FPROC) ( GLfloat, GLfloat );
typedef void (*PFNGLTEXCOORD2FPROC) ( GLfloat, GLfloat );
typedef void (*PFNGLORTHOPROC) ( GLdouble, GLdouble, GLdouble, GLdouble,
        GLdouble, GLdouble );
typedef void (*PFNGLBLENDFUNCPROC) ( GLenum, GLenum );
typedef void (*PFNGLTEXPARAMETERFPROC) ( GLenum, GLenum, GLfloat );
typedef void (*PFNGLTEXPARAMETERIPROC) ( GLenum, GLenum, GLint );
typedef void (*PFNGLDRAWBUFFERPROC) ( GLenum );
typedef void (*PFNGLGENTEXTURESPROC) ( GLsizei, GLuint* );
typedef void (*PFNGLDELETETEXTURESPROC) ( GLsizei, GLuint* );

typedef GLboolean (*PFNGLISRENDERBUFFEREXTPROC) (GLuint);
typedef void (*PFNGLBINDRENDERBUFFEREXTPROC) (GLenum, GLuint);
typedef void (*PFNGLDELETERENDERBUFFERSEXTPROC) (GLsizei, const GLuint *);
typedef void (*PFNGLGENRENDERBUFFERSEXTPROC) (GLsizei, GLuint *);
typedef void (*PFNGLRENDERBUFFERSTORAGEEXTPROC) (GLenum, GLenum, GLsizei, GLsizei);
typedef void (*PFNGLGETRENDERBUFFERPARAMETERIVEXTPROC) (GLenum, GLenum, GLint *);
typedef GLboolean (*PFNGLISFRAMEBUFFEREXTPROC) (GLuint);
typedef void (*PFNGLBINDFRAMEBUFFEREXTPROC) (GLenum, GLuint);
typedef void (*PFNGLDELETEFRAMEBUFFERSEXTPROC) (GLsizei, const GLuint *);
typedef void (*PFNGLGENFRAMEBUFFERSEXTPROC) (GLsizei, GLuint *);
typedef GLenum (*PFNGLCHECKFRAMEBUFFERSTATUSEXTPROC) (GLenum);
typedef void (*PFNGLFRAMEBUFFERTEXTURE1DEXTPROC) (GLenum, GLenum, GLenum, GLuint, GLint);
typedef void (*PFNGLFRAMEBUFFERTEXTURE2DEXTPROC) (GLenum, GLenum, GLenum, GLuint, GLint);
typedef void (*PFNGLFRAMEBUFFERTEXTURE3DEXTPROC) (GLenum, GLenum, GLenum, GLuint, GLint, 
        GLint);
typedef void (*PFNGLFRAMEBUFFERRENDERBUFFEREXTPROC) (GLenum, GLenum, GLenum, GLuint);
typedef void (*PFNGLGETFRAMEBUFFERATTACHMENTPARAMETERIVEXTPROC) (GLenum, GLenum, GLenum, 
        GLint *);
typedef void (*PFNGLGENERATEMIPMAPEXTPROC) (GLenum);

typedef void (*PFNGLACTIVETEXTUREARBPROC) (GLenum);

typedef unsigned int GLhandleARB;
typedef void (*PFNGLDELETEOBJECTARBPROC) (GLhandleARB);
typedef GLhandleARB (*PFNGLGETHANDLEARBPROC) (GLenum);
typedef void (*PFNGLDETACHOBJECTARBPROC) (GLhandleARB, GLhandleARB);
typedef GLhandleARB (*PFNGLCREATESHADEROBJECTARBPROC) (GLenum);
typedef void (*PFNGLSHADERSOURCEARBPROC) (GLhandleARB, GLsizei, const GLcharARB* *,
        const GLint *);
typedef void (*PFNGLCOMPILESHADERARBPROC) (GLhandleARB);
typedef GLhandleARB (*PFNGLCREATEPROGRAMOBJECTARBPROC) (void);
typedef void (*PFNGLATTACHOBJECTARBPROC) (GLhandleARB, GLhandleARB);
typedef void (*PFNGLLINKPROGRAMARBPROC) (GLhandleARB);
typedef void (*PFNGLUSEPROGRAMOBJECTARBPROC) (GLhandleARB);
typedef void (*PFNGLVALIDATEPROGRAMARBPROC) (GLhandleARB);
typedef void (*PFNGLUNIFORM1FARBPROC) (GLint, GLfloat);
typedef void (*PFNGLUNIFORM2FARBPROC) (GLint, GLfloat, GLfloat);
typedef void (*PFNGLUNIFORM3FARBPROC) (GLint, GLfloat, GLfloat, GLfloat);
typedef void (*PFNGLUNIFORM4FARBPROC) (GLint, GLfloat, GLfloat, GLfloat, GLfloat);
typedef void (*PFNGLUNIFORM1IARBPROC) (GLint, GLint);
typedef void (*PFNGLUNIFORM2IARBPROC) (GLint, GLint, GLint);
typedef void (*PFNGLUNIFORM3IARBPROC) (GLint, GLint, GLint, GLint);
typedef void (*PFNGLUNIFORM4IARBPROC) (GLint, GLint, GLint, GLint, GLint);
typedef void (*PFNGLUNIFORM1FVARBPROC) (GLint, GLsizei, const GLfloat *);
typedef void (*PFNGLUNIFORM2FVARBPROC) (GLint, GLsizei, const GLfloat *);
typedef void (*PFNGLUNIFORM3FVARBPROC) (GLint, GLsizei, const GLfloat *);
typedef void (*PFNGLUNIFORM4FVARBPROC) (GLint, GLsizei, const GLfloat *);
typedef void (*PFNGLUNIFORM1IVARBPROC) (GLint, GLsizei, const GLint *);
typedef void (*PFNGLUNIFORM2IVARBPROC) (GLint, GLsizei, const GLint *);
typedef void (*PFNGLUNIFORM3IVARBPROC) (GLint, GLsizei, const GLint *);
typedef void (*PFNGLUNIFORM4IVARBPROC) (GLint, GLsizei, const GLint *);
typedef void (*PFNGLGETOBJECTPARAMETERFVARBPROC) (GLhandleARB, GLenum, GLfloat *);
typedef void (*PFNGLGETOBJECTPARAMETERIVARBPROC) (GLhandleARB, GLenum, GLint *);
typedef void (*PFNGLGETINFOLOGARBPROC) (GLhandleARB, GLsizei, GLsizei *, GLcharARB *);
typedef void (*PFNGLGETATTACHEDOBJECTSARBPROC) (GLhandleARB, GLsizei, GLsizei *, GLhandleARB *);
typedef GLint (*PFNGLGETUNIFORMLOCATIONARBPROC) (GLhandleARB, const GLcharARB *);
typedef void (*PFNGLGETACTIVEUNIFORMARBPROC) (GLhandleARB, GLuint index, GLsizei, GLsizei *, 
        GLint *, GLenum *, GLcharARB *);
typedef void (*PFNGLGETUNIFORMFVARBPROC) (GLhandleARB, GLint, GLfloat *);
typedef void (*PFNGLGETUNIFORMIVARBPROC) (GLhandleARB, GLint, GLint *);
typedef void (*PFNGLGETSHADERSOURCEARBPROC) (GLhandleARB, GLsizei, GLsizei *, GLcharARB *);
/// @}

namespace Internal { 
    template<typename Key> class LRUCache;
}

//=============================================================================
/// Returns the current GL context being used for rendering or NULL if there
/// is non. This only returns non-NULL when at least one context is within it's
/// Begin()/End() bracket.
OpenGLContext* GetCurrentGLContext();

//=============================================================================
/// An OpenGLContext is an object which knows how to start up, tear down and
/// make active a particular OpenGL context. The GLRenderer object
/// makes use of an OpenGLContext to ensure that the current OpenGL context is
/// the correct one.
///
/// The default implementation does nothing moving the entire burden for
/// OpenGL context management to the controlling app.
class OpenGLContext : public ReferenceCounted
{
    protected:
        OpenGLContext();

    public:

        // ====================================================================
        // PUBLIC FUNCTION POINTER METHODS

        ///
        /// @{
        /// Function pointers to OpenGL API entry points for this context.
        PFNGLBINDTEXTUREPROC                glBindTexture;
        PFNGLGETTEXLEVELPARAMETERIVPROC     glGetTexLevelParameteriv;
        PFNGLGETTEXIMAGEPROC                glGetTexImage;
        PFNGLTEXIMAGE2DPROC                 glTexImage2D;
        PFNGLGETERRORPROC                   glGetError;
        PFNGLGETINTEGERVPROC                glGetIntegerv;
        PFNGLGETFLOATVPROC                  glGetFloatv;
        PFNGLVIEWPORTPROC                   glViewport;
        PFNGLPOPMATRIXPROC                  glPopMatrix;
        PFNGLPUSHMATRIXPROC                 glPushMatrix;
        PFNGLMATRIXMODEPROC                 glMatrixMode;
        PFNGLBEGINPROC                      glBegin;
        PFNGLENDPROC                        glEnd;
        PFNGLCLEARPROC                      glClear;
        PFNGLLOADIDENTITYPROC               glLoadIdentity;
        PFNGLCLEARCOLORPROC                 glClearColor;
        PFNGLENABLEPROC                     glEnable;
        PFNGLDISABLEPROC                    glDisable;
        PFNGLVERTEX2FPROC                   glVertex2f;
        PFNGLTEXCOORD2FPROC                 glTexCoord2f;
        PFNGLORTHOPROC                      glOrtho;
        PFNGLBLENDFUNCPROC                  glBlendFunc;
        PFNGLTEXPARAMETERFPROC              glTexParameterf;
        PFNGLTEXPARAMETERIPROC              glTexParameteri;
        PFNGLDRAWBUFFERPROC                 glDrawBuffer;
        PFNGLGENTEXTURESPROC                glGenTextures;
        PFNGLDELETETEXTURESPROC             glDeleteTextures;

        PFNGLGENERATEMIPMAPEXTPROC          glGenerateMipmapEXT;
        PFNGLGENFRAMEBUFFERSEXTPROC         glGenFramebuffersEXT;
        PFNGLDELETEFRAMEBUFFERSEXTPROC      glDeleteFramebuffersEXT;
        PFNGLBINDFRAMEBUFFEREXTPROC         glBindFramebufferEXT;
        PFNGLFRAMEBUFFERTEXTURE2DEXTPROC    glFramebufferTexture2DEXT;
        PFNGLCHECKFRAMEBUFFERSTATUSEXTPROC  glCheckFramebufferStatusEXT;
        PFNGLGENRENDERBUFFERSEXTPROC        glGenRenderbuffersEXT;
        PFNGLBINDRENDERBUFFEREXTPROC        glBindRenderbufferEXT;
        PFNGLRENDERBUFFERSTORAGEEXTPROC     glRenderbufferStorageEXT;
        PFNGLFRAMEBUFFERRENDERBUFFEREXTPROC glFramebufferRenderbufferEXT;

        PFNGLACTIVETEXTUREARBPROC           glActiveTextureARB;
        PFNGLUSEPROGRAMOBJECTARBPROC        glUseProgramObjectARB;
        PFNGLGETINFOLOGARBPROC              glGetInfoLogARB;
        PFNGLGETOBJECTPARAMETERIVARBPROC    glGetObjectParameterivARB;
        PFNGLSHADERSOURCEARBPROC            glShaderSourceARB;
        PFNGLCOMPILESHADERARBPROC           glCompileShaderARB;
        PFNGLCREATEPROGRAMOBJECTARBPROC     glCreateProgramObjectARB;
        PFNGLATTACHOBJECTARBPROC            glAttachObjectARB;
        PFNGLLINKPROGRAMARBPROC             glLinkProgramARB;
        PFNGLCREATESHADEROBJECTARBPROC      glCreateShaderObjectARB;
        PFNGLGETUNIFORMLOCATIONARBPROC      glGetUniformLocationARB;
        PFNGLUNIFORM1IARBPROC               glUniform1iARB;
        PFNGLUNIFORM2IARBPROC               glUniform2iARB;
        PFNGLUNIFORM3IARBPROC               glUniform3iARB;
        PFNGLUNIFORM4IARBPROC               glUniform4iARB;
        PFNGLUNIFORM1FARBPROC               glUniform1fARB;
        PFNGLUNIFORM2FARBPROC               glUniform2fARB;
        PFNGLUNIFORM3FARBPROC               glUniform3fARB;
        PFNGLUNIFORM4FARBPROC               glUniform4fARB;
        PFNGLDELETEOBJECTARBPROC            glDeleteObjectARB;
        /// @}

    public:
        virtual ~OpenGLContext();

        // ====================================================================
        // CONSTRUCTION METHODS
        
        /// Create a simple 'null' context which does nothing when
        /// MakeCurrent() is called.
        static OpenGLContext* CreateNullContext();

        /// Create an off-screen pbuffer backed 8-bit rendering context which
        /// is of the specified size.
        static OpenGLContext* CreateOffScreenContext(uint32_t width,
                uint32_t height, bool softwareOnly = false);

        // ====================================================================
        // CONST METHODS
        
        /// Return a pointer to the named OpenGL routine.
        virtual void* GetProcAddress(const char* name) const;

        // ====================================================================
        // MUTATING METHODS
        
        /// Create a new texture within this context and return it's OpenGL
        /// name.
        unsigned int GenTexture();
        
        /// Delete a texture previously created via GenTexture();
        void DeleteTexture(unsigned int texName);

        /// Begin rendering into this context.
        ///
        /// Sub classes sould be careful to call this implementation in their
        /// overridden methods.
        virtual void Begin();

        /// Finish rendering into this context.
        ///
        /// Sub classes sould be careful to call this implementation in their
        /// overridden methods.
        virtual void End();

        /// Return the number of Begin() calls which are waiting for a matching
        /// End().
        uint32_t GetBeginDepth() const { return m_BeginDepth; }

    protected:
        /// Call this to use GetProcAddress() to fill in the function pointer table
        /// for this class.
        void FillFunctionPointerTable();
        
        /// This is initially 0 on construction and is incremented once for
        /// every call to Begin() and decremented once for each call to End().
        uint32_t    m_BeginDepth;

        /// A vector of textures created within this context which are yet
        /// to be destroyed.
        std::vector<unsigned int> m_ActiveTextures;

        /// Each call to Begin() pushes the currently active context to this
        /// stack, each call to End() pops it.
        std::stack<OpenGLContext*> m_PriorContexts;

        /// Set to true once FillFunctionPointerTable() has been called.
        bool m_FilledFunctionPointerTable;
};

}

//=============================================================================
#endif // FIRTREE_GLSL_RUNTIME_H
//=============================================================================

//=============================================================================
// vim:sw=4:ts=4:cindent:et

