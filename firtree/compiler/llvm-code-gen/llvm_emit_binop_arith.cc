//===========================================================================
/// \file llvm_binop_arith.cc

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

		/// Massage the values passed to have identical types using
		/// the type promotion rules for binary arithmetic operators.
		/// If the massaging fails, return false.
		///
		/// This method takes care of releasing left and right if their
		/// values need to change.
		bool MassageBinOpTypes( LLVMContext* context,
		                        firtreeExpression binopexpr,
		                        ExpressionValue* left_in,
		                        ExpressionValue* right_in,
		                        ExpressionValue** left_out,
		                        ExpressionValue** right_out ) {
			// By default, nothing need be done.
			*left_out = left_in;
			*right_out = right_in;

			FullType left_in_type = left_in->GetType();
			FullType right_in_type = right_in->GetType();

			// If both sides are vectors, they must have the same
			// -arity.
			if ( left_in_type.IsVector() && right_in_type.IsVector() ) {
				if ( left_in_type.GetArity() == right_in_type.GetArity() ) {
					// Everything is hunky-dory.
					return true;
				} else {
					// This cannot be massaged.
					return false;
				}
			}

			if ( left_in_type.IsScalar() && right_in_type.IsScalar() ) {
				// If both sides are scalars, short cut the 'nothing needs
				// doing' option.
				if ( left_in_type.Specifier == right_in_type.Specifier ) {
					return true;
				}

				// If both sides are scalars then decide on the
				// greatest common denominator type
				KernelTypeSpecifier gcd_type;

				if (( left_in_type.Specifier == Firtree::TySpecFloat ) ||
				        ( right_in_type.Specifier == Firtree::TySpecFloat ) ) {
					gcd_type = Firtree::TySpecFloat;
				} else if (( left_in_type.Specifier == Firtree::TySpecInt ) ||
				           ( right_in_type.Specifier == Firtree::TySpecInt ) ) {
					gcd_type = Firtree::TySpecInt;
				} else {
					gcd_type = Firtree::TySpecBool;
				}

				ExpressionValue* left_cast = NULL;
				ExpressionValue* right_cast = NULL;
				try {
					// Attempt to cast both sides.
					left_cast = TypeCaster::CastValue( context,
					                                   binopexpr,
					                                   left_in, gcd_type );
					right_cast = TypeCaster::CastValue( context,
					                                    binopexpr,
					                                    right_in, gcd_type );

					*left_out = left_cast;
					FIRTREE_SAFE_RELEASE( left_in );
					*right_out = right_cast;
					FIRTREE_SAFE_RELEASE( right_in );

					return true;
				} catch ( CompileErrorException e ) {
					FIRTREE_SAFE_RELEASE( left_cast );
					FIRTREE_SAFE_RELEASE( right_cast );
					throw e;
				}

				return false;
			}

			// Now we wish to handle the case where one side is a scalar
			// which needs to be 'auto broadcast' into a vector.
			if ( left_in_type.IsScalar() && right_in_type.IsVector() ) {
				ExpressionValue* left_cast;
				ExpressionValue* left_vec;

				try {
					left_cast = TypeCaster::CastValue( context,
					                                   binopexpr,
					                                   left_in,
					                                   Firtree::TySpecFloat );

					std::vector<ExpressionValue*> vec_elements;
					for ( unsigned int i=0; i<right_in_type.GetArity(); ++i ) {
						vec_elements.push_back( left_cast );
					}

					left_vec = CreateVector( context, vec_elements );

					FIRTREE_SAFE_RELEASE( left_cast );
					FIRTREE_SAFE_RELEASE( left_in );
					*left_out = left_vec;

					return true;
				} catch ( CompileErrorException e ) {
					FIRTREE_SAFE_RELEASE( left_cast );
					FIRTREE_SAFE_RELEASE( left_vec );
					throw e;
				}

				return false;
			}

			if ( right_in_type.IsScalar() && left_in_type.IsVector() ) {
				ExpressionValue* right_cast;
				ExpressionValue* right_vec;

				try {
					right_cast = TypeCaster::CastValue( context,
					                                    binopexpr,
					                                    right_in,
					                                    Firtree::TySpecFloat );

					std::vector<ExpressionValue*> vec_elements;
					for ( unsigned int i=0; i<left_in_type.GetArity(); ++i ) {
						vec_elements.push_back( right_cast );
					}

					right_vec = CreateVector( context, vec_elements );

					FIRTREE_SAFE_RELEASE( right_cast );
					FIRTREE_SAFE_RELEASE( right_in );
					*right_out = right_vec;

					return true;
				} catch ( CompileErrorException e ) {
					FIRTREE_SAFE_RELEASE( right_cast );
					FIRTREE_SAFE_RELEASE( right_vec );
					throw e;
				}

				return false;
			}

			// If we get here, no massaging will help.
			return false;
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

				if ( ! MassageBinOpTypes( context, expression,
				                          left_val, right_val,
				                          &left_val, &right_val ) ) {
					FIRTREE_LLVM_ERROR( context, expression,
					                    "Incompatible types for binary "
					                    "operator." );
				}

				llvm::Value* result_val = BinaryOperator::create( op,
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
