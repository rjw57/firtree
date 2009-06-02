//===========================================================================
/// \file llvm-emit-constant.h Classes for outputting constant values.

#ifndef __LLVM_EMIT_CONSTANT_H
#define __LLVM_EMIT_CONSTANT_H

#include "llvm-expression.h"

namespace Firtree
{

//===========================================================================
/// \brief A constant value.
///
/// Descended from VoidExpressionValue since, like a void value, it is
/// not mutable.
class ConstantExpressionValue : public VoidExpressionValue
{
	protected:
		ConstantExpressionValue( LLVMContext* ctx, llvm::Value* val,
				bool is_static );
		virtual ~ConstantExpressionValue();

	public:
		static ExpressionValue* Create( LLVMContext* ctx, llvm::Value* val,
				bool is_static = false);

		virtual llvm::Value*	GetLLVMValue() const;

		virtual FullType		GetType() const;

	private:
		llvm::Value*			m_WrappedValue;
		bool					m_IsStatic;
};

//===========================================================================
/// Utilitiy function to create a floating point immediate value.
ExpressionValue* CreateFloat( LLVMContext* context, float float_val );

//===========================================================================
/// Utilitiy function to create an integer immediate value.
ExpressionValue* CreateInt( LLVMContext* context, int int_val );

//===========================================================================
/// Utilitiy function to create a boolean immediate value.
ExpressionValue* CreateBool( LLVMContext* context, bool bool_val );

//===========================================================================
/// Utilitiy function to create a vector immediate value.
ExpressionValue* CreateVector( LLVMContext* context,
                               const float* params, int param_count );
ExpressionValue* CreateVector( LLVMContext* context,
                               float x, float y );
ExpressionValue* CreateVector( LLVMContext* context,
                               float x, float y, float z );
ExpressionValue* CreateVector( LLVMContext* context,
                               float x, float y, float z, float w );

//===========================================================================
/// Utility function to create a vector from a set of ExpressionValues.
ExpressionValue* CreateVector( LLVMContext* context,
                               std::vector<ExpressionValue*> values,
							   bool is_static = false);

//===========================================================================
/// Utility function to extend a vector of ExpressionValues by cracking a
/// vector.
void CrackVector( LLVMContext* context, ExpressionValue* vector,
                  std::vector<ExpressionValue*>& output_values );


} // namespace Firtree

#endif // __LLVM_EMIT_CONSTANT_H 

// vim:sw=4:ts=4:cindent:noet
