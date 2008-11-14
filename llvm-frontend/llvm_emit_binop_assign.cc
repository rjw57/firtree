//===========================================================================
/// \file llvm_binop_assign.cc

#include <firtree/main.h>

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
/// \brief Class to emit an assignment binary operator.
class BinaryOpAssignEmitter : ExpressionEmitter
{
	public:
		BinaryOpAssignEmitter()
				: ExpressionEmitter() {
		}

		virtual ~BinaryOpAssignEmitter() {
		}

		/// Create an instance of this emitter.
		static ExpressionEmitter* Create() {
			return new BinaryOpAssignEmitter();
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

			if ( firtreeExpression_assign( expression, &left, &right ) ) {
				/* We signal assignment by op == Instruction::BinaryOpsEnd */
				op = Instruction::BinaryOpsEnd;
			} else if ( firtreeExpression_addassign( expression, &left, &right ) ) {
				op = Instruction::Add;
			} else if ( firtreeExpression_subassign( expression, &left, &right ) ) {
				op = Instruction::Sub;
			} else if ( firtreeExpression_mulassign( expression, &left, &right ) ) {
				op = Instruction::Mul;
			} else if ( firtreeExpression_divassign( expression, &left, &right ) ) {
				op = Instruction::FDiv;
			} else {
				FIRTREE_LLVM_ICE( context, expression,
				                  "Unknown assignment operator." );
			}

			try {
				left_val = ExpressionEmitterRegistry::GetRegistry()->Emit(
				               context, left );
				right_val = ExpressionEmitterRegistry::GetRegistry()->Emit(
				                context, right );

				// For assignment, the right value must have same type as
				// left.
				// The 'value' of the assignment expression is the right hand
				// value.
				return_val = TypeCaster::CastValue( context,
				                                    expression,
				                                    right_val,
				                                    left_val->GetType().Specifier );

				// If the assignment involves an arithmetic instruction,
				// emit it.
				if ( op != Instruction::BinaryOpsEnd ) {
					llvm::Value* result_val = BinaryOperator::create( op,
					                          left_val->GetLLVMValue(),
					                          return_val->GetLLVMValue(),
					                          "tmpbinop", context->BB );

					FIRTREE_SAFE_RELEASE( return_val );
					return_val = ConstantExpressionValue::
					             Create( context, result_val );
				}

				// Now check that the left_value is mutable.
				if ( !left_val->IsMutable() ) {
					FIRTREE_LLVM_ERROR( context, expression,
					                    "Cannot assign to non mutable "
					                    "expression." );
				}

				// Perform assignment
				left_val->AssignFrom( *return_val );

				FIRTREE_SAFE_RELEASE( left_val );
				FIRTREE_SAFE_RELEASE( right_val );
			} catch ( CompileErrorException e ) {
				FIRTREE_SAFE_RELEASE( left_val );
				FIRTREE_SAFE_RELEASE( right_val );
				FIRTREE_SAFE_RELEASE( return_val );
				throw e;
			}

			return return_val;
		}
};

//===========================================================================
// Register the emitter.
FIRTREE_LLVM_DECLARE_EMITTER(BinaryOpAssignEmitter, assign)
FIRTREE_LLVM_DECLARE_EMITTER(BinaryOpAssignEmitter, addassign)
FIRTREE_LLVM_DECLARE_EMITTER(BinaryOpAssignEmitter, subassign)
FIRTREE_LLVM_DECLARE_EMITTER(BinaryOpAssignEmitter, mulassign)
FIRTREE_LLVM_DECLARE_EMITTER(BinaryOpAssignEmitter, divassign)

}

// vim:sw=4:ts=4:cindent:noet
