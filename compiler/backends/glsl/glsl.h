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
// This file defines the interface to the FIRTREE GLSL backend.
//=============================================================================

//=============================================================================
#ifndef FIRTREE_GLSL_H
#define FIRTREE_GLSL_H
//=============================================================================

#include <compiler/include/compiler.h>
#include <compiler/include/main.h>

#include <stdio.h>
#include <vector>

// Forward declaration of intermediate representation node (from GLSL frontend).
class TIntermNode;
class TIntermSymbol;
class TIntermConstantUnion;
class TIntermBinary;
class TIntermUnary;
class TIntermSelection;
class TIntermAggregate;
class TIntermLoop;
class TIntermBranch;
class TIntermTraverser;
class TType;

namespace Firtree {

//=============================================================================
// The GLSL backend
class GLSLBackend : public Backend
{
    public:
        // A kernel input parameter
        struct Parameter {
            enum {
                Int, 
                Float,
                Bool,
                Sampler,
            }                       basicType;
            int                     vectorSize; //< In range [1,4].
            std::string             humanName;  //< The human readable name from
                                                //< The original dource.
            std::string             uniformName;    //< The name used in the
                                                    //< kernel's uniform structure.
                                                   
            bool                    isColor : 1;
        };

        typedef std::vector<Parameter> Parameters;

    public:
        GLSLBackend(const char* prefix);
        virtual ~GLSLBackend();

        bool Generate(TIntermNode* root);
        
        const char* GetOutput() { return m_Output.c_str(); }
        
        Parameters& GetInputParameters() { return m_InputParameters; }

    protected:
        std::string     m_Prefix;
        std::string     m_Output;
        bool            m_SuccessFlag;

        bool            m_InParams;
        bool            m_InFunction;
        bool            m_InKernel;
        bool            m_ProcessedOneParam;

        Parameters      m_InputParameters;

        struct Priv;
        Priv*           m_Priv;

        void VisitSymbol(TIntermSymbol* n);
        void VisitConstantUnion(TIntermConstantUnion* n);
        bool VisitBinary(bool preVisit, TIntermBinary* n);
        bool VisitUnary(bool preVisit, TIntermUnary* n);
        bool VisitSelection(bool preVisit, TIntermSelection* n);
        bool VisitAggregate(bool preVisit, TIntermAggregate* n);
        bool VisitLoop(bool preVisit, TIntermLoop* n);
        bool VisitBranch(bool preVisit, TIntermBranch* n);

        void AppendGLSLType(TType* t);
        void AppendPrefix() { AppendOutput(m_Prefix.c_str()); }

        void AppendOutput(const char* s, ...);

        void AddSymbol(int id, const char* typePrefix);
        const char* GetSymbol(int id);

        const char* AddFunction(const char* name);
        const char* GetFunction(const char* name);

        const char* AddTemporary(); 
        void PushTemporary(const char* t);
        const char* PopTemporary();

        friend class GLSLTrav;
};

} // namespace Firtree

//=============================================================================
#endif // FIRTREE_GLSL_H
//=============================================================================

//=============================================================================
// vim:sw=4:ts=4:cindent:et
