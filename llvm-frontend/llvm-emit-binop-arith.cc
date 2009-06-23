//===========================================================================
/// \file llvm-binop_arith.cc

#define __STDC_CONSTANT_MACROS

#include "llvm-frontend.h"
#include "llvm-private.h"
#include "llvm-emit-decl.h"
#include "llvm-expression.h"
#include "llvm-type-cast.h"
#include "llvm-emit-constant.h"

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

				/* Special case division, scalar division is *always* done
				 * as a floating point division. */
				if(op == Instruction::FDiv) {
					KernelTypeSpecifier left_spec = left_val->GetType().Specifier;
					if((left_spec == Firtree::TySpecInt) || 
							(left_spec == Firtree::TySpecBool)) {
						ExpressionValue* left_cast = NULL;
						ExpressionValue* right_cast = NULL;
						try {
							// Attempt to cast both sides.
							left_cast = TypeCaster::CastValue( context,
									expression,
									left_val, Firtree::TySpecFloat );
							right_cast = TypeCaster::CastValue( context,
									expression,
									right_val, Firtree::TySpecFloat );

							FIRTREE_SAFE_RELEASE( left_val );
							left_val = left_cast;
							FIRTREE_SAFE_RELEASE( right_val );
							right_val = right_cast;
						} catch ( CompileErrorException e ) {
							FIRTREE_SAFE_RELEASE( left_cast );
							FIRTREE_SAFE_RELEASE( right_cast );
							throw e;
						}
					}
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
