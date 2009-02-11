//===========================================================================
/// \file llvm_expression.cc

#include <firtree/main.h>

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
/// \brief Class to emit a loop.
class LoopEmitter : public ExpressionEmitter
{
	public:
		LoopEmitter()
				: ExpressionEmitter() {
		}

		virtual ~LoopEmitter() {
		}

		/// Create an instance of this emitter.
		static ExpressionEmitter* Create() {
			return new LoopEmitter();
		}

		/// Emit the passed expression term returning a pointer to
		/// a ExpressionValue containing it's value. It is the
		/// responsibility of the caller to Release() this value.
		virtual ExpressionValue* Emit( LLVMContext* context,
		                               firtreeExpression expression ) {
			firtreeExpression init_expr = NULL;
			firtreeExpression pre_condition_expr = NULL;
			firtreeExpression post_condition_expr = NULL;
			firtreeExpression loop_body_expr = NULL;
			firtreeExpression loop_iterator_expr = NULL;

			if ( firtreeExpression_do( expression,
						&loop_body_expr, &post_condition_expr ) ) {
				/* good */
			} else if ( firtreeExpression_while( expression,
						&pre_condition_expr, &loop_body_expr ) ) {
				/* good */
			} else if ( firtreeExpression_for( expression,
						&init_expr, &pre_condition_expr, 
						&loop_iterator_expr, &loop_body_expr) ) {
				/* good */
			} else {
				/* bad */
				FIRTREE_LLVM_ICE( context, expression, "Invalid loop." );
			}

			assert((pre_condition_expr && !post_condition_expr) ||
					(!pre_condition_expr && post_condition_expr));

			ExpressionValue* init_value = NULL;
			ExpressionValue* pre_condition_value = NULL;
			ExpressionValue* post_condition_value = NULL;
			ExpressionValue* loop_body_value = NULL;
			ExpressionValue* loop_iterator_value = NULL;

			try {
				// If there is an initialisation expression, emit it.
				if(init_expr) {
					init_value = ExpressionEmitterRegistry::
						GetRegistry()->Emit( context, init_expr );
				}

				// Create a BB for the pre-condition.
				BasicBlock *pre_cond_BB = context->BB;
				if(pre_condition_expr) {
					pre_cond_BB = LLVM_CREATE( BasicBlock, "precond",
							context->Function );

					// Emit a straight branch into the pre-cond.
					LLVM_CREATE( BranchInst, pre_cond_BB, context->BB  );

					// Set the pre-cond block as the new insertion point.
					context->BB = pre_cond_BB;

					// Emit the pre-condition if present.
					if(pre_condition_expr) {
						pre_condition_value = ExpressionEmitterRegistry::
							GetRegistry()->Emit( context,
									pre_condition_expr );
					}
				}

				// Ensure that the BB which ends the pre-condition is
				// the one we remember.
				BasicBlock *pre_cond_start_BB = pre_cond_BB;
				pre_cond_BB = context->BB;

				// Create a BB for the loop body and set it as the 
				// insertion point.
				BasicBlock *body_BB = LLVM_CREATE( BasicBlock, "loopbody",
						context->Function );
				context->BB = body_BB;

				// Emit the loop body.
				loop_body_value = ExpressionEmitterRegistry::
						GetRegistry()->Emit( context, loop_body_expr );

				// If present, emit the iterator
				if(loop_iterator_expr) {
					loop_iterator_value = ExpressionEmitterRegistry::
						GetRegistry()->Emit( context, loop_iterator_expr );

				}

				// Ensure that the BB which ends the loop body is
				// the one we remember.
				BasicBlock *body_start_BB = body_BB;
				body_BB = context->BB;

				BasicBlock *post_cond_BB = context->BB;
				// Emit the post-condition if present.
				if(post_condition_expr) {
					post_condition_value = ExpressionEmitterRegistry::
						GetRegistry()->Emit( context, 
								post_condition_expr );
				}

				// Ensure that the BB which ends the post-condition is
				// the one we remember.

				// BasicBlock *post_cond_start_BB = post_cond_BB; // Unused.
				post_cond_BB = context->BB;

				// Create a BB for the continuation.
				BasicBlock *cont_BB = LLVM_CREATE( BasicBlock, "cont",
						context->Function );
				context->BB = cont_BB;

				// Now tidy up all the BBs we created.
				
				// The pre-condition block either ends with a conditional
				// branch into the loop body or into the continuation.
				if(pre_condition_value) {
					LLVM_CREATE( BranchInst, body_start_BB, cont_BB,
							pre_condition_value->GetLLVMValue(),
							pre_cond_BB);
				} else {
					LLVM_CREATE( BranchInst, body_start_BB, pre_cond_BB);
				}
	
				// The post-condition block either ends with a conditional
				// branch into the continuation or into the loop body.
				if(post_condition_value) {
					LLVM_CREATE( BranchInst, body_start_BB, cont_BB,
							post_condition_value->GetLLVMValue(),
							post_cond_BB);
				} else {
					LLVM_CREATE( BranchInst, pre_cond_start_BB, body_BB);
				}

				FIRTREE_SAFE_RELEASE( init_value );
				FIRTREE_SAFE_RELEASE( pre_condition_value );
				FIRTREE_SAFE_RELEASE( post_condition_value );
				FIRTREE_SAFE_RELEASE( loop_body_value );
				FIRTREE_SAFE_RELEASE( loop_iterator_value );

				return VoidExpressionValue::Create(context);
			} catch ( CompileErrorException e ) {
				FIRTREE_SAFE_RELEASE( init_value );
				FIRTREE_SAFE_RELEASE( pre_condition_value );
				FIRTREE_SAFE_RELEASE( post_condition_value );
				FIRTREE_SAFE_RELEASE( loop_body_value );
				FIRTREE_SAFE_RELEASE( loop_iterator_value );
				throw e;
			}

			return NULL;
		}
};

//===========================================================================
// Register the emitter.
FIRTREE_LLVM_DECLARE_EMITTER(LoopEmitter, for)
FIRTREE_LLVM_DECLARE_EMITTER(LoopEmitter, do)
FIRTREE_LLVM_DECLARE_EMITTER(LoopEmitter, while)

}

// vim:sw=4:ts=4:cindent:noet
