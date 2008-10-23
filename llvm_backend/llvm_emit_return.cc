//===========================================================================
/// \file llvm_expression.cc

#include <firtree/main.h>

#include "llvm_backend.h"
#include "llvm_private.h"
#include "llvm_emit_decl.h"
#include "llvm_expression.h"

#include <llvm/Instructions.h>

using namespace llvm;

namespace Firtree
{

//===========================================================================
/// \brief Class to emit a return instruction.
class ReturnEmitter : ExpressionEmitter
{
	public:
		ReturnEmitter()
				: ExpressionEmitter() {
		}

		virtual ~ReturnEmitter() {
		}

		/// Create an instance of this emitter.
		static ExpressionEmitter* Create() {
			return new ReturnEmitter();
		}

		/// Emit the passed expression term returning a pointer to
		/// a ExpressionValue containing it's value. It is the
		/// responsibility of the caller to Release() this value.
		virtual ExpressionValue* Emit( LLVMContext* context,
		                               firtreeExpression expression ) {
			firtreeExpression return_value_expr;
			if ( !firtreeExpression_return( expression, &return_value_expr ) ) {
				FIRTREE_LLVM_ICE( context, expression, "Invalid return." );
			}

			// Emit the code to calculate the return value.
			ExpressionValue* return_value =
			    ExpressionEmitterRegistry::GetRegistry()->Emit(
			        context, return_value_expr );

			// Check if the return type matches the function
			if ( ! return_value->GetType().IsConstCastableTo(
			            context->CurrentPrototype->ReturnType ) ) {
				FIRTREE_SAFE_RELEASE( return_value );
				FIRTREE_LLVM_ERROR( context, expression,
				                    "Return value does not match "
				                    "expected return type of function." );
				return NULL;
			}

			// Create the return statement.
			llvm::Value* llvm_ret_val =
			    ( return_value->GetType().Specifier == FullType::TySpecVoid ) ?
			    NULL : return_value->GetLLVMValue();
			LLVM_CREATE( ReturnInst, llvm_ret_val, context->BB );

			return return_value;
		}
};

//===========================================================================
// Register the emitter.
RegisterEmitter<ReturnEmitter> g_ReturnEmitterReg( "return" );

}

// vim:sw=4:ts=4:cindent:noet
