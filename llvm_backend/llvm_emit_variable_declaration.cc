//===========================================================================
/// \file llvm_expression.cc

#include <firtree/main.h>

#include "llvm_backend.h"
#include "llvm_private.h"
#include "llvm_emit_decl.h"
#include "llvm_expression.h"
#include "llvm_type_cast.h"

#include <llvm/Instructions.h>

using namespace llvm;

namespace Firtree
{

//===========================================================================
/// \brief Class to emit a return instruction.
class VariableDeclarationEmitter : ExpressionEmitter
{
	public:
		VariableDeclarationEmitter()
				: ExpressionEmitter() {
		}

		virtual ~VariableDeclarationEmitter() {
		}

		/// Create an instance of this emitter.
		static ExpressionEmitter* Create() {
			return new VariableDeclarationEmitter();
		}

		/// Emit a variable declaration of type 'var_type' with
		/// the SingleDeclaration 'var_decl'.
		void EmitSingleDeclaration( LLVMContext* context,
		                            FullType var_type,
		                            firtreeSingleDeclaration var_decl ) {
			GLS_Tok identifier_tok;
			firtreeExpression initialiser;
			if ( !firtreeSingleDeclaration_variableinitializer(
			            var_decl, &identifier_tok,
			            &initialiser ) ) {
				FIRTREE_LLVM_ICE( context, var_decl,
				                  "Invalid variable declaration." );
			}

			// Register the variable in the symbol table (checking
			// for conflicts).
			SymbolTable* sym_table = context->Variables;
			const VariableDeclaration* existing_decl =
			    sym_table->LookupSymbol( GLS_Tok_symbol( identifier_tok ) );
			if ( existing_decl != NULL ) {
				FIRTREE_LLVM_ERROR( context, var_decl,
				                    "Declaration for variable '%s' "
				                    "conflicts with prior declaration.",
				                    GLS_Tok_string( identifier_tok ) );
			}

			// Create a VariableDeclaration structure for this
			// variable.
			VariableDeclaration var_decl_s;
			var_decl_s.value = LLVM_CREATE( AllocaInst,
			                                var_type.ToLLVMType( context ),
			                                GLS_Tok_string( identifier_tok ),
			                                context->BB );
			var_decl_s.name = GLS_Tok_symbol( identifier_tok );
			var_decl_s.type = var_type;
			var_decl_s.initialised = false;

			// Attempt to emit the initialiser
			ExpressionValue* initialiser_val =
			    ExpressionEmitterRegistry::GetRegistry()->Emit(
			        context, initialiser );

			// If the initialiser is non void (i.e. not a NOP), assign
			// the value.
			if ( initialiser_val->GetType().Specifier !=
			        FullType::TySpecVoid ) {
				var_decl_s.initialised = true;
				LLVM_CREATE( StoreInst, initialiser_val->GetLLVMValue(),
				             var_decl_s.value, context->BB );
			}

			// Release the initialiser since we have no more use
			// for it
			FIRTREE_SAFE_RELEASE( initialiser_val );

			// Record this declaration
			sym_table->AddDeclaration( var_decl_s );
		}

		/// Emit the passed expression term returning a pointer to
		/// a ExpressionValue containing it's value. It is the
		/// responsibility of the caller to Release() this value.
		virtual ExpressionValue* Emit( LLVMContext* context,
		                               firtreeExpression expression ) {
			firtreeFullySpecifiedType var_ft_type;
			firtreeSingleDeclaration decl_head;
			GLS_Lst( firtreeSingleDeclaration ) decl_tail;
			if ( !firtreeExpression_declare( expression,
			                                 &var_ft_type, &decl_head,
			                                 &decl_tail ) ) {
				FIRTREE_LLVM_ICE( context, expression,
				                  "Invalid declaration." );
			}

			FullType var_type = FullType::FromFullySpecifiedType(
			                        var_ft_type );

			EmitSingleDeclaration( context, var_type, decl_head );

			GLS_Lst( firtreeSingleDeclaration ) it;
			GLS_FORALL( it, decl_tail ) {
				firtreeSingleDeclaration d = GLS_FIRST(
				                                 firtreeSingleDeclaration,
				                                 it );
				EmitSingleDeclaration( context, var_type, d );
			}

			// Variable declarations have no value.
			return VoidExpressionValue::Create( context );
		}
};

//===========================================================================
// Register the emitter.
RegisterEmitter<VariableDeclarationEmitter>
g_VariableDeclarationEmitterReg( "declare" );

}

// vim:sw=4:ts=4:cindent:noet
