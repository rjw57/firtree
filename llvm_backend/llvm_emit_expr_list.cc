//===========================================================================
/// \file llvm_emit_expr_list.cc

#include <firtree/main.h>

#include "llvm_backend.h"
#include "llvm_private.h"
#include "llvm_expression.h"

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
			firtreeExpression head;
			GLS_Lst( firtreeExpression ) tail;
			if ( !firtreeExpression_expression( expression, &head, &tail ) ) {
				FIRTREE_LLVM_ICE( context, expression,
				                  "Invalid expression list." );
			}

			// Emit head expression.
			ExpressionValue* expression_value =
			    ExpressionEmitterRegistry::GetRegistry()->Emit(
			        context, head );

			// Emit remaining expressions.
			GLS_Lst( firtreeExpression ) it;
			GLS_FORALL( it, tail ) {
				firtreeExpression e = GLS_FIRST( firtreeExpression, it );

				FIRTREE_SAFE_RELEASE( expression_value );
				expression_value = ExpressionEmitterRegistry::
				                   GetRegistry()->Emit( context, e );
			}

			return expression_value;
		}
};

//===========================================================================
// Register the emitter.
RegisterEmitter<ExprListEmitter> g_ExprListEmitterReg( "expression" );

}

// vim:sw=4:ts=4:cindent:noet
