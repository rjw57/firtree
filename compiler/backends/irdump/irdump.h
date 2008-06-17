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
// This file defines the interface to the FIRTREE IR dumper backend.
//=============================================================================

//=============================================================================
#ifndef FIRTREE_IRDUMP_H
#define FIRTREE_IRDUMP_H
//=============================================================================

#include <public/include/main.h>

#include <stdio.h>

#include <compiler/include/compiler.h>

// Forward declaration of intermediate representation node (from GLSL frontend).
class TIntermNode;

namespace Firtree {

//=============================================================================
// A simple intermediate representation dumping backend.
class IRDumpBackend : public Backend
{
    public:
        IRDumpBackend(FILE* outputStream); 
        virtual ~IRDumpBackend();

        bool Generate(TIntermNode* root);
        FILE* GetOutputStream() { return m_pOutput; }

    protected:
        FILE*       m_pOutput;
};

} // namespace Firtree

//=============================================================================
#endif // FIRTREE_IRDUMP_H
//=============================================================================

//=============================================================================
// vim:sw=4:ts=4:cindent:et
