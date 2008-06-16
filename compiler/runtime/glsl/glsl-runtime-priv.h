/* 
 * FIRTREE - A generic image processing system.
 * Copyright (C) 2008 Rich Wareham <srichwareham@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 as published by
 * the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program; if not, write to the Free Software Foundation, Inc., 51
 * Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

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

#include <compiler/include/kernel.h>

#include "glsl-runtime.h"

namespace Firtree { namespace GLSL {

class SamplerParameter;
class Kernel;

//=============================================================================
class SamplerParameter : public Firtree::SamplerParameter
{
    protected:
        SamplerParameter();
        virtual ~SamplerParameter();

    public:
        virtual SamplerParameter* GetAsSampler() { return this; }

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

    private:
        int             m_SamplerIndex;
        std::string     m_BlockPrefix;
};

//=============================================================================
class TextureSamplerParameter : public GLSL::SamplerParameter
{
    protected:
        TextureSamplerParameter(unsigned int texObj);
        virtual ~TextureSamplerParameter();

    public:
        static SamplerParameter* Create(unsigned int texObj);

        virtual const Rect2D GetExtent() const;

        /// Write any top-level GLSL for this shader into dest.
        virtual bool BuildTopLevelGLSL(std::string& dest);

        /// Write GLSL to assign result of sampling shader at
        /// samplerCoordVar to resultVar 
        virtual void BuildSampleGLSL(std::string& dest,
                const char* samplerCoordVar,
                const char* resultVar);

        virtual bool IsValid() const;
        virtual void SetGLSLUniforms(unsigned int program);

        unsigned int GetGLTextureObject() const { return m_TexObj; }

        void SetGLTextureUnit(unsigned int i) { m_TextureUnit = i; }
        unsigned int GetGLTextureUnit() const { return m_TextureUnit; }

    private:
        Rect2D              m_Definition;
        unsigned int        m_TextureUnit;
        unsigned int        m_TexObj;
};

//=============================================================================
class KernelSamplerParameter : public Firtree::GLSL::SamplerParameter
{
    protected:
        KernelSamplerParameter(Firtree::Kernel* kernel);
        virtual ~KernelSamplerParameter();

    public:
        static SamplerParameter* Create(Firtree::Kernel* kernel);

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

        Kernel* GetKernel() const { return m_Kernel; }

    private:
        Kernel*         m_Kernel;
        bool            m_KernelCompileStatus;

    protected:
        virtual void AddChildSamplersToVector(
                std::vector<GLSL::SamplerParameter*>& sampVec);

        friend bool BuildGLSLShaderForSampler(std::string&, Firtree::SamplerParameter*);
};

//=============================================================================
class Kernel : public Firtree::Kernel
{
    protected:
        Kernel();
        Kernel(const char* source);
        virtual ~Kernel();

    public:
        static Firtree::Kernel* Create();
        static Firtree::Kernel* Create(const char* source);

        virtual void SetSource(const char* source);
        virtual const char* GetSource() const { return m_Source.c_str(); }

        virtual void SetValueForKey(float value, const char* key);
        virtual void SetValueForKey(const float* value, int count, const char* key);
        virtual void SetValueForKey(int value, const char* key);
        virtual void SetValueForKey(const int* value, int count, const char* key);
        virtual void SetValueForKey(bool value, const char* key);
        virtual void SetValueForKey(const bool* value, int count, const char* key);
        virtual void SetValueForKey(Firtree::SamplerParameter* sampler, const char* key);

        std::map<std::string, Parameter*>& GetParameters() { return m_Parameters; }

        bool GetIsCompiled() const { return m_IsCompiled; }

        void SetBlockName(const char* blockName);
        const char* GetBlockName() const { return m_BlockName.c_str(); }

        const char* GetCompiledGLSL() const;
        const char* GetCompiledKernelName() const;
        const char* GetInfoLog() const { return m_InfoLog.c_str(); }

        const char* GetUniformNameForKey(const char* key);

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

        void ClearParameters();

        Parameter* ParameterForKey(const char* key);
        NumericParameter* NumericParameterForKeyAndType(const char* key, 
                NumericParameter::BaseType type);
        KernelSamplerParameter* SamplerParameterForKey(const char* key);
        void UpdateBlockNameReplacedSourceCache();
};

} }

//=============================================================================
#endif // FIRTREE_GLSL_RUNTIME_PRIV_H
//=============================================================================

//=============================================================================
// vim:sw=4:ts=4:cindent:et

