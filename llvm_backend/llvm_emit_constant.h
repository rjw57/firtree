//===========================================================================
/// \file llvm_emit_constant.h Classes for outputting constant values.

#ifndef __LLVM_EMIT_CONSTANT_H
#define __LLVM_EMIT_CONSTANT_H

#include "llvm_expression.h"

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
		ConstantExpressionValue( LLVMContext* ctx, llvm::Value* val );
		virtual ~ConstantExpressionValue();

	public:
		static ExpressionValue* Create( LLVMContext* ctx, llvm::Value* val );

		virtual llvm::Value*	GetLLVMValue() const;

		virtual FullType		GetType() const;

	private:
		llvm::Value*			m_WrappedValue;
};

} // namespace Firtree

#endif // __LLVM_EMIT_CONSTANT_H 

// vim:sw=4:ts=4:cindent:noet
