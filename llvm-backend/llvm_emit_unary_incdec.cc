//===========================================================================
/// \file llvm_emit_unary_incdec.cc

#include <firtree/main.h>

#include "llvm_backend.h"
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
/// \brief Class to emit a negation instruction.
class IncDecEmitter : ExpressionEmitter
{
	public:
		IncDecEmitter()
				: ExpressionEmitter() {
		}

		virtual ~IncDecEmitter() {
		}

		/// Create an instance of this emitter.
		static ExpressionEmitter* Create() {
			return new IncDecEmitter();
		}

		/// Emit the passed expression term returning a pointer to
		/// a ExpressionValue containing it's value. It is the
		/// responsibility of the caller to Release() this value.
		virtual ExpressionValue* Emit( LLVMContext* context,
		                               firtreeExpression expression ) {
			// Flags to indicate if the operation is an increment
			// or decrement and whether is is a pre- or post-op.
			bool is_inc, is_postop;

			firtreeExpression operand;

			if ( firtreeExpression_inc( expression, &operand ) ) {
				is_inc = true;
				is_postop = false;
			} else if ( firtreeExpression_dec( expression, &operand ) ) {
				is_inc = false;
				is_postop = false;
			} else if ( firtreeExpression_postinc( expression, &operand ) ) {
				is_inc = true;
				is_postop = true;
			} else if ( firtreeExpression_postdec( expression, &operand ) ) {
				is_inc = false;
				is_postop = true;
			} else {
				FIRTREE_LLVM_ICE( context, expression,
				                  "Invalid unary {inc,dec}rement." );
			}

			// Emit the code to calculate the value to increment/decrement.
			ExpressionValue* operand_value =
			    ExpressionEmitterRegistry::GetRegistry()->Emit(
			        context, operand );
			ExpressionValue* new_value = NULL;
			ExpressionValue* return_value = NULL;

			try {
				// Depending on the type of the negation value, form
				// an appropriate LLVM value for '1'.
				llvm::Value* one;
				FullType req_type = operand_value->GetType();
				if ( req_type.IsScalar() ) {
					switch ( req_type.Specifier ) {
						case FullType::TySpecFloat:
							one = ConstantFP::
							      get( Type::FloatTy,
							           APFloat( 1.f ) );
							break;
						case FullType::TySpecInt:
							one = ConstantInt::
							      get( Type::Int32Ty, 1 );
							break;
						case FullType::TySpecBool:
							one = ConstantInt::
							      get( Type::Int1Ty, 1 );
							break;
						default:
							FIRTREE_LLVM_ERROR( context, operand,
							                    "Expression is of invalid "
							                    "type for unary operator." );
							break;
					}
				} else {
					// Vector {inc,dec}rement not supported.
					FIRTREE_LLVM_ERROR( context, operand,
					                    "Expression is of invalid type for "
					                    "unary operator." );
				}

				Instruction::BinaryOps llvm_op =
				    is_inc ? Instruction::Add : Instruction::Sub;
				llvm::Value* new_val = BinaryOperator::create(
				                           llvm_op,
				                           operand_value->GetLLVMValue(),
				                           one,
				                           "tmpincdec", context->BB );

				new_value = ConstantExpressionValue::
				            Create( context, new_val );

				if ( !( operand_value->IsMutable() ) ) {
					FIRTREE_LLVM_ERROR( context, operand,
					                    "Operand to unary operator is not "
					                    "mutable." );
				}

				if ( is_postop ) {
					return_value = operand_value;
				} else {
					return_value = new_value;
				}

				FIRTREE_SAFE_RETAIN( return_value );
				operand_value->AssignFrom( *new_value );

				FIRTREE_SAFE_RELEASE( operand_value );
				FIRTREE_SAFE_RELEASE( new_value );
			} catch ( CompileErrorException e ) {
				FIRTREE_SAFE_RELEASE( operand_value );
				FIRTREE_SAFE_RELEASE( return_value );
				FIRTREE_SAFE_RELEASE( new_value );
				throw e;
			}

			return return_value;
		}
};

//===========================================================================
// Register the emitter.
RegisterEmitter<IncDecEmitter> g_IncEmitterReg( "inc" );
RegisterEmitter<IncDecEmitter> g_DecEmitterReg( "dec" );
RegisterEmitter<IncDecEmitter> g_PostIncEmitterReg( "postinc" );
RegisterEmitter<IncDecEmitter> g_PostDecEmitterReg( "postdec" );

}

// vim:sw=4:ts=4:cindent:noet
