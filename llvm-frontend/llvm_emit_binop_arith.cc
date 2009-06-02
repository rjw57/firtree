//===========================================================================
/// \file llvm_binop_arith.cc

#define __STDC_CONSTANT_MACROS

#include "llvm_frontend.h"
#include "llvm_private.h"
#include "llvm_emit_decl.h"
#include "llvm_expression.h"
#include "llvm_type_cast.h"
#include "llvm_emit_constant.h"

#include <llvm/Instructions.h>

using namespace llvm;

namespace Firtree
{

//===========================================================================
/// \brief Class to emit an arithmetic binary operator.
class BinaryOpArithEmitter : ExpressionEmitter
{
	public:
		BinaryOpArithEmitter()
				: ExpressionEmitter() {
		}

		virtual ~BinaryOpArithEmitter() {
		}

		/// Create an instance of this emitter.
		static ExpressionEmitter* Create() {
			return new BinaryOpArithEmitter();
		}

		/// Emit the passed expression term returning a pointer to
		/// a ExpressionValue containing it's value. It is the
		/// responsibility of the caller to Release() this value.
		virtual ExpressionValue* Emit( LLVMContext* context,
		                               firtreeExpression expression ) {
			firtreeExpression left, right;

			// Emit code for the left and right.
			ExpressionValue* left_val = NULL;
			ExpressionValue* right_val = NULL;
			ExpressionValue* return_val = NULL;

			Instruction::BinaryOps op = Instruction::BinaryOpsEnd;

			if ( firtreeExpression_add( expression, &left, &right ) ) {
				op = Instruction::Add;
			} else if ( firtreeExpression_sub( expression, &left, &right ) ) {
				op = Instruction::Sub;
			} else if ( firtreeExpression_mul( expression, &left, &right ) ) {
				op = Instruction::Mul;
			} else if ( firtreeExpression_div( expression, &left, &right ) ) {
				op = Instruction::FDiv;
			} else {
				FIRTREE_LLVM_ICE( context, expression,
				                  "Unknown arithmetic operator." );
			}

			try {
				left_val = ExpressionEmitterRegistry::GetRegistry()->Emit(
				               context, left );
				right_val = ExpressionEmitterRegistry::GetRegistry()->Emit(
				                context, right );

				if ( ! TypeCaster::MassageBinOpTypes( context, expression,
				                          left_val, right_val,
				                          &left_val, &right_val ) ) {
					FIRTREE_LLVM_ERROR( context, expression,
					                    "Incompatible types for binary "
					                    "operator." );
				}

				llvm::Value* result_val = BinaryOperator::Create( op,
				                          left_val->GetLLVMValue(),
				                          right_val->GetLLVMValue(),
				                          "tmp", context->BB );
				return_val =  ConstantExpressionValue::
				              Create( context, result_val );

				FIRTREE_SAFE_RELEASE( left_val );
				FIRTREE_SAFE_RELEASE( right_val );
			} catch ( CompileErrorException e ) {
				FIRTREE_SAFE_RELEASE( left_val );
				FIRTREE_SAFE_RELEASE( right_val );
				throw e;
			}

			return return_val;
		}
};

//===========================================================================
// Register the emitter.
FIRTREE_LLVM_DECLARE_EMITTER(BinaryOpArithEmitter, add)
FIRTREE_LLVM_DECLARE_EMITTER(BinaryOpArithEmitter, sub)
FIRTREE_LLVM_DECLARE_EMITTER(BinaryOpArithEmitter, mul)
FIRTREE_LLVM_DECLARE_EMITTER(BinaryOpArithEmitter, div)
/*
FIRTREE_LLVM_DECLARE_EMITTER(BinaryOpArithEmitter, logicalor)
FIRTREE_LLVM_DECLARE_EMITTER(BinaryOpArithEmitter, logicaland)
FIRTREE_LLVM_DECLARE_EMITTER(BinaryOpArithEmitter, logicalxor)
FIRTREE_LLVM_DECLARE_EMITTER(BinaryOpArithEmitter, equal)
FIRTREE_LLVM_DECLARE_EMITTER(BinaryOpArithEmitter, notequal)
FIRTREE_LLVM_DECLARE_EMITTER(BinaryOpArithEmitter, greater)
FIRTREE_LLVM_DECLARE_EMITTER(BinaryOpArithEmitter, greaterequal)
FIRTREE_LLVM_DECLARE_EMITTER(BinaryOpArithEmitter, less)
FIRTREE_LLVM_DECLARE_EMITTER(BinaryOpArithEmitter, lessequal)
*/

}

// vim:sw=4:ts=4:cindent:noet
