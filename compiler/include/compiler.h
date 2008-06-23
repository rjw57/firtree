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
// This file defined the interface to the FIRTREE compiler.
//=============================================================================

//=============================================================================
#ifndef FIRTREE_COMPILER_H
#define FIRTREE_COMPILER_H
//=============================================================================

// Forward declaration of intermediate representation node (from GLSL frontend).
class TIntermNode;

namespace Firtree {

//=============================================================================
// A FIRTREE code generation backend. A pure-virtual class
class Backend
{
    public:
        Backend() { }
        virtual ~Backend() { }
        virtual bool Generate(TIntermNode* root) = 0;
};

//=============================================================================
// A null-backend. Does no code generation
class NullBackend : public Backend
{
    public:
        NullBackend() { }
        virtual ~NullBackend() { }

        bool Generate(TIntermNode* root) { return true; }
};

//=============================================================================
class Compiler
{
    public:
        Compiler(Backend& backend);
        ~Compiler();

        bool Compile(const char** sourceLines, unsigned int lineCount);

        const char* GetInfoLog();

    protected:
        struct PrivData;

        Backend&        m_backend;
        PrivData*       m_Priv;
};

}

//=============================================================================
#endif // FIRTREE_COMPILER_H
//=============================================================================

//=============================================================================
// vim:sw=4:ts=4:cindent:et
