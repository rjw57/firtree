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
// This file defines the interface to the FIRTREE kernels and samplers.
//=============================================================================

//=============================================================================
#ifndef FIRTREE_GLSL_RUNTIME_PRIV_H
#define FIRTREE_GLSL_RUNTIME_PRIV_H
//=============================================================================

#include <stdlib.h>
#include <string>
#include <vector>
#include <map>

#include <firtree/kernel.h>
#include <firtree/glsl-runtime.h>
#include <firtree/linker/sampler_provider.h>

namespace Firtree { namespace Internal {
    class ImageImpl;
} }

namespace Firtree { namespace GLSL {

class GLSLSamplerParameter;
class CompiledGLSLKernel;

//=============================================================================
GLSLSamplerParameter* CreateSampler(Image* im);

//=============================================================================
GLSLSamplerParameter* CreateTextureSampler(Image* image);

//=============================================================================
GLSLSamplerParameter* CreateTextureSamplerWithTransform(
        Image* image, const AffineTransform* transform);

//=============================================================================
GLSLSamplerParameter* CreateKernelSampler(Image* im);

//=============================================================================
GLSLSamplerParameter* CreateKernelSamplerWithTransform(
        Image* im, const AffineTransform* transform);

//=============================================================================
class GLSLSamplerParameter : public Firtree::ReferenceCounted
{
    protected:
        GLSLSamplerParameter(Image* im);

    public:
        virtual ~GLSLSamplerParameter();

        ///@{
        /// Overloaded methods from Firtree::SamplerParameter.
        virtual const Rect2D GetDomain() const;
        virtual const Rect2D GetExtent() const;
        virtual AffineTransform* GetAndOwnTransform() const { return m_Transform->Copy(); }
        ///@}
        
        inline static GLSLSamplerParameter* ExtractFrom(
                Firtree::SamplerParameter* s) {
            return s->GetWrappedGLSLSampler();
        }

        /// Return a (possibly cached) ARB shader program object which will
        /// render from this shader.
        int GetShaderProgramObject();

        /// Write any top-level GLSL for this shader into dest.
        virtual bool BuildTopLevelGLSL(std::string& dest) = 0;

        /// Write GLSL to assign result of sampling shader at
        /// samplerCoordVar to resultVar 
        virtual void BuildSampleGLSL(std::string& dest,
                const char* samplerCoordVar,
                const char* resultVar) = 0;

        virtual bool IsValid() const = 0;
        virtual void SetGLSLUniforms(unsigned int program) = 0;

        void SetSamplerIndex(int i) { m_SamplerIndex = i; }
        int GetSamplerIndex() const  { return m_SamplerIndex; }

        void SetBlockPrefix(const char* p) { m_BlockPrefix = p; }
        const char* GetBlockPrefix() const { return m_BlockPrefix.c_str(); }

        virtual void SetTransform(const AffineTransform* f);

        // Compute a digest for this sampler. This digest is 
        // composed of the child sampler digests (if they are
        // kernel samplers) and the current sampler state. In effect
        // this can be viewed as a hash for the results of calling
        // BuildTopLevelGLSL() and the current transform and
        // extent.
        virtual void ComputeDigest(uint8_t digest[20]) = 0;

        void SetOpenGLContext(OpenGLContext* glContext);
        OpenGLContext* GetOpenGLContext() const { return m_GLContext; }

    private:
        /// Affine transformation to map from world co-ordinates
        /// to sampler co-ordinates.
        AffineTransform*    m_Transform;

        int                 m_SamplerIndex;
        std::string         m_BlockPrefix;

        int                 m_CachedFragmentShaderObject;
        int                 m_CachedVertexShaderObject;
        int                 m_CachedProgramObject;
        uint8_t             m_CachedShaderDigest[20];

        Image*              m_RepresentedImage;

        OpenGLContext*      m_GLContext;
};

//=============================================================================
class TextureSamplerParameter : public GLSLSamplerParameter
{
    protected:
        TextureSamplerParameter(Image* image);

    public:
        virtual ~TextureSamplerParameter();

        static GLSLSamplerParameter* Create(Image* image);

        /// Overloaded methods from Firtree::SamplerParameter.
        virtual const Rect2D GetDomain() const { return m_Domain; }
        virtual AffineTransform* GetAndOwnTransform() const;
        ///@}

        /// Write any top-level GLSL for this shader into dest.
        virtual bool BuildTopLevelGLSL(std::string& dest);

