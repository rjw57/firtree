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
                                        FullType::TypeSpecifier dest_ty_spec )
{
	FullType source_type = source->GetType();
	Value* llvm_value = source->GetLLVMValue();

	// Trivial case where the cast is a nop. The cast does implicitly
	// make the result a constant however.
	if ( source_type.Specifier == dest_ty_spec ) {
		return ConstantExpressionValue::Create( context, llvm_value );
	}

	switch ( dest_ty_spec ) {
		case FullType::TySpecFloat: {
			// We want a floating point value.
			switch ( source_type.Specifier ) {
				case FullType::TySpecInt:
				case FullType::TySpecBool: {
					// Ensure that the corresponding LLVM value is
					// indeed an integer
					FIRTREE_LLVM_ASSERT( context, term,
					                     isa<IntegerType>( llvm_value->getType() ) );

					// Add a conversion instruction
					Value* llvm_new_value = LLVM_CREATE( SIToFPInst,
					                                     llvm_value,
					                                     Type::FloatTy,
					                                     "tmpcast",
					                                     context->BB );
					return ConstantExpressionValue::Create(
					           context, llvm_new_value );
				}
				break;
				default:
					// Do nothing, the call to FIRTREE_LLVM_ERROR at the
					// end of the function will report failure.
					break;
			};
		}
		break;

		case FullType::TySpecInt:
		case FullType::TySpecBool: {
			// We want an integer value.
			switch ( source_type.Specifier ) {
				case FullType::TySpecFloat: {
					const Type* dest_llvm_type =
					    ( dest_ty_spec == FullType::TySpecInt ) ?
					    Type::Int32Ty : Type::Int1Ty;
					Value* llvm_new_value = LLVM_CREATE( FPToSIInst,
					                                     llvm_value,
					                                     dest_llvm_type,
					                                     "tmpcast",
					                                     context->BB );
					return ConstantExpressionValue::Create(
					           context, llvm_new_value );
				}
				break;
				case FullType::TySpecBool: {
					// Only support implicityle casting bool -> int since
					// this does not lose bits.
					Value* llvm_new_value = LLVM_CREATE( SExtInst,
					                                     llvm_value,
					                                     Type::Int32Ty,
					                                     "tmpcast",
					                                     context->BB );
					return ConstantExpressionValue::Create(
					           context, llvm_new_value );
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
	// Can always cast equivalent types.
	if(source.Specifier == dest.Specifier)
	{
		return true;
	}

	// Can cast color <-> vec4
	if((source.Specifier == FullType::TySpecColor) &&
	  (dest.Specifier == FullType::TySpecVec4))
	{
		return true;
	}
	if((source.Specifier == FullType::TySpecVec4) &&
	  (dest.Specifier == FullType::TySpecColor))
	{
		return true;
	}
	
	// Can cast float <-> int
	if((source.Specifier == FullType::TySpecFloat) &&
	  (dest.Specifier == FullType::TySpecInt))
	{
		return true;
	}
	if((source.Specifier == FullType::TySpecInt) &&
	  (dest.Specifier == FullType::TySpecFloat))
	{
		return true;
	}

	// Can cast bool -> int
	if((source.Specifier == FullType::TySpecBool) &&
	  (dest.Specifier == FullType::TySpecInt))
	{
		return true;
	}

	// By default, casting is not supported.
	return false;
}

}

// vim:sw=4:ts=4:cindent:noet
