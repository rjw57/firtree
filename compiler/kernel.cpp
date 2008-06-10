
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
// This file implements the FIRTREE compiler utility functions.
//=============================================================================

#include "include/kernel.h"

#include <compiler/include/compiler.h>
#include <compiler/include/main.h>
#include <compiler/backends/glsl/glsl.h>
#include <compiler/backends/irdump/irdump.h>

namespace Firtree {

//=============================================================================
KernelParameter::KernelParameter(const char* name)
    :   m_Name(name)
{
}

//=============================================================================
KernelParameter::~KernelParameter()
{
}

//=============================================================================
KernelConstParameter::KernelConstParameter(const char* name)
    :   KernelParameter(name)
    ,   m_BaseType(KernelConstParameter::Float)
    ,   m_Size(0)
{
}

//=============================================================================
KernelConstParameter::~KernelConstParameter()
{
}

//=============================================================================
Kernel::Kernel()
{
}

//=============================================================================
Kernel::Kernel(const char* source)
{
    SetSource(source);
}

//=============================================================================
Kernel::~Kernel()
{
}

//=============================================================================
bool Kernel::Compile(const char* blockName)
{
    const char* pSrc = m_Source.c_str();

    m_Parameters.clear();
    m_CompiledGLSL.clear();
    m_InfoLog.clear();
    m_CompiledKernelName.clear();

    /*
    {
        IRDumpBackend irbe(stdout);
        Compiler irc(irbe);
        irc.Compile(&pSrc, 1);
    }
    */

    GLSLBackend be(blockName);
    Compiler c(be);
    bool rv = c.Compile(&pSrc, 1);
    m_InfoLog = c.GetInfoLog();
    if(!rv)
    {
        return false;
    }

    m_CompiledGLSL = be.GetOutput();
    m_CompiledKernelName = be.GetOutputKernelName();

    GLSLBackend::Parameters& params = be.GetInputParameters();
    for(GLSLBackend::Parameters::iterator i = params.begin();
            i != params.end(); i++)
    {
        GLSLBackend::Parameter& p = *i;

        switch(p.basicType)
        {
            case GLSLBackend::Parameter::Int:
            case GLSLBackend::Parameter::Float:
            case GLSLBackend::Parameter::Bool:
                {
                    KernelConstParameter kp(p.humanName.c_str());
                    kp.SetSize(p.vectorSize);
                    kp.SetGLSLName(p.uniformName);
                    kp.SetIsColor(p.isColor);

                    switch(p.basicType)
                    {
                        case GLSLBackend::Parameter::Int:
                            kp.SetBaseType(KernelConstParameter::Int);
                            break;
                        case GLSLBackend::Parameter::Float:
                            kp.SetBaseType(KernelConstParameter::Float);
                            break;
                        case GLSLBackend::Parameter::Bool:
                            kp.SetBaseType(KernelConstParameter::Bool);
                            break;
                    }

                    m_Parameters.push_back(kp);
                }
                break;
            default:
                FIRTREE_WARNING("Unhandled parameter type: %i", p.basicType);
                break;
        }
    }

    return true;
}

//=============================================================================
KernelSamplerParameter::KernelSamplerParameter(const char* name, Kernel& kernel)
    :   KernelParameter(name)
    ,   m_Kernel(kernel)
    ,   m_KernelCompileStatus(false)
{
}

//=============================================================================
KernelSamplerParameter::~KernelSamplerParameter()
{
}

//=============================================================================
void KernelSamplerParameter::SetBlockPrefix(const char* p)
{
    KernelParameter::SetBlockPrefix(p);

    m_KernelCompileStatus = m_Kernel.Compile(GetBlockPrefix());
}

//=============================================================================
void KernelSamplerParameter::BuildTopLevelGLSL(std::string& dest)
{
    dest = m_Kernel.GetCompiledGLSL();
}

//=============================================================================
void KernelSamplerParameter::BuildSampleGLSL(std::string& dest,
                const char* samplerCoordVar,
                const char* resultVar)
{
    std::string result = resultVar;
    result += " = ";
    result += m_Kernel.GetCompiledKernelName();
    result += "(";
    result += samplerCoordVar;
    result += ");";
    dest = result;
}

} // namespace Firtree 

//=============================================================================
// vim:sw=4:ts=4:cindent:et
