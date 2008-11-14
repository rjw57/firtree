//===========================================================================
/// \file llvm_expression.cc

#include <firtree/main.h>

#include "llvm_frontend.h"
#include "llvm_private.h"
#include "llvm_expression.h"
#include "llvm_emit_constant.h"
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

        //===================================================================
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

			// Scan the function table looking for a matching function
			// and emit a call to it. If this loop finishes, no
			// matching function was found.
			std::multimap<symbol, FunctionPrototype>::const_iterator it =
				context->FuncTable.begin();

			for( ; it != context->FuncTable.end(); ++it)
			{
				if(it->first == GLS_Tok_symbol(identifier))
				{
					const FunctionPrototype& proto = it->second;

					//FIRTREE_LLVM_WARNING( context, func_spec, 
					//		"Examine: %s",
					//		it->second.GetMangledName(context).c_str());

					// Check parameter count matches.
					if(parameters.size() != proto.Parameters.size())
					{
						continue;
					}

					//FIRTREE_LLVM_WARNING( context, func_spec, 
					//		"Correct param count: %s",
					//		it->second.GetMangledName(context).c_str());

					// Check parameter types match.
					bool params_match = true;
					for(unsigned int i=0; i<parameters.size(); i++)
					{
						params_match &= TypeCaster::CanImplicitlyCast(
								parameters[i]->GetType(),
								proto.Parameters[i].Type);
					}

					if(!params_match)
					{
						continue;
					}

					//FIRTREE_LLVM_WARNING( context, func_spec, 
					//		"Possible match: %s",
					//		it->second.GetMangledName(context).c_str());

					std::vector<ExpressionValue*> func_params;
					std::vector<llvm::Value*> llvm_params;
					try {
						for(unsigned int i=0; i<parameters.size(); ++i)
						{
							func_params.push_back(TypeCaster::CastValue(
										context, func_spec,
										parameters[i],
										proto.Parameters[i].Type.Specifier));
							llvm_params.push_back(
									func_params.back()->GetLLVMValue());
						}

						// If we get this far then a match has been found.
						bool is_void =
							proto.ReturnType.Specifier == FullType::TySpecVoid;
						llvm::Value* func_call = LLVM_CREATE(CallInst,
								proto.LLVMFunction,
								llvm_params.begin(), llvm_params.end(),
								is_void ? "" : "tmpfuncretval",
								context->BB);

						while(func_params.size() != 0) {
							FIRTREE_SAFE_RELEASE(func_params.back());
							func_params.pop_back();
						}

						return ConstantExpressionValue::Create(context,
								func_call);
					} catch (CompileErrorException e) {
						while(func_params.size() != 0) {
							FIRTREE_SAFE_RELEASE(func_params.back());
							func_params.pop_back();
						}
						throw e;
					}
				}
			}

			FIRTREE_LLVM_ERROR( context, func_spec, 
					"No function found to match call to '%s'.",
					GLS_Tok_string(identifier));

			return NULL;
		}

        //===================================================================
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
				case FullType::TySpecFloat:
				case FullType::TySpecBool:
					if ( parameters.size() != 1 ) {
						FIRTREE_LLVM_ERROR( context, func_spec,
						                    "Constructor for expects "
						                    "1 argument." );
					}
					return TypeCaster::CastValue( context,
					                              func_spec,
					                              parameters.back(),
					                              required_type.Specifier );
				default:
					// No action because we are processed by code
					// below.
					break;
			}

			// If we get this far, the constructor must be for a
			// vector type. Get the required dimension.
			uint32_t required_dimension = required_type.GetArity();

			// Vector constructors can take scalar or vector
			// parameters. Vector parameters are 'cracked' into
			// individual scalar values. This vector stores the
			// cracked parameters
			std::vector<ExpressionValue*> cracked_params;
			ExpressionValue* return_value = NULL;

			try {
				std::vector<ExpressionValue*>::iterator param_it =
				    parameters.begin();
				for ( ; param_it != parameters.end(); ++param_it ) {
					// Depending on the parameter type it is either
					// implicitly cast to a float, or cracked.
					ExpressionValue* param_value = *param_it;
					FullType param_type = param_value->GetType();
					if ( param_type.IsScalar() ) {
						cracked_params.push_back(
						    TypeCaster::CastValue( context,
						                           func_spec,
						                           param_value,
						                           FullType::TySpecFloat ) );
					} else if ( param_type.IsVector() ) {
						CrackVector( context, param_value, cracked_params );
					} else {
						FIRTREE_LLVM_ERROR( context, func_spec,
						                    "Invalid type for parameter in "
						                    "constructor." );
					}
				}

				if ( cracked_params.size() != required_dimension ) {
					FIRTREE_LLVM_ERROR( context, func_spec,
					                    "Incorrect parameter count "
					                    "for constructor." );
				}

				return_value = CreateVector( context, cracked_params );

				// Free the cracked parameter list.
				while ( cracked_params.size() > 0 ) {
					FIRTREE_SAFE_RELEASE( cracked_params.back() );
					cracked_params.pop_back();
				}
			} catch ( CompileErrorException e ) {
				// Free the cracked parameter list.
				while ( cracked_params.size() > 0 ) {
					FIRTREE_SAFE_RELEASE( cracked_params.back() );
					cracked_params.pop_back();
				}
				throw e;
			}

			if ( return_value == NULL ) {
				FIRTREE_LLVM_ERROR( context, func_spec,
				                    "No constructor for type." );
			}

			return return_value;
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
FIRTREE_LLVM_DECLARE_EMITTER(FunctionCallEmitter, functioncall)

}

// vim:sw=4:ts=4:cindent:noet
