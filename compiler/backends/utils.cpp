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
// This file implements the FIRTREE compiler utility functions.
//=============================================================================

#include "utils.h"

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
const char* OperatorCodeToDescription(int opCode)
{
    if((opCode < EOpNull) || (opCode > EOpArrayLength))
        return "???";

    return _OperatorNames[opCode];
}

} // namespace Firtree 

//=============================================================================
// vim:sw=4:ts=4:cindent:et
