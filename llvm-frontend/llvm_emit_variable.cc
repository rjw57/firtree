//===========================================================================
/// \file llvm_expression.cc

#define __STDC_CONSTANT_MACROS



#include "llvm_frontend.h"
#include "llvm_private.h"
#include "llvm_emit_decl.h"
#include "llvm_expression.h"
#include "llvm_type_cast.h"

#include <llvm/Instructions.h>

using namespace llvm;

namespace Firtree
{

//===========================================================================
/// \brief ExpressionValue sub-class which encapsulates a reference to a
/// variable.
class VariableExpressionValue : public ExpressionValue
{
	protected:
		VariableExpressionValue( LLVMContext* ctx,
		                         const VariableDeclaration* decl )
				: ExpressionValue()
				, m_VarDeclaration( decl )
				, m_Context( ctx ) {
			// Get the value at time of creation.
			m_VarValue = new LoadInst( decl->value,
			                          "tmp", m_Context->BB );
		}
		virtual ~VariableExpressionValue() { }

	public:
		static ExpressionValue* Create( LLVMContext* ctx,
		                                const VariableDeclaration* decl ) {
			return new VariableExpressionValue( ctx, decl );
		}

		/// Return the LLVM value associated with this value.
		virtual llvm::Value*	GetLLVMValue() const {
			if ( !m_VarDeclaration->initialised ) {
				// FIXME: It is sub-optimal that the location of this
				// error can not be reported.
				FIRTREE_LLVM_ERROR( m_Context, NULL,
				                    "Use of uninitialised variable." );
			}
			return m_VarValue;
		}

		/// Return the Firtree type associated with this value.
		virtual FullType		GetType() const {
			return m_VarDeclaration->type;
		}

		/// Return a flag which is true iff this value is a lvalue.
		virtual bool			IsMutable() const {
			return (!m_VarDeclaration->type.IsConst()) && 
				(!m_VarDeclaration->type.IsStatic());
		}

		/// Assign the value from the passed ExpressionValue.
		virtual void	AssignFrom( const ExpressionValue& val ) const {
			if ( !IsMutable() ) {
				FIRTREE_LLVM_ICE( m_Context, NULL, "Attempt to "
				                  "assign immutable variable." );
			}

			// A bit naughty... we break const correctness here
			const_cast<VariableDeclaration*>(
			    m_VarDeclaration )->initialised = true;

			// Store the value.
			new StoreInst( val.GetLLVMValue(),
			             m_VarDeclaration->value,
			             m_Context->BB );
		}

	private:
		llvm::Value*				m_VarValue;
		const VariableDeclaration*	m_VarDeclaration;
		LLVMContext*				m_Context;
};

//===========================================================================
/// \brief Class to emit a reference to a variable.
class VariableEmitter : ExpressionEmitter
{
	public:
		VariableEmitter()
				: ExpressionEmitter() {
		}

		virtual ~VariableEmitter() {
		}

		/// Create an instance of this emitter.
		static ExpressionEmitter* Create() {
			return new VariableEmitter();
		}

		/// Emit the passed expression term returning a pointer to
		/// a ExpressionValue containing it's value. It is the
		/// responsibility of the caller to Release() this value.
		virtual ExpressionValue* Emit( LLVMContext* context,
		                               firtreeExpression expression ) {
			GLS_Tok var_ident;
			if ( !firtreeExpression_variablenamed( expression, &var_ident ) ) {
				FIRTREE_LLVM_ICE( context, expression,
				                  "Invalid variable reference." );
			}

			const VariableDeclaration* var_decl = context->Variables->
			                                      LookupSymbol( GLS_Tok_symbol( var_ident ) );
			if ( var_decl == NULL ) {
				FIRTREE_LLVM_ERROR( context, expression, "No such "
				                    "variable '%s'.",
				                    GLS_Tok_string( var_ident ) );
			}

			return VariableExpressionValue::Create( context, var_decl );
		}
};

//===========================================================================
// Register the emitter.
FIRTREE_LLVM_DECLARE_EMITTER(VariableEmitter, variablenamed)

}

// vim:sw=4:ts=4:cindent:noet
