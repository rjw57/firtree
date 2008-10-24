//===========================================================================
/// \file llvm_expression.cc

#include <firtree/main.h>

#include "llvm_backend.h"
#include "llvm_private.h"
#include "llvm_expression.h"
#include "llvm_type_cast.h"

#include <llvm/Instructions.h>

using namespace llvm;

namespace Firtree
{

//===========================================================================
/// \brief Class to emit a call to a function or type constructor.
class FunctionCallEmitter : ExpressionEmitter
{
	public:
		FunctionCallEmitter()
				: ExpressionEmitter() {
		}

		virtual ~FunctionCallEmitter() {
		}

		/// Create an instance of this emitter.
		static ExpressionEmitter* Create() {
			return new FunctionCallEmitter();
		}

		ExpressionValue* EmitFunctionCall(
		    LLVMContext* context,
		    firtreeFunctionSpecifier func_spec,
		    std::vector<ExpressionValue*>& parameters ) {

			GLS_Tok identifier;
			if ( !firtreeFunctionSpecifier_functionnamed(
			            func_spec, &identifier ) ) {
				FIRTREE_LLVM_ICE( context, func_spec,
				                  "not a function call." );
				return NULL;
			}

			FIRTREE_LLVM_ICE( context, func_spec, "FIXME: unimplemented." );
			return NULL;
		}

		ExpressionValue* EmitConstructorFor(
		    LLVMContext* context,
		    firtreeFunctionSpecifier func_spec,
		    std::vector<ExpressionValue*>& parameters ) {

			firtreeTypeSpecifier type_spec;
			if ( !firtreeFunctionSpecifier_constructorfor(
			            func_spec, &type_spec ) ) {
				FIRTREE_LLVM_ICE( context, func_spec, "not a constructor." );
				return NULL;
			}

			FullType required_type =
			    FullType::FromQualiferAndSpecifier( NULL, type_spec );

			switch ( required_type.Specifier ) {
				case FullType::TySpecInt:
					if ( parameters.size() != 1 ) {
						FIRTREE_LLVM_ERROR( context, func_spec,
						                    "Constructor for 'int' expects "
						                    "1 argument." );
					}
					return TypeCaster::CastValue( context,
					                              func_spec,
					                              parameters.back(),
					                              required_type.Specifier );
				default:
					// No action because we are caught by the call
					// to FIRTREE_LLVM_ERROR below.
					break;
			}


			FIRTREE_LLVM_ERROR( context, func_spec,
			                    "No constructor for type." );

			return NULL;
		}

		/// Emit the passed expression term returning a pointer to
		/// a ExpressionValue containing it's value. It is the
		/// responsibility of the caller to Release() this value.
		virtual ExpressionValue* Emit( LLVMContext* context,
		                               firtreeExpression expression ) {
			firtreeFunctionSpecifier func_spec;
			GLS_Lst( firtreeExpression ) param_lst;

			if ( !firtreeExpression_functioncall( expression, &func_spec,
			                                      &param_lst ) ) {
				FIRTREE_LLVM_ICE( context, expression,
				                  "Invalid func call." );
			}

			std::vector<ExpressionValue*> parameter_values;
			ExpressionValue* return_value = NULL;

			try {
				// For each parameter, emit code to calculate its value.
				GLS_Lst( firtreeExpression ) param_lst_tail;
				GLS_FORALL( param_lst_tail, param_lst ) {
					firtreeExpression e = GLS_FIRST( firtreeExpression,
					                                 param_lst_tail );
					parameter_values.push_back(
					    ExpressionEmitterRegistry::GetRegistry()->
					    Emit( context, e ) );
				}

				// Functions are one of two classes, type constructors and
				// plain ol' simple function calls.
				if ( firtreeFunctionSpecifier_constructorfor(
				            func_spec, NULL ) ) {
					return_value =  EmitConstructorFor(
					                    context, func_spec, 
										parameter_values );
				} else if ( firtreeFunctionSpecifier_functionnamed(
				                func_spec, NULL ) ) {
					return_value =  EmitFunctionCall(
					                    context, func_spec,
										parameter_values );
				} else {
					FIRTREE_LLVM_ICE( context, expression,
					                  "Unknown function term." );
				}
			} catch ( CompileErrorException e ) {
				// On error, free any values in the parameter list.
				while ( parameter_values.size() > 0 ) {
					FIRTREE_SAFE_RELEASE( parameter_values.back() );
					parameter_values.pop_back();
				}

				// Throw the error up the call chain.
				throw e;
			}

			// Free the values for the parameters.
			while ( parameter_values.size() > 0 ) {
				FIRTREE_SAFE_RELEASE( parameter_values.back() );
				parameter_values.pop_back();
			}

			return return_value;
		}
};

//===========================================================================
// Register the emitter.
RegisterEmitter<FunctionCallEmitter> g_FunctionCallEmitterReg(
    "functioncall" );

}

// vim:sw=4:ts=4:cindent:noet
