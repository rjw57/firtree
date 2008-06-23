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
// This file implements the FIRTREE IR dumper backend.
//=============================================================================

#include "irdump.h"

#include "glslang/Include/ShHandle.h"
#include "glslang/Include/intermediate.h"
#include "glslang/Public/ShaderLang.h"

#include "../utils.h"

namespace Firtree {

//=============================================================================
class IRTrav : public TIntermTraverser
{
    public:
        IRTrav(IRDumpBackend& be) : TIntermTraverser(), m_Backend(be) { }
        FILE* os() { return m_Backend.GetOutputStream(); }

    protected:
        IRDumpBackend&  m_Backend;
};

//=============================================================================
IRDumpBackend::IRDumpBackend(FILE* outputStream)
    :   m_pOutput(outputStream)
{
}

//=============================================================================
IRDumpBackend::~IRDumpBackend()
{
}

//=============================================================================
void irPrintNodeHeader(TIntermNode* n, TIntermTraverser* t)
{
    IRTrav* trav = reinterpret_cast<IRTrav*>(t);
    // fprintf(trav->os(), "<!-- input line: %i --> ", n->getLine());
    for(int i=0; i<t->depth; i++)
    {
        fprintf(trav->os(), "  ");
    }
}

//=============================================================================
void irVisitSymbol(TIntermSymbol* s, TIntermTraverser* t)
{
    irPrintNodeHeader(s,t);
    IRTrav* trav = reinterpret_cast<IRTrav*>(t);
    fprintf(trav->os(), "<symbol name=\"%s\" />\n", s->getSymbol().c_str());
}

//=============================================================================
void irVisitConstantUnion(TIntermConstantUnion* s, TIntermTraverser* t)
{
    irPrintNodeHeader(s,t);
    IRTrav* trav = reinterpret_cast<IRTrav*>(t);
    fprintf(trav->os(), "<constunion />\n");
}

//=============================================================================
bool irVisitBinary(bool preVisit, TIntermBinary* s, TIntermTraverser* t)
{
    irPrintNodeHeader(s,t);
    IRTrav* trav = reinterpret_cast<IRTrav*>(t);
    if(preVisit)
    {
        fprintf(trav->os(), "<binop op=\"%s\">\n", OperatorCodeToDescription(s->getOp()));
    } else {
        fprintf(trav->os(), "</binop>\n");
    }
    return true;
}

//=============================================================================
bool irVisitUnary(bool preVisit, TIntermUnary* s, TIntermTraverser* t)
{
    irPrintNodeHeader(s,t);
    IRTrav* trav = reinterpret_cast<IRTrav*>(t);
    if(preVisit)
    {
        fprintf(trav->os(), "<unop op=\"%s\">\n", OperatorCodeToDescription(s->getOp()));
    } else {
        fprintf(trav->os(), "</unop>\n");
    }
    return true;
}

//=============================================================================
bool irVisitSelection(bool preVisit, TIntermSelection* s, TIntermTraverser* t)
{
    irPrintNodeHeader(s,t);
    IRTrav* trav = reinterpret_cast<IRTrav*>(t);
    if(preVisit)
    {
        fprintf(trav->os(), "<selection>\n");
    } else {
        fprintf(trav->os(), "</selection>\n");
    }
    return true;
}

//=============================================================================
bool irVisitAggregate(bool preVisit, TIntermAggregate* s, TIntermTraverser* t)
{
    irPrintNodeHeader(s,t);
    IRTrav* trav = reinterpret_cast<IRTrav*>(t);
    if(preVisit)
    {
        fprintf(trav->os(), "<aggregate op=\"%s\">\n", OperatorCodeToDescription(s->getOp()));
    } else {
        fprintf(trav->os(), "</aggregate>\n");
    }
    return true;
}

//=============================================================================
bool irVisitLoop(bool preVisit, TIntermLoop* s, TIntermTraverser* t)
{
    irPrintNodeHeader(s,t);
    IRTrav* trav = reinterpret_cast<IRTrav*>(t);
    if(preVisit)
    {
        fprintf(trav->os(), "<loop first=\"%s\">\n", s->testFirst() ? "true" : "false" );
    } else {
        fprintf(trav->os(), "</loop>\n");
    }
    return true;
}

//=============================================================================
bool irVisitBranch(bool preVisit, TIntermBranch* s, TIntermTraverser* t)
{
    irPrintNodeHeader(s,t);
    IRTrav* trav = reinterpret_cast<IRTrav*>(t);
    if(preVisit)
    {
        fprintf(trav->os(), "<branch op=\"%s\">\n", OperatorCodeToDescription(s->getFlowOp()));
    } else {
        fprintf(trav->os(), "</branch>\n");
    }
    return true;
}

//=============================================================================
bool IRDumpBackend::Generate(TIntermNode* root)
{
    fprintf(m_pOutput, "<irtree>\n");

    IRTrav traverser(*this);

    traverser.visitSymbol = irVisitSymbol;
    traverser.visitConstantUnion = irVisitConstantUnion;
    traverser.visitBinary = irVisitBinary;
    traverser.visitUnary = irVisitUnary;
    traverser.visitSelection = irVisitSelection;
    traverser.visitAggregate = irVisitAggregate;
    traverser.visitLoop = irVisitLoop;
    traverser.visitBranch = irVisitBranch;

    traverser.preVisit = true;
    traverser.postVisit = true;

    root->traverse(&traverser);

    fprintf(m_pOutput, "</irtree>\n");

    return true;
}

} // namespace Firtree

//=============================================================================
// vim:sw=4:ts=4:cindent:et
