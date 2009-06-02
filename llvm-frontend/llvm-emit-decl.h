//===========================================================================
/// \file llvm-emit-decl.h Emit top-level declarations.

#ifndef __LLVM_EMIT_DECL_H
#define __LLVM_EMIT_DECL_H

#include "llvm-frontend.h"
#include "llvm-private.h"

namespace llvm {
	class Function;
}

namespace Firtree
{

//===========================================================================
/// \brief An object which knows how to emit a list of declarations.

class EmitDeclarations
{

	public:
		EmitDeclarations( LLVMContext* ctx );
		virtual ~EmitDeclarations();

		/// Emit a single firtree declaration.
		void emitDeclaration( firtreeExternalDeclaration decl );

		/// Emit a list of firtree declarations.
		void emitDeclarationList(
		    GLS_Lst( firtreeExternalDeclaration ) decl_list );

		/// Emit a function prototype.
		void emitPrototype( firtreeFunctionPrototype proto );

		/// Emit a function definition.
		void emitFunction( firtreeFunctionDefinition func );

		/// Check for undefined functions (i.e. functions with
		/// prototypes but no definition).
		void checkEmittedDeclarations();

	protected:
		/// Return true if there already exists a prototype registered
		/// in the function table which conflicts with the passed prototype.
		/// A prototype conflicts if it matches both the function name of
		/// an existing prototype and the type and number of it's parameters.
		///
		/// Optionally, if matched_proto_iter_ptr is non-NULL, write an
		/// iterator pointing to the matched prototype into it. If no
		/// prototype is matched, matched_proto_iter_ptr has 
		/// m_Context->FuncTable.end() written into it.
		bool existsConflictingPrototype( const FunctionPrototype& proto,
		                                 std::multimap<symbol,
		                                 FunctionPrototype>::iterator*
		                                 matched_proto_iter_ptr = NULL );

		/// Construct a FunctionPrototype struct from a
		/// firtreeFunctionPrototype parse-tree term.
		void constructPrototypeStruct( FunctionPrototype& to_construct,
		                               firtreeFunctionPrototype proto );

		/// Construct an LLVM function object corresponding to a particular
		/// prototype.
		llvm::Function* ConstructFunction(
				const FunctionPrototype& prototype);

	private:
		/// The current LLVM context.
		LLVMContext*      m_Context;
};

} // namespace Firtree

#endif // __LLVM_EMIT_DECL_H 

// vim:sw=4:ts=4:cindent:noet
