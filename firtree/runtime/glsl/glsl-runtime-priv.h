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
        GLSLSamplerParameter();
        virtual ~GLSLSamplerParameter();

    public:
        ///@{
        /// Overloaded methods from Firtree::SamplerParameter.
        virtual const Rect2D GetDomain() const;
        virtual const Rect2D GetExtent() const;
        virtual const AffineTransform* GetTransform() const { return m_Transform; }
        ///@}
        
        inline static GLSLSamplerParameter* ExtractFrom(
                Firtree::SamplerParameter* s) {
            return s->GetWrappedGLSLSampler();
        }

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

    private:
        /// Affine transformation to map from world co-ordinates
        /// to sampler co-ordinates.
        AffineTransform*    m_Transform;

        int                 m_SamplerIndex;
        std::string         m_BlockPrefix;
};

//=============================================================================
class TextureSamplerParameter : public GLSLSamplerParameter
{
    protected:
        TextureSamplerParameter(Image* image);
        virtual ~TextureSamplerParameter();

    public:
        static GLSLSamplerParameter* Create(Image* image);

        /// Overloaded methods from Firtree::SamplerParameter.
        virtual const Rect2D GetDomain() const { return m_Domain; }
        virtual const Rect2D GetExtent() const;
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
        virtual ~KernelSamplerParameter();

    public:
        static GLSLSamplerParameter* Create(Image* im);

        /// Write any top-level GLSL for this shader into dest.
        virtual bool BuildTopLevelGLSL(std::string& dest);

        /// Write GLSL to assign result of sampling shader at
        /// samplerCoordVar to resultVar 
        virtual void BuildSampleGLSL(std::string& dest,
                const char* samplerCoordVar,
                const char* resultVar);

        virtual bool IsValid() const { return m_KernelCompileStatus; }
        virtual void SetGLSLUniforms(unsigned int program);

        // Build an entire shader for this sampler in GLSL.
        // virtual bool BuildGLSL(std::string& dest);

        CompiledGLSLKernel* GetKernel() const { return m_Kernel; }

    private:
        CompiledGLSLKernel*         m_Kernel;
        bool            m_KernelCompileStatus;

    protected:
        virtual void AddChildSamplersToVector(
                std::vector<SamplerParameter*>& sampVec);

        friend bool BuildGLSLShaderForSampler(std::string&,
                GLSLSamplerParameter*);
};

//=============================================================================
class CompiledGLSLKernel : public Firtree::ReferenceCounted
{
    protected:
        CompiledGLSLKernel(const char* source);
        virtual ~CompiledGLSLKernel();

    public:
        static CompiledGLSLKernel* CreateFromSource(const char* source);

        void SetSource(const char* source);
        const char* GetSource() const { return m_Source.c_str(); }

        void SetValueForKey(Parameter* param, const char* key);

        const std::map<std::string, Parameter*>& GetParameters()
            { return m_Parameters; }

        void Compile();
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
        std::map<std::string, std::string>   m_UniformNameMap;
        std::string                     m_CompiledGLSL;
        std::string                     m_InfoLog;
        std::string                     m_CompiledKernelName;
        std::string                     m_Source;
        std::string                     m_BlockName;

        std::string                     m_BlockReplacedGLSL;
        std::string                     m_BlockReplacedKernelName;

        bool                            m_IsCompiled;

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

