//===========================================================================
/// \file llvm-emit-negate.cc

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
/// \brief Class to emit a negation instruction.
class NegateEmitter : ExpressionEmitter
{
	public:
		NegateEmitter()
				: ExpressionEmitter() {
		}

		virtual ~NegateEmitter() {
		}

		/// Create an instance of this emitter.
		static ExpressionEmitter* Create() {
			return new NegateEmitter();
		}

		/// Emit the passed expression term returning a pointer to
		/// a ExpressionValue containing it's value. It is the
		/// responsibility of the caller to Release() this value.
		virtual ExpressionValue* Emit( LLVMContext* context,
		                               firtreeExpression expression ) {
			firtreeExpression negate_value_expr;
			if ( !firtreeExpression_negate( expression,
			                                &negate_value_expr ) ) {
				FIRTREE_LLVM_ICE( context, expression, "Invalid unary -." );
			}

			// Emit the code to calculate the value to negate.
			ExpressionValue* negate_value =
			    ExpressionEmitterRegistry::GetRegistry()->Emit(
			        context, negate_value_expr );
			ExpressionValue* return_value = NULL;

			try {
				// Depending on the type of the negation value, form
				// an appropriate LLVM value for '-1'.
				llvm::Value* negative_one;
				FullType req_type = negate_value->GetType();
				if ( req_type.IsVector() ) {
					std::vector<Constant*> neg_ones;
					for ( unsigned int i=0; i<req_type.GetArity(); ++i ) {
#if LLVM_AT_LEAST_2_3
						neg_ones.push_back( ConstantFP::
						                    get( Type::FloatTy, -1.0 ) );
#else
						neg_ones.push_back( ConstantFP::
						                    get( Type::FloatTy,
						                         APFloat( -1.f ) ) );
#endif
					}
					negative_one = ConstantVector::get( neg_ones );
				} else if ( req_type.IsScalar() ) {
					switch ( req_type.Specifier ) {
						case Firtree::TySpecFloat:
#if LLVM_AT_LEAST_2_3
							negative_one = ConstantFP::
							               get( Type::FloatTy, -1.0 );
#else
							negative_one = ConstantFP::
							               get( Type::FloatTy,
							                    APFloat( -1.f ) );
#endif
							break;
						case Firtree::TySpecInt:
							negative_one = ConstantInt::
							               get( Type::Int32Ty, -1 );
							break;
						case Firtree::TySpecBool:
							negative_one = ConstantInt::
							               get( Type::Int1Ty, -1 );
							break;
						default:
							FIRTREE_LLVM_ERROR( context, negate_value_expr,
							                    "Expression is of invalid "
							                    "type for unary '-'." );
							break;
					}
				} else {
					FIRTREE_LLVM_ERROR( context, negate_value_expr,
					                    "Expression is of invalid type for "
					                    "unary '-'." );
				}

				llvm::Value* new_val = BinaryOperator::Create(
				                           Instruction::Mul,
				                           negative_one,
				                           negate_value->GetLLVMValue(),
				                           "tmp", context->BB );
				return_value = ConstantExpressionValue::
				               Create( context, new_val );

				FIRTREE_SAFE_RELEASE( negate_value );
			} catch ( CompileErrorException e ) {
				FIRTREE_SAFE_RELEASE( negate_value );
				FIRTREE_SAFE_RELEASE( return_value );
				throw e;
			}

			return return_value;
		}
};

//===========================================================================
// Register the emitter.
FIRTREE_LLVM_DECLARE_EMITTER(NegateEmitter, negate)

}

// vim:sw=4:ts=4:cindent:noet
