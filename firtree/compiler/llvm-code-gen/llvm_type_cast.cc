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
