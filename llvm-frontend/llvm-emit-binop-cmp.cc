//===========================================================================
/// \file llvm-binop_cmp.cc

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
/// \brief Class to emit an comparison binary operator.
class BinaryOpCmpEmitter : ExpressionEmitter
{
	public:
		BinaryOpCmpEmitter()
				: ExpressionEmitter() {
		}

		virtual ~BinaryOpCmpEmitter() {
		}

		/// Create an instance of this emitter.
		static ExpressionEmitter* Create() {
			return new BinaryOpCmpEmitter();
		}

		/// Massage the values passed to have identical types using
		/// the type promotion rules for binary comparison operators.
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

			// Work out which comparison we want and set the predicate
			// appropriately. Note that each instruction (fcmp/icmp) has
			// different predicates for the same comparison.
			ICmpInst::Predicate icmp;
			FCmpInst::Predicate fcmp;

			if ( firtreeExpression_greater( expression, &left, &right ) ) {
				icmp = ICmpInst::ICMP_SGT;
				fcmp = FCmpInst::FCMP_OGT;
			} else if ( firtreeExpression_greaterequal( expression, &left, &right ) ) {
				icmp = ICmpInst::ICMP_SGE;
				fcmp = FCmpInst::FCMP_OGE;
			} else if ( firtreeExpression_less( expression, &left, &right ) ) {
				icmp = ICmpInst::ICMP_SLT;
				fcmp = FCmpInst::FCMP_OLT;
			} else if ( firtreeExpression_lessequal( expression, &left, &right ) ) {
				icmp = ICmpInst::ICMP_SLE;
				fcmp = FCmpInst::FCMP_OLE;
			} else if ( firtreeExpression_equal( expression, &left, &right ) ) {
				icmp = ICmpInst::ICMP_EQ;
				fcmp = FCmpInst::FCMP_OEQ;
			} else if ( firtreeExpression_notequal( expression, &left, &right ) ) {
				icmp = ICmpInst::ICMP_NE;
				fcmp = FCmpInst::FCMP_ONE;
			} else {
				FIRTREE_LLVM_ICE( context, expression,
				                  "Unknown comparison operator." );
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
					                    "comparison operator." );
				}


				llvm::Value* result_val = NULL;

				if ( left_val->GetType().Specifier == Firtree::TySpecFloat ) {
#if LLVM_AT_LEAST_2_6
					result_val = new FCmpInst( 
							*(context->BB),
							fcmp,
		  					left_val->GetLLVMValue(),
							right_val->GetLLVMValue(),
							"tmp");
#else
					result_val = new FCmpInst( 
							fcmp,
		  					left_val->GetLLVMValue(),
							right_val->GetLLVMValue(),
							"tmp",
							context->BB );
#endif
				} else if (( left_val->GetType().Specifier == Firtree::TySpecInt ) ||
				           ( left_val->GetType().Specifier == Firtree::TySpecBool ) ) {
#if LLVM_AT_LEAST_2_6
					result_val = new ICmpInst( *(context->BB), icmp,
					                          left_val->GetLLVMValue(),
					                          right_val->GetLLVMValue(),
					                          "tmp");
#else
					result_val = new ICmpInst( icmp,
					                          left_val->GetLLVMValue(),
					                          right_val->GetLLVMValue(),
					                          "tmp",
					                          context->BB );
#endif
				} else {
					FIRTREE_LLVM_ERROR( context, expression,
					                    "Incompatible types for binary "
					                    "comparison operator." );
				}

				return_val =  ConstantExpressionValue::
				              Create( context, result_val, TySpecBool );

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
FIRTREE_LLVM_DECLARE_EMITTER(BinaryOpCmpEmitter, equal)
FIRTREE_LLVM_DECLARE_EMITTER(BinaryOpCmpEmitter, notequal)
FIRTREE_LLVM_DECLARE_EMITTER(BinaryOpCmpEmitter, greater)
FIRTREE_LLVM_DECLARE_EMITTER(BinaryOpCmpEmitter, greaterequal)
FIRTREE_LLVM_DECLARE_EMITTER(BinaryOpCmpEmitter, less)
FIRTREE_LLVM_DECLARE_EMITTER(BinaryOpCmpEmitter, lessequal)

}

// vim:sw=4:ts=4:cindent:noet
