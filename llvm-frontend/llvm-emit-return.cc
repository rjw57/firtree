//===========================================================================
/// \file llvm-expression.cc

#define __STDC_CONSTANT_MACROS



#include "llvm-frontend.h"
#include "llvm-private.h"
#include "llvm-emit-decl.h"
#include "llvm-expression.h"
#include "llvm-type-cast.h"

#include <llvm/Instructions.h>

using namespace llvm;

namespace Firtree
{

//===========================================================================
/// \brief Class to emit a return instruction.
class ReturnEmitter : public ExpressionEmitter
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
			if ( !firtreeExpression_return( expression,
			                                &return_value_expr ) ) {
				FIRTREE_LLVM_ICE( context, expression, "Invalid return." );
			}

			// Emit the code to calculate the return value.
			ExpressionValue* return_value =
			    ExpressionEmitterRegistry::GetRegistry()->Emit(
			        context, return_value_expr );
			ExpressionValue* cast_return_value = NULL;

			try {
				// Cast the return value if necessary.
				if(return_value->GetType().Specifier != Firtree::TySpecVoid) {
					cast_return_value =
						TypeCaster::CastValue( context, return_value_expr,
								return_value,
					 			context->CurrentPrototype->
		  						ReturnType.Specifier );
					FIRTREE_SAFE_RELEASE( return_value );
				} else {
					cast_return_value = return_value;
					return_value = NULL;
				}

				// Create the return statement.
				llvm::Value* llvm_ret_val = NULL;
				if ( cast_return_value->GetType().Specifier !=
				        Firtree::TySpecVoid ) {
					llvm_ret_val = cast_return_value->GetLLVMValue();
				}
				LLVM_CREATE(context, ReturnInst, llvm_ret_val, context->BB );

				// Create a basic block to follow (in case we have
				// some unreachable code below).
				context->BB = LLVM_CREATE(context,  BasicBlock, "cont",
						context->Function);

				// Ensure static correctness.
				if(context->CurrentPrototype->ReturnType.IsStatic())
				{
					if(!(cast_return_value->GetType().IsStatic())) {
						FIRTREE_LLVM_ERROR( context, expression,
								"Return value must be static." );
					}
				}

				return cast_return_value;
			} catch ( CompileErrorException e ) {
				FIRTREE_SAFE_RELEASE( return_value );
				FIRTREE_SAFE_RELEASE( cast_return_value );
				throw e;
			}

			return NULL;
		}
};

//===========================================================================
// Register the emitter.
FIRTREE_LLVM_DECLARE_EMITTER(ReturnEmitter, return)

}

// vim:sw=4:ts=4:cindent:noet
