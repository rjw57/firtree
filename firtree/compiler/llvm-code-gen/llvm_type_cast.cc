//===========================================================================
/// \file llvm_expression.cc

#include <firtree/main.h>

#include "llvm_frontend.h"
#include "llvm_private.h"
#include "llvm_type_cast.h"
#include "llvm_emit_constant.h"

#include <llvm/Instructions.h>

using namespace llvm;

namespace Firtree
{

//===========================================================================
/// Massage the values passed to have identical types using
/// the type promotion rules for binary arithmetic operators.
/// If the massaging fails, return false.
///
/// This method takes care of releasing left and right if their
/// values need to change.
bool TypeCaster::MassageBinOpTypes( LLVMContext* context,
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

//===========================================================================
/// Take an value and attempt to cast it to the type specified.
/// If the cast cannot be performed, raise a compile error at the
/// term specified.
///
/// It is the responsibility of the caller to Release() the
/// returned ExpressionValue.
ExpressionValue* TypeCaster::CastValue( LLVMContext* context,
                                        PT_Term term,
                                        const ExpressionValue* source,
                                        KernelTypeSpecifier dest_ty_spec )
{
	FullType source_type = source->GetType();
	llvm::Value* llvm_value = source->GetLLVMValue();

	// Trivial case where the cast is a nop. The cast does implicitly
	// make the result a constant however.
	if ( source_type.Specifier == dest_ty_spec ) {
		return ConstantExpressionValue::Create( context, llvm_value,
				source_type.IsStatic() );
	}

	switch ( dest_ty_spec ) {
		case Firtree::TySpecColor: 
			{
				if( source_type.Specifier == Firtree::TySpecVec4 )
				{
					return ConstantExpressionValue::Create( 
							context, llvm_value, source_type.IsStatic() );
				}
			}
			break;
		case Firtree::TySpecVec4: 
			{
				if( source_type.Specifier == Firtree::TySpecColor )
				{
					return ConstantExpressionValue::Create( 
							context, llvm_value, source_type.IsStatic() );
				}
			}
			break;
		case Firtree::TySpecFloat: {
			// We want a floating point value.
			switch ( source_type.Specifier ) {
				case Firtree::TySpecInt:
				case Firtree::TySpecBool: {
					// Ensure that the corresponding LLVM value is
					// indeed an integer
					FIRTREE_LLVM_ASSERT( context, term,
					                     isa<IntegerType>( llvm_value->getType() ) );

					// Add a conversion instruction
					llvm::Value* llvm_new_value = LLVM_NEW_2_3( SIToFPInst,
					                                     llvm_value,
					                                     Type::FloatTy,
					                                     "tmp",
					                                     context->BB );
					return ConstantExpressionValue::Create(
					           context, llvm_new_value,
							   source_type.IsStatic() );
				}
				break;
				default:
					// Do nothing, the call to FIRTREE_LLVM_ERROR at the
					// end of the function will report failure.
					break;
			};
		}
		break;

		case Firtree::TySpecInt:
		case Firtree::TySpecBool: {
			// We want an integer value.
			switch ( source_type.Specifier ) {
				case Firtree::TySpecFloat: {
					const Type* dest_llvm_type =
					    ( dest_ty_spec == Firtree::TySpecInt ) ?
					    Type::Int32Ty : Type::Int1Ty;
					llvm::Value* llvm_new_value = LLVM_NEW_2_3( FPToSIInst,
					                                     llvm_value,
					                                     dest_llvm_type,
					                                     "tmp",
					                                     context->BB );
					return ConstantExpressionValue::Create(
					           context, llvm_new_value, 
							   source_type.IsStatic() );
				}
				break;
				case Firtree::TySpecBool: {
					// Only support implicityle casting bool -> int since
					// this does not lose bits.
  	 			    llvm::Value* llvm_new_value = LLVM_NEW_2_3( SExtInst,
					                                     llvm_value,
					                                     Type::Int32Ty,
					                                     "tmp",
					                                     context->BB );
					return ConstantExpressionValue::Create(
					           context, llvm_new_value, 
							   source_type.IsStatic() );
				}
				default:
					// Do nothing, the call to FIRTREE_LLVM_ERROR at the
					// end of the function will report failure.
					break;
			}
		}
		break;

		default:
			// Do nothing, the call to FIRTREE_LLVM_ERROR below
			// will report failure.
			break;
	}

	FIRTREE_LLVM_ERROR( context, term, "Invalid type cast." );
	return NULL;
}

//===========================================================================
bool TypeCaster::CanImplicitlyCast(FullType source, FullType dest)
{
	// Enforce static correctness.
	if(dest.IsStatic() && !source.IsStatic()) {
		return false;
	}

	// Can always cast equivalent types.
	if(source.Specifier == dest.Specifier)
	{
		return true;
	}

	// Can cast color <-> vec4
	if((source.Specifier == Firtree::TySpecColor) &&
	  (dest.Specifier == Firtree::TySpecVec4))
	{
		return true;
	}
	if((source.Specifier == Firtree::TySpecVec4) &&
	  (dest.Specifier == Firtree::TySpecColor))
	{
		return true;
	}
	
	// Can cast float <-> int
	if((source.Specifier == Firtree::TySpecFloat) &&
	  (dest.Specifier == Firtree::TySpecInt))
	{
		return true;
	}
	if((source.Specifier == Firtree::TySpecInt) &&
	  (dest.Specifier == Firtree::TySpecFloat))
	{
		return true;
	}

	// Can cast bool -> int
	if((source.Specifier == Firtree::TySpecBool) &&
	  (dest.Specifier == Firtree::TySpecInt))
	{
		return true;
	}

	// By default, casting is not supported.
	return false;
}

}

// vim:sw=4:ts=4:cindent:noet
