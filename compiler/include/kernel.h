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
#ifndef FIRTREE_KERNEL_H
#define FIRTREE_KERNEL_H
//=============================================================================

#include <stdlib.h>
#include <string>
#include <vector>

namespace Firtree {

class KernelConstParameter;
class KernelSamplerParameter;
class Kernel;

//=============================================================================
class KernelParameter
{
    public:
        KernelParameter(const char* name);
        virtual ~KernelParameter();

        virtual void SetBlockPrefix(const char* p) { m_BlockPrefix = p; }
        virtual const char* GetBlockPrefix() const { return m_BlockPrefix.c_str(); }

        const std::string& GetName() const { return m_Name; }

        const std::string& GetGLSLName() const { return m_GLSLName; }
        void SetGLSLName(const std::string& n) { m_GLSLName = n; }

        virtual KernelConstParameter* GetAsConst() { return NULL; }
        virtual KernelSamplerParameter* GetAsSampler() { return NULL; }

    private:
        std::string         m_Name;
        std::string         m_GLSLName;
        std::string         m_BlockPrefix;
};

//=============================================================================
class KernelConstParameter : public KernelParameter
{
    public:
        enum BaseType {
            Int, Float, Bool,
        };

    public:
        KernelConstParameter(const char* name);
        virtual ~KernelConstParameter();

        virtual KernelConstParameter* GetAsConst() { return this; }

        float GetFloatValue(int idx) { return m_Value[idx].f; }
        int GetIntValue(int idx) { return m_Value[idx].i; }
        bool GetBoolValue(int idx) { return m_Value[idx].i != 0; }

        void SetFloatValue(float f, int idx) { m_Value[idx].f = f; }
        void SetIntValue(int i, int idx) { m_Value[idx].i = i; }
        void SetBoolValue(bool b, int idx) { m_Value[idx].i = b ? 1 : 0; }

        BaseType GetBaseType() { return m_BaseType; }
        void SetBaseType(BaseType bt) { m_BaseType = bt; }

        int GetSize() { return m_Size; }
        void SetSize(int s) { m_Size = s; }

        bool IsColor() { return m_IsColor; }
        void SetIsColor(bool flag) { m_IsColor = flag; }

    private:
        BaseType    m_BaseType;
        int         m_Size;
        union {
            float f;
            int i;      /* 0/1 for bool */
        }           m_Value[4];
        bool        m_IsColor : 1;
};

//=============================================================================
class KernelSamplerParameter : public KernelParameter
{
    public:
        KernelSamplerParameter(const char* name, Kernel& kernel);
        virtual ~KernelSamplerParameter();

        virtual KernelSamplerParameter* GetAsSampler() { return this; }

        virtual void SetBlockPrefix(const char* p);

        /// Write any top-level GLSL for this shader into dest.
        virtual void BuildTopLevelGLSL(std::string& dest);

        /// Write GLSL to assign result of sampling shader at
        /// samplerCoordVar to resultVar 
        virtual void BuildSampleGLSL(std::string& dest,
                const char* samplerCoordVar,
                const char* resultVar);

        virtual bool IsValid() const { return m_KernelCompileStatus; }

        const Kernel& GetKernel() const { return m_Kernel; }

        void SetGLSLUniforms(unsigned int program);

    private:
        Kernel&         m_Kernel;
        bool            m_KernelCompileStatus;
};

//=============================================================================
class Kernel
{
    public:
        Kernel();
        Kernel(const char* source);
        ~Kernel();

        void SetSource(const char* source) { m_Source = source; }
        const char* GetSource() const { return m_Source.c_str(); }

        bool Compile(const char* blockName);

        const char* GetCompiledGLSL() const { return m_CompiledGLSL.c_str(); }
        const char* GetCompiledKernelName() const { return m_CompiledKernelName.c_str(); }
        const char* GetInfoLog() const { return m_InfoLog.c_str(); }

        void SetValueForKey(float value, const char* key);

        std::vector<KernelParameter*>& GetParameters() { return m_Parameters; }

    private:
        std::vector<KernelParameter*>   m_Parameters;
        std::string                     m_CompiledGLSL;
        std::string                     m_InfoLog;
        std::string                     m_CompiledKernelName;
        std::string                     m_Source;

        void ClearParameters();

        KernelParameter* ParameterForKey(const char* key);
        KernelConstParameter* ConstParameterForKeyAndType(const char* key, 
                KernelConstParameter::BaseType type);
        KernelSamplerParameter* SamplerParameterForKey(const char* key);
};

}

//=============================================================================
#endif // FIRTREE_KERNEL_H
//=============================================================================

//=============================================================================
// vim:sw=4:ts=4:cindent:et

