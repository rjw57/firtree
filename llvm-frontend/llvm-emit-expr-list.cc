//===========================================================================
/// \file llvm-emit-expr_list.cc

#define __STDC_CONSTANT_MACROS



#include "llvm-frontend.h"
#include "llvm-private.h"
#include "llvm-expression.h"

#include <llvm/Instructions.h>

using namespace llvm;

namespace Firtree
{

//===========================================================================
/// \brief Class to emit a list of expressions.
class ExprListEmitter : ExpressionEmitter
{
	public:
		ExprListEmitter()
				: ExpressionEmitter() {
		}

		virtual ~ExprListEmitter() {
		}

		/// Create an instance of this emitter.
		static ExpressionEmitter* Create() {
			return new ExprListEmitter();
		}

		/// Emit the passed expression term returning a pointer to
		/// a ExpressionValue containing it's value. It is the
		/// responsibility of the caller to Release() this value.
		virtual ExpressionValue* Emit( LLVMContext* context,
		                               firtreeExpression expression ) {
			bool need_new_scope = false;
			firtreeExpression head = NULL;
			GLS_Lst( firtreeExpression ) tail;
			if ( firtreeExpression_expression( expression, &head, &tail ) ) {
				need_new_scope = false;
			} else if ( firtreeExpression_compound( expression, &tail ) ) {
				need_new_scope = true;
			} else {
				FIRTREE_LLVM_ICE( context, expression,
				                  "Invalid expression list/compound." );
			}

			if ( need_new_scope ) {
				context->Variables->PushScope();
			}

			// Emit head expression (if present).
			ExpressionValue* expression_value = NULL;

			if ( head != NULL ) {
				expression_value =
				    ExpressionEmitterRegistry::GetRegistry()->Emit(
				        context, head );
			}

			// Emit remaining expressions.
			GLS_Lst( firtreeExpression ) it;
			GLS_FORALL( it, tail ) {
				firtreeExpression e = GLS_FIRST( firtreeExpression, it );

				FIRTREE_SAFE_RELEASE( expression_value );
				expression_value = ExpressionEmitterRegistry::
				                   GetRegistry()->Emit( context, e );
			}

			if ( need_new_scope ) {
				context->Variables->PopScope();
			}

			return expression_value;
		}
};

//===========================================================================
// Register the emitter.
FIRTREE_LLVM_DECLARE_EMITTER(ExprListEmitter, expression)
FIRTREE_LLVM_DECLARE_EMITTER(ExprListEmitter, compound)

}

// vim:sw=4:ts=4:cindent:noet