        /// Write GLSL to assign result of sampling shader at
        /// samplerCoordVar to resultVar 
        virtual void BuildSampleGLSL(std::string& dest,
                const char* samplerCoordVar,
                const char* resultVar);

        virtual bool IsValid() const;
        virtual void SetGLSLUniforms(unsigned int program);

        virtual void ComputeDigest(uint8_t digest[20]);

        unsigned int GetGLTextureObject() const;

        void SetGLTextureUnit(unsigned int i) { m_TextureUnit = i; }
        unsigned int GetGLTextureUnit() const { return m_TextureUnit; }

    private:
        Internal::ImageImpl*    m_Image;
        Rect2D                  m_Domain;
        unsigned int            m_TextureUnit;
};

//=============================================================================
class KernelSamplerParameter : public GLSLSamplerParameter
{
    protected:
        KernelSamplerParameter(Image* im);

    public:
        virtual ~KernelSamplerParameter();

        static GLSLSamplerParameter* Create(Image* im);

        /// Write any top-level GLSL for this shader into dest.
        virtual bool BuildTopLevelGLSL(std::string& dest);

        /// Write GLSL to assign result of sampling shader at
        /// samplerCoordVar to resultVar 
        virtual void BuildSampleGLSL(std::string& dest,
                const char* samplerCoordVar,
                const char* resultVar);

        virtual bool IsValid() const;

        virtual void SetGLSLUniforms(unsigned int program);

        virtual void ComputeDigest(uint8_t digest[20]);

        // Build an entire shader for this sampler in GLSL.
        // virtual bool BuildGLSL(std::string& dest);

        CompiledGLSLKernel* GetKernel() const { return m_Kernel; }

        virtual void AddChildSamplersToVector(
                std::vector<SamplerParameter*>& sampVec);

        LLVM::SamplerProvider* GetSampler() const { return m_LLVMSampler; }

    private:
        CompiledGLSLKernel*         m_Kernel;
        LLVM::SamplerProvider*      m_LLVMSampler;
};

//=============================================================================
class CompiledGLSLKernel : public Firtree::ReferenceCounted
{
    protected:
        CompiledGLSLKernel(const char* source);

    public:
        virtual ~CompiledGLSLKernel();

        static CompiledGLSLKernel* CreateFromSource(const char* source);

        static CompiledGLSLKernel* ExtractFrom(Kernel* k)
        {
            return k->GetWrappedGLSLKernel();
        }

        void SetSource(const char* source);
        const char* GetSource() const { return m_Source.c_str(); }

        bool GetStatus() const { return m_CompileStatus; }
        const char* GetCompileLog() const { return m_InfoLog.c_str(); }

        Parameter* GetValueForKey(const char* key) const;

        void SetValueForKey(Parameter* param, const char* key);

        const std::vector<std::string>& GetParameterNames() const
            { return m_ParameterNames; }

        const std::map<std::string, Parameter*>& GetParameters() const
            { return m_Parameters; }

        bool Compile();
        bool GetIsCompiled() const { return m_IsCompiled; }

        void SetBlockName(const char* blockName);
        const char* GetBlockName() const { return m_BlockName.c_str(); }

        const char* GetCompiledGLSL() const;
        const char* GetCompiledKernelName() const;
        const char* GetInfoLog() const { return m_InfoLog.c_str(); }

        const char* GetUniformNameForKey(const char* key);

        const unsigned char* GetCompiledGLSLDigest() const 
        {
            return m_GLSLDigest;
        }

    private:
        std::map<std::string, Parameter*>   m_Parameters;
        std::vector<std::string>            m_ParameterNames;
        std::map<std::string, std::string>   m_UniformNameMap;
        std::string                     m_CompiledGLSL;
        std::string                     m_InfoLog;
        std::string                     m_CompiledKernelName;
        std::string                     m_Source;
        std::string                     m_BlockName;

        std::string                     m_BlockReplacedGLSL;
        std::string                     m_BlockReplacedKernelName;

        bool                            m_IsCompiled;
        bool                            m_CompileStatus;

        unsigned char                   m_GLSLDigest[20];

        void ClearParameters();

        Parameter* ParameterForKey(const char* key);
        NumericParameter* NumericParameterForKeyAndType(const char* key, 
                NumericParameter::BaseType type);
        SamplerParameter* SamplerParameterForKey(const char* key);
        void UpdateBlockNameReplacedSourceCache();
};

} }

//=============================================================================
#endif // FIRTREE_GLSL_RUNTIME_PRIV_H
//=============================================================================

//=============================================================================
// vim:sw=4:ts=4:cindent:et

