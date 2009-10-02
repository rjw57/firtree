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
class SelectionEmitter : public ExpressionEmitter
{
	public:
		SelectionEmitter()
				: ExpressionEmitter() {
		}

		virtual ~SelectionEmitter() {
		}

		/// Create an instance of this emitter.
		static ExpressionEmitter* Create() {
			return new SelectionEmitter();
		}

		/// Emit the passed expression term returning a pointer to
		/// a ExpressionValue containing it's value. It is the
		/// responsibility of the caller to Release() this value.
		virtual ExpressionValue* Emit( LLVMContext* context,
		                               firtreeExpression expression ) {
			firtreeExpression condition_expr;
			firtreeExpression then_expr;
			firtreeExpression else_expr;

			if ( !firtreeExpression_selection( expression,
						&condition_expr, &then_expr, &else_expr ) ) {
				FIRTREE_LLVM_ICE( context, expression, 
						"Invalid selection." );
			}

			ExpressionValue* condition_value = NULL;
			ExpressionValue* then_value = NULL;
			ExpressionValue* else_value = NULL;

			try {
				// Emit the code to calculate the condition value.
				condition_value = ExpressionEmitterRegistry::
					GetRegistry()->Emit( context, condition_expr );

				BasicBlock *then_BB = LLVM_CREATE(context,  BasicBlock, "then",
						context->Function );
				BasicBlock *else_BB = LLVM_CREATE(context,  BasicBlock, "else",
						context->Function );

				// Emit the conditional branch.
				LLVM_CREATE_NO_CONTEXT(BranchInst,
						then_BB, else_BB, 
						condition_value->GetLLVMValue(),
						context->BB );

				// Emit then block
				context->BB = then_BB;
				then_value = ExpressionEmitterRegistry::
					GetRegistry()->Emit( context, then_expr );
				// The 'then' block ends at the current insertion BB.
				// This may be different to then_BB if there were further
				// branches.
				then_BB = context->BB;

				// Emit else block
				context->BB = else_BB;
				else_value = ExpressionEmitterRegistry::
					GetRegistry()->Emit( context, else_expr );
				// The 'else' block ends at the current insertion BB.
				// This may be different to else_BB if there were further
				// branches.
				else_BB = context->BB;

				// Create a continuation block.
				BasicBlock *cont_BB = LLVM_CREATE(context,  BasicBlock, "cont",
						context->Function );

				// Terminate then and else block by branches to
				// continuation.
				LLVM_CREATE_NO_CONTEXT( BranchInst, cont_BB, then_BB );
				LLVM_CREATE_NO_CONTEXT( BranchInst, cont_BB, else_BB );

				// Make the continuation where future instructions
				// should be insterted.
				context->BB = cont_BB;

				FIRTREE_SAFE_RELEASE( condition_value );
				FIRTREE_SAFE_RELEASE( then_value );
				FIRTREE_SAFE_RELEASE( else_value );

				return VoidExpressionValue::Create(context);
			} catch ( CompileErrorException e ) {
				FIRTREE_SAFE_RELEASE( condition_value );
				FIRTREE_SAFE_RELEASE( then_value );
				FIRTREE_SAFE_RELEASE( else_value );
				throw e;
			}

			return NULL;
		}
};

//===========================================================================
// Register the emitter.
FIRTREE_LLVM_DECLARE_EMITTER(SelectionEmitter, selection)

}

// vim:sw=4:ts=4:cindent:noet
