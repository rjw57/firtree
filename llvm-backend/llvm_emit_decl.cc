//===========================================================================
/// \file llvm_emit_decl.cc Implementation of Firtree::EmitDeclarations.

#include "llvm_backend.h"
#include "llvm_private.h"

#include <firtree/main.h>

#include "llvm_emit_decl.h"
#include "llvm_expression.h"

#include <llvm/Instructions.h>

using namespace llvm;

namespace Firtree
{

//===========================================================================
std::string FunctionPrototype::GetMangledName( LLVMContext* ctx ) const
{
	std::string namestr(symbolToString(Name));

	// If no parameters, leave it as that
	if(Parameters.size() == 0)
	{
		return namestr;
	}

	namestr += "_";

	std::vector<FunctionParameter>::const_iterator it =
		Parameters.begin();
	for( ; it != Parameters.end(); ++it)
	{
		// Depending on the type of the parameter, append a
		// type code.
		switch(it->Type.Specifier)
		{
			case FullType::TySpecFloat:
				namestr += "f";
				break;
			case FullType::TySpecInt:
				namestr += "i";
				break;
			case FullType::TySpecBool:
				namestr += "b";
				break;
			case FullType::TySpecVec2:
				namestr += "v2";
				break;
			case FullType::TySpecVec3:
				namestr += "v3";
				break;
			case FullType::TySpecVec4:
				namestr += "b4";
				break;
			case FullType::TySpecSampler:
				namestr += "s";
				break;
			case FullType::TySpecColor:
				namestr += "c";
				break;
			case FullType::TySpecVoid:
				namestr += "v";
				break;
			default:
				FIRTREE_LLVM_ICE( ctx, NULL, "Invalid type.");
				break;
		}
	}

	return namestr;
}

//===========================================================================
EmitDeclarations::EmitDeclarations( LLVMContext* ctx )
{
	m_Context = ctx;
}

//===========================================================================
EmitDeclarations::~EmitDeclarations()
{
	// Not really necessary but it gives one the warm fuzzy feeling of being
	// a Good Boy.
	m_Context = NULL;
}

//===========================================================================
/// Emit a single firtree declaration.
void EmitDeclarations::emitDeclaration( firtreeExternalDeclaration decl )
{
	firtreeFunctionDefinition func_def;
	firtreeFunctionPrototype func_proto;

	// Decide if this declaration is a prototype or definition and
	// act accordingly.

	if ( firtreeExternalDeclaration_definefuntion( decl, &func_def ) ) {
		emitFunction( func_def );
	} else if ( firtreeExternalDeclaration_declarefunction( decl,
	            &func_proto ) ) {
		emitPrototype( func_proto );
	} else {
		FIRTREE_LLVM_ICE( m_Context, decl, "Unknown declaration node." );
	}
}

//===========================================================================
/// Emit a list of firtree declarations.
void EmitDeclarations::emitDeclarationList(
    GLS_Lst( firtreeExternalDeclaration ) decl_list )
{
	GLS_Lst( firtreeExternalDeclaration ) tail = decl_list;
	firtreeExternalDeclaration decl;

	GLS_FORALL( tail, decl_list ) {
		decl = GLS_FIRST( firtreeExternalDeclaration, tail );

		// Wrap each external declaration in an exception handler so
		// we can re-start from here.

		try {
			emitDeclaration( decl );
		} catch ( CompileErrorException e ) {
			m_Context->Backend->HandleCompilerError( e );
		}
	}
}

//===========================================================================
/// Emit a function prototype.
void EmitDeclarations::emitPrototype( firtreeFunctionPrototype proto_term )
{
	FunctionPrototype prototype;
	constructPrototypeStruct( prototype, proto_term );

	// Check this prototype does not conflict with an existing one.

	if ( existsConflictingPrototype( prototype ) ) {
		FIRTREE_LLVM_ERROR( m_Context, proto_term, "Function '%s' "
		                    "conflicts with previously declared or defined "
		                    "function.", symbolToString( prototype.Name ) );
	}

	prototype.LLVMFunction = ConstructFunction(prototype);

	// Insert this prototype into the function table.
	m_Context->FuncTable.insert(
	    std::pair<symbol, FunctionPrototype>( prototype.Name, prototype ) );
}

//===========================================================================
/// Construct an LLVM function object corresponding to a particular
/// prototype.
llvm::Function* EmitDeclarations::ConstructFunction(
		const FunctionPrototype& prototype)
{
	// Create a LLVM function object corresponding to this definition.

	std::vector<const Type*> param_llvm_types;

	std::vector<FunctionParameter>::const_iterator it =
	    prototype.Parameters.begin();

	while ( it != prototype.Parameters.end() ) {
		// output parameters are passed by reference.
		const llvm::Type* base_type = it->Type.ToLLVMType( m_Context );
		const llvm::Type* param_type = base_type;

		if (( it->Direction == FunctionParameter::FuncParamOut ) ||
		        ( it->Direction == FunctionParameter::FuncParamInOut ) ) {
			param_type = PointerType::get( base_type, 0 );
		}

		param_llvm_types.push_back( param_type );

		it++;
	}

	FunctionType *FT = FunctionType::get(
	                       prototype.ReturnType.ToLLVMType( m_Context ),
	                       param_llvm_types, false );
	Function* F = NULL;

	std::string func_name = prototype.GetMangledName( m_Context );

	llvm::Function::LinkageTypes linkage;
	switch( prototype.Qualifier )
	{
		case FunctionPrototype::FuncQualKernel:
			linkage = Function::ExternalLinkage;
			break;
		case FunctionPrototype::FuncQualFunction:
			linkage = Function::InternalLinkage;
			break;
		case FunctionPrototype::FuncQualIntrinsic:
			linkage = Function::ExternalLinkage;
			break;
		default:
			FIRTREE_LLVM_ICE( m_Context, NULL, "Invalid linkage.");
			break;
	}

	F = LLVM_CREATE( Function, FT,
			linkage,
			func_name.c_str(),
			m_Context->Module );

	// Check that the name we've asked for is available.
	std::string llvm_name = F->getName();
	if(func_name != llvm_name)
	{
		FIRTREE_LLVM_ICE( m_Context, NULL, 
				"LLVM wouldn't name function '%s', got "
				"'%s' instead.",
				func_name.c_str(),
				llvm_name.c_str());
	}
	
	return F;
}

//===========================================================================
/// Emit a function definition.
void EmitDeclarations::emitFunction( firtreeFunctionDefinition func )
{
	firtreeFunctionPrototype function_prototype;
	GLS_Lst( firtreeExpression ) function_body;

	if ( !firtreeFunctionDefinition_functiondefinition( func,
	        &function_prototype, &function_body ) ) {
		FIRTREE_LLVM_ICE( m_Context, func, "Invalid function definition." );
	}

	// Form prototype struct from parse tree term.
	FunctionPrototype prototype;

	constructPrototypeStruct( prototype, function_prototype );

	// Do we have an existing prototype for this function?
	std::multimap<symbol, FunctionPrototype>::iterator existing_proto_it;

	if ( existsConflictingPrototype( prototype, &existing_proto_it ) ) {
		// Check that the existing prototype does not have a definition
		// already.
		if ( existing_proto_it->second.DefinitionTerm != NULL ) {
			FIRTREE_LLVM_ERROR( m_Context, func, "Multiple defintions of "
			                    "function '%s'.",
			                    symbolToString( prototype.Name ) );
		}

		// Check the function qualifiers
		if ( existing_proto_it->second.Qualifier != prototype.Qualifier ) {
			FIRTREE_LLVM_ERROR( m_Context, func, "Conflicting function "
			                    "qualifiers between definition and "
			                    "declaration of function '%s'.",
			                    symbolToString( prototype.Name ) );
		}

		// Check the return types are compatible.
		if ( existing_proto_it->second.ReturnType != prototype.ReturnType ) {
			FIRTREE_LLVM_ERROR( m_Context, func, "Conflicting return types "
			                    "between definition and declaration of "
			                    "function '%s'.",
			                    symbolToString( prototype.Name ) );
		}

		// Copy and erase the old prototype.
		prototype = existing_proto_it->second;
		m_Context->FuncTable.erase( existing_proto_it );
	} else {
		// If there was no existing prototype, construct a
		// LLVM function
		prototype.LLVMFunction = ConstructFunction(prototype);
	}

	prototype.DefinitionTerm = func;
	llvm::Function* F = prototype.LLVMFunction;

	// This function does not yet exist in the function table, since
	// we erase it above if it did so. Insert it.
	m_Context->FuncTable.insert(
	    std::pair<symbol, FunctionPrototype>( prototype.Name, prototype ) );

	m_Context->Function = F;
	m_Context->CurrentPrototype = &prototype;

	// Create a basic block for this function
	BasicBlock *BB = LLVM_CREATE( BasicBlock, "entry", F );
	m_Context->BB = BB;

	// Create a symbol table
	m_Context->Variables = new SymbolTable();

	// Set names for all arguments.
	std::vector<FunctionParameter>::const_iterator it = 
		prototype.Parameters.begin();
	Function::arg_iterator AI = F->arg_begin();

	while ( it != prototype.Parameters.end() ) {
		if ( it->Name != NULL ) {
			AI->setName( symbolToString( it->Name ) );

			// We now add this parameter to the function table. There
			// are three sorts of parameter: in, out and inout. We
			// treat each case differently.

			switch ( it->Direction ) {

				case FunctionParameter::FuncParamIn: {
					// Variables are assumed to be implemented as pointers to
					// the underlying value. Since we pass 'in' parameters by
					// value, allocate a temporary variable on the stack to
					// hold the value.
					Value* paramLoc = LLVM_CREATE(
					                      AllocaInst,
					                      it->Type.ToLLVMType( m_Context ),
					                      "paramtmp", m_Context->BB
					                  );
					Value* argValue = cast<Value>( AI );
					LLVM_CREATE( StoreInst, argValue, paramLoc,
					             m_Context->BB );

					// Create a 'declaration' for this parameter.
					VariableDeclaration var_decl;
					var_decl.value = paramLoc;
					var_decl.name = it->Name;
					var_decl.type = it->Type;
					var_decl.initialised = true;

					// Add this parameter to the symbol table.
					m_Context->Variables->AddDeclaration( var_decl );
				}

				break;

				case FunctionParameter::FuncParamInOut:

				case FunctionParameter::FuncParamOut: {
					// The only difference between inout and out parameters
					// is that inout parameters are initialised. We use the
					// parameter value directly as a pointer to the variable.

					VariableDeclaration var_decl;
					var_decl.value = cast<Value>( AI );
					var_decl.name = it->Name;
					var_decl.type = it->Type;

					if ( it->Direction ==
					        FunctionParameter::FuncParamInOut ) {
						var_decl.initialised = true;
					} else {
						var_decl.initialised = false;
					}

					// Add this parameter to the symbol table.
					m_Context->Variables->AddDeclaration( var_decl );
				}

				break;

				default:
					FIRTREE_LLVM_ICE( m_Context, it->Term,
					                  "Unknown parameter type." );
			}
		}

		++it;

		++AI;
	}

	// Do the following inside using try/catch block
	// so that compile errors don't cause us to leak memory and
	// we can continue after errors.
	GLS_Lst( firtreeExpression ) tail;
	GLS_FORALL( tail, function_body ) {
		firtreeExpression e = GLS_FIRST( firtreeExpression, tail );
		try {
			// Emit expression
			ExpressionValue* emitted_val =
			    ExpressionEmitterRegistry::GetRegistry()->Emit(
			        m_Context, e );
			// Free the emitted value
			FIRTREE_SAFE_RELEASE( emitted_val );
		} catch ( CompileErrorException e ) {
			m_Context->Backend->HandleCompilerError( e );
		}
	}

	// Create a default return inst if there was no terminator.
	if ( m_Context->BB->getTerminator() == NULL ) {
		// Check function return type is void.
		if ( prototype.ReturnType.Specifier != FullType::TySpecVoid ) {
			FIRTREE_LLVM_ERROR( m_Context, func, "Control reaches end "
			                    "of non-void function." );
		}

		LLVM_CREATE( ReturnInst, NULL, m_Context->BB );
	}

	delete m_Context->Variables;

	m_Context->Variables = NULL;
	m_Context->BB = NULL;
	m_Context->Function = NULL;
	m_Context->CurrentPrototype = NULL;
}

//===========================================================================
/// Check for undefined functions (i.e. functions with
/// prototypes but no definition).
void EmitDeclarations::checkEmittedDeclarations()
{
	std::multimap<symbol, FunctionPrototype>::const_iterator it =
	    m_Context->FuncTable.begin();

	while ( it != m_Context->FuncTable.end() ) {
		if (( it->second.DefinitionTerm == NULL ) &&
		        ( it->second.Qualifier !=
		          FunctionPrototype::FuncQualIntrinsic ) ) {
			try {
				FIRTREE_LLVM_ERROR( m_Context, it->second.PrototypeTerm,
				                    "No definition found for function '%s'.",
				                    symbolToString( it->second.Name ) );
			} catch ( CompileErrorException e ) {
				m_Context->Backend->HandleCompilerError( e );
			}
		}

		it++;
	}
}

//===========================================================================
/// Return true if there already exists a prototype registered
/// in the function table which conflicts with the passed prototype.
/// A prototype conflicts if it matches both the function name of
/// an existing prototype and the type and number of it's parameters.
///
/// Optionally, if matched_proto_iter_ptr is non-NULL, write an
/// iterator pointing to the matched prototype into it. If no prototype
/// is matched, matched_proto_iter_ptr has m_Context->FuncTable.end() written
/// into it.
bool EmitDeclarations::existsConflictingPrototype(
    const FunctionPrototype& proto,
    std::multimap<symbol, FunctionPrototype>::iterator*
    matched_proto_iter_ptr )
{
	// Initially, assume no matching prototype.
	if ( matched_proto_iter_ptr != NULL ) {
		*matched_proto_iter_ptr = m_Context->FuncTable.end();
	}

	// First test: does there exist at least one prototype with a matching
	// name.
	if ( m_Context->FuncTable.count( proto.Name ) == 0 ) {
		// No match, return false.
		return false;
	}

	// If we got this far, there a potential conflict. Iterate over all of
	// them.
	std::multimap<symbol, FunctionPrototype>::
	iterator it = m_Context->FuncTable.begin();

	for ( ; it != m_Context->FuncTable.end(); it ++ ) {
		if ( it->first != proto.Name )
			continue;

		// Sanity check.
		FIRTREE_LLVM_ASSERT( m_Context, NULL, it->second.Name == proto.Name );

		// If the number of parameters is different, no conflict
		if ( it->second.Parameters.size() != proto.Parameters.size() ) {
			continue;
		}

		// Clear the is_conflict flag if any parameters have differing
		// type.
		bool is_conflict = true;

		std::vector<FunctionParameter>::const_iterator it_a =
		    it->second.Parameters.begin();

		std::vector<FunctionParameter>::const_iterator it_b =
		    proto.Parameters.begin();

		while (( it_a != it->second.Parameters.end() ) &&
		        ( it_b != proto.Parameters.end() ) ) {
			if ( it_a->Type != it_b->Type ) {
				is_conflict = false;
			}

			it_a++;

			it_b++;
		}

		// Test the is_conflict flag and return signalling conflict if
		// one found.
		if ( is_conflict ) {
			if ( matched_proto_iter_ptr != NULL ) {
				*matched_proto_iter_ptr = it;
			}

			return true;
		}
	}

	// If we got this far, none of the candidate prototypes conflicted.
	return false;
}

//===========================================================================
/// Construct a FunctionPrototype struct from a
/// firtreeFunctionPrototype parse-tree term.
void EmitDeclarations::constructPrototypeStruct(
    FunctionPrototype& prototype,
    firtreeFunctionPrototype proto_term )
{
	firtreeFunctionQualifier qual;
	firtreeFullySpecifiedType type;
	GLS_Tok name;
	GLS_Lst( firtreeParameterDeclaration ) params;

	if ( !firtreeFunctionPrototype_functionprototype( proto_term,
	        &qual, &type, &name, &params ) ) {
		FIRTREE_LLVM_ICE( m_Context, proto_term,
		                  "Invalid function prototype." );
	}

	prototype.PrototypeTerm = proto_term;

	prototype.DefinitionTerm = NULL;
	prototype.LLVMFunction = NULL;

	if ( firtreeFunctionQualifier_function( qual ) ) {
		prototype.Qualifier = FunctionPrototype::FuncQualFunction;
	} else if ( firtreeFunctionQualifier_kernel( qual ) ) {
		prototype.Qualifier = FunctionPrototype::FuncQualKernel;
	} else if ( firtreeFunctionQualifier_builtin( qual ) ) {
		prototype.Qualifier = FunctionPrototype::FuncQualIntrinsic;
	} else {
		FIRTREE_LLVM_ICE( m_Context, qual, "Invalid function qualifier." );
	}

	prototype.Name = GLS_Tok_symbol( name );

	if ( prototype.Name == NULL ) {
		FIRTREE_LLVM_ICE( m_Context, type, "Invalid name." );
	}

	prototype.ReturnType = FullType::FromFullySpecifiedType( type );

	if ( !FullType::IsValid( prototype.ReturnType ) ) {
		FIRTREE_LLVM_ICE( m_Context, type, "Invalid type." );
	}

	// For each parameter, append a matching FunctionParameter structure
	// to the prototype's Parameters field.
	GLS_Lst( firtreeParameterDeclaration ) params_tail;

	GLS_FORALL( params_tail, params ) {
		firtreeParameterDeclaration param_decl =
		    GLS_FIRST( firtreeParameterDeclaration, params_tail );

		firtreeTypeQualifier param_type_qual;
		firtreeParameterQualifier param_qual;
		firtreeTypeSpecifier param_type_spec;
		firtreeParameterIdentifierOpt param_identifier;

		if ( firtreeParameterDeclaration_parameterdeclaration( param_decl,
		        &param_type_qual, &param_qual, &param_type_spec,
		        &param_identifier ) ) {
			FunctionParameter new_param;

			new_param.Term = param_decl;

			// Extract the parameter type
			new_param.Type = FullType::FromQualiferAndSpecifier(
			                     param_type_qual, param_type_spec );

			if ( !FullType::IsValid( new_param.Type ) ) {
				FIRTREE_LLVM_ICE( m_Context, param_decl, "Invalid type." );
			}

			// Get the parameter name
			GLS_Tok parameter_name_token;

			if ( firtreeParameterIdentifierOpt_named( param_identifier,
			        &parameter_name_token ) ) {
				new_param.Name = GLS_Tok_symbol( parameter_name_token );
			} else if ( firtreeParameterIdentifierOpt_anonymous(
			                param_identifier ) ) {
				new_param.Name = NULL;
			} else {
				FIRTREE_LLVM_ICE( m_Context, param_identifier,
				                  "Invalid identifier." );
			}

			// Get the parameter direction
			if ( firtreeParameterQualifier_in( param_qual ) ) {
				new_param.Direction = FunctionParameter::FuncParamIn;
			} else if ( firtreeParameterQualifier_out( param_qual ) ) {
				new_param.Direction = FunctionParameter::FuncParamOut;
			} else if ( firtreeParameterQualifier_inout( param_qual ) ) {
				new_param.Direction = FunctionParameter::FuncParamInOut;
			} else {
				FIRTREE_LLVM_ICE( m_Context, param_qual,
				                  "Invalid parameter qualifier." );
			}

			// Kernels must have 'in' parameters only.
			if ( !firtreeParameterQualifier_in( param_qual ) &&
			        ( prototype.Qualifier ==
			          FunctionPrototype::FuncQualKernel ) ) {
				FIRTREE_LLVM_ERROR( m_Context, param_decl,
				                    "Kernel functions must "
				                    "only have 'in' parameters." );
			}

			prototype.Parameters.push_back( new_param );
		} else {
			FIRTREE_LLVM_ICE( m_Context, param_decl,
			                  "Invalid parameter declaration." );
		}
	}

	// Kernels must return vec4
	if ( prototype.Qualifier == FunctionPrototype::FuncQualKernel ) {
		if ( prototype.ReturnType.Specifier != FullType::TySpecVec4 ) {
			FIRTREE_LLVM_ERROR( m_Context, proto_term, "Kernel functions "
			                    "must have a return type of vec4." );
		}
	}
}

} // namespace Firtree

// vim:sw=4:ts=4:cindent:noet
