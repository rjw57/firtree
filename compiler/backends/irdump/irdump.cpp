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
// This file implements the FIRTREE IR dumper backend.
//=============================================================================

#include "irdump.h"

#include "glslang/Include/ShHandle.h"
#include "glslang/Include/intermediate.h"
#include "glslang/Public/ShaderLang.h"

namespace Firtree {

//=============================================================================
const char* _OperatorNames[] = {
    "EOpNull",            // if in a node", should only mean a node is still being built
    "EOpSequence",        // denotes a list of statements, or parameters", etc.
    "EOpFunctionCall",    
    "EOpFunction",        // For function definition
    "EOpKernel",          // For kernel definition. FIRTREE only
    "EOpParameters",      // an aggregate listing the parameters to a function

    //
    // Unary operators
    //
    
    "EOpNegative",
    "EOpLogicalNot",
    "EOpVectorLogicalNot",
    "EOpBitwiseNot",

    "EOpPostIncrement",
    "EOpPostDecrement",
    "EOpPreIncrement",
    "EOpPreDecrement",

    "EOpConvIntToBool",
    "EOpConvFloatToBool",
    "EOpConvBoolToFloat",
    "EOpConvIntToFloat",
    "EOpConvFloatToInt",
    "EOpConvBoolToInt",

    //
    // binary operations
    //

    "EOpAdd",
    "EOpSub",
    "EOpMul",
    "EOpDiv",
    "EOpMod",
    "EOpRightShift",
    "EOpLeftShift",
    "EOpAnd",
    "EOpInclusiveOr",
    "EOpExclusiveOr",
    "EOpEqual",
    "EOpNotEqual",
    "EOpVectorEqual",
    "EOpVectorNotEqual",
    "EOpLessThan",
    "EOpGreaterThan",
    "EOpLessThanEqual",
    "EOpGreaterThanEqual",
    "EOpComma",

    "EOpVectorTimesScalar",
    "EOpVectorTimesMatrix",
    "EOpMatrixTimesVector",
    "EOpMatrixTimesScalar",

    "EOpLogicalOr",
    "EOpLogicalXor",
    "EOpLogicalAnd",

    "EOpIndexDirect",
    "EOpIndexIndirect",
    "EOpIndexDirectStruct",

    "EOpVectorSwizzle",

    //
    // Built-in functions potentially mapped to operators
    //

    "EOpRadians",
    "EOpDegrees",
    "EOpSin",
    "EOpCos",
    "EOpTan",
    "EOpAsin",
    "EOpAcos",
    "EOpAtan",

    //
    // FIRTREE only
    // 
    "EOpSinRange",
    "EOpCosRange",
    "EOpTanRange",
    "EOpSinCos",
    "EOpCosSin",
    "EOpSinCosRange",
    "EOpCosSinRange",

    "EOpPow",
    "EOpExp",
    "EOpLog",
    "EOpExp2",
    "EOpLog2",
    "EOpSqrt",
    "EOpInverseSqrt",

    "EOpAbs",
    "EOpSign",
    "EOpFloor",
    "EOpCeil",
    "EOpFract",
    "EOpMin",
    "EOpMax",
    "EOpClamp",
    "EOpMix",
    "EOpStep",
    "EOpSmoothStep",

    "EOpLength",
    "EOpDistance",
    "EOpDot",
    "EOpCross",
    "EOpNormalize",
    "EOpFaceForward",
    "EOpReflect",
    "EOpRefract",

    "EOpDPdx",            // Fragment only
    "EOpDPdy",            // Fragment only
    "EOpFwidth",          // Fragment only

    "EOpMatrixTimesMatrix",

    "EOpAny",
    "EOpAll",
    
    "EOpItof",         // pack/unpack only
    "EOpFtoi",         // pack/unpack only    
    "EOpSkipPixels",   // pack/unpack only
    "EOpReadInput",    // unpack only
    "EOpWritePixel",   // unpack only
    "EOpBitmapLsb",    // unpack only
    "EOpBitmapMsb",    // unpack only
    "EOpWriteOutput",  // pack only
    "EOpReadPixel",    // pack only

    "EOpDestCoord",    // FIRTREE only
    "EOpCompare",    // FIRTREE only
    "EOpPremultiply",    // FIRTREE only
    "EOpUnPremultiply",    // FIRTREE only
    "EOpSample",    // FIRTREE only
    "EOpSamplerCoord",    // FIRTREE only
    "EOpSamplerExtent",    // FIRTREE only
    "EOpSamplerOrigin",    // FIRTREE only
    "EOpSamplerSize",    // FIRTREE only
    "EOpSamplerTransform",    // FIRTREE only

    //
    // Branch
    //

    "EOpKill",            // Fragment only
    "EOpReturn",
    "EOpBreak",
    "EOpContinue",

    //
    // Constructors
    //

    "EOpConstructInt",
    "EOpConstructBool",
    "EOpConstructFloat",
    "EOpConstructVec2",
    "EOpConstructVec3",
    "EOpConstructVec4",
    "EOpConstructBVec2",
    "EOpConstructBVec3",
    "EOpConstructBVec4",
    "EOpConstructIVec2",
    "EOpConstructIVec3",
    "EOpConstructIVec4",
    "EOpConstructMat2",
    "EOpConstructMat3",
    "EOpConstructMat4",
    "EOpConstructStruct",

    //
    // moves
    //
    
    "EOpAssign",
    "EOpAddAssign",
    "EOpSubAssign",
    "EOpMulAssign",
    "EOpVectorTimesMatrixAssign",
    "EOpVectorTimesScalarAssign",
    "EOpMatrixTimesScalarAssign",
    "EOpMatrixTimesMatrixAssign",
    "EOpDivAssign",
    "EOpModAssign",
    "EOpAndAssign",
    "EOpInclusiveOrAssign",
    "EOpExclusiveOrAssign",
    "EOpLeftShiftAssign",
    "EOpRightShiftAssign",

    //
    // Array operators
    //

    "EOpArrayLength",
};

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
    fprintf(trav->os(), "<!-- input line: %i --> ", n->getLine());
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
        fprintf(trav->os(), "<binop op=\"%s\">\n", _OperatorNames[s->getOp()]);
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
        fprintf(trav->os(), "<unop op=\"%s\">\n", _OperatorNames[s->getOp()]);
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
        fprintf(trav->os(), "<aggregate op=\"%s\">\n", _OperatorNames[s->getOp()]);
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
        fprintf(trav->os(), "<branch op=\"%s\">\n", _OperatorNames[s->getFlowOp()]);
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
