//===========================================================================
/// \file llvm_type_cast.h Classes for type casting

#ifndef __LLVM_TYPE_CAST_H
#define __LLVM_TYPE_CAST_H

#include "llvm_expression.h"

namespace Firtree
{

//===========================================================================
/// \brief Utility class to perform type casting.
///
/// This class defined various static methods for casting ExpressionValue-s
/// from one type to another.
class TypeCaster
{
	public:
		/// Take an value and attempt to cast it to the type specified.
		/// If the cast cannot be performed, raise a compile error at the
		/// term specified.
		///
		/// It is the responsibility of the caller to Release() the
		/// returned ExpressionValue.
		static ExpressionValue* CastValue( LLVMContext* context,
		                                   PT_Term term,
		                                   const ExpressionValue* source,
		                                   KernelTypeSpecifier type );

		/// Return true if the FullType source can be implicitly cast to
		/// dest.
		static bool CanImplicitlyCast(FullType source, FullType dest);
	
		/// Massage the values passed to have identical types using
		/// the type promotion rules for binary arithmetic operators.
		/// If the massaging fails, return false.
		///
		/// This method takes care of releasing left and right if their
		/// values need to change.
		static bool MassageBinOpTypes( LLVMContext* context,
								firtreeExpression binopexpr,
								ExpressionValue* left_in,
								ExpressionValue* right_in,
								ExpressionValue** left_out,
								ExpressionValue** right_out );
};

} // namespace Firtree

#endif // __LLVM_TYPE_CAST_H 

// vim:sw=4:ts=4:cindent:noet
