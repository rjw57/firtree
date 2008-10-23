//===========================================================================
/// \file llvm_backend.h LLVM output backend for the firtree kernel language.
///
/// This file defines the interface to the LLVM output backend. The backend
/// also takes care of checking the well-formedness of the passed abstract
/// depth grammar.

#ifndef __LLVM_BACKEND_H
#define __LLVM_BACKEND_H

#include "ptm_gen.h" // General Parsing Routines
#include "ptm_pp.h"  // Pretty Printer
#include "gls.h"     // General Language Services
#include "symbols.h" // Datatype: Symbols

#include "llvm/Module.h"
#include "llvm/DerivedTypes.h"
#include "llvm/Constants.h"

#include "../gen/firtree_int.h"

// STL templates
#include <vector>
#include <map>

namespace Firtree
{

class CompileErrorException;

/// Opaque type used for passing the current LLVM context around
/// between code-generators.

struct LLVMContext;

//===========================================================================
/// \brief A structure defining a fully specified type.

struct FullType {
	//=======================================================================
	/// \brief The possible type qualifiers.
	enum TypeQualfier {
		TyQualNone,           ///< The 'default' qualifier.
		TyQualConstant,       ///< The value is const (i.e. non-assignable).
		TyQualInvalid = -1,   ///< An 'invalid' qualifier.
	};

	//=======================================================================
	/// \brief The possible types in the Firtree kernel language.
	///
	/// Note that during code generation, the '__color' type is aliased to
	/// vec4 and the sampler type is aliased to const int.
	enum TypeSpecifier {
		TySpecFloat,          ///< A 32-bit floating point.
		TySpecInt,            ///< A 32-bit signed integet.
		TySpecBool,           ///< A 1-bit boolean.
		TySpecVec2,           ///< A 2 component floating point vector.
		TySpecVec3,           ///< A 3 component floating point vector.
		TySpecVec4,           ///< A 4 component floating point vector.
		TySpecSampler,        ///< An image sampler.
		TySpecColor,          ///< A colour.
		TySpecVoid,           ///< A 'void' type.
		TySpecInvalid = -1,   ///< An 'invalid' type.
	};

	TypeQualfier    Qualifier;
	TypeSpecifier   Specifier;

	/// The constructor defines the default values.
	inline FullType()
			: Qualifier( TyQualInvalid ), Specifier( TySpecInvalid ) { }

	/// Return a flag indicating the validity of the passed type.
	inline static bool IsValid( const FullType& t ) {
		return ( t.Qualifier != TyQualInvalid ) &&
		       ( t.Specifier != TySpecInvalid );
	}

	/// Static initialiser from a perse tree qualifier and specifier. If
	/// the qualifier is NULL, it is assumed to be TyQualNone.
	static FullType FromQualiferAndSpecifier( firtreeTypeQualifier qual,
	        firtreeTypeSpecifier spec );

	/// Static initialiser from a perse tree fully specified type.
	static FullType FromFullySpecifiedType( firtreeFullySpecifiedType t );

	/// Convert this type to the matching LLVM type. Pass a LLVM context
	/// which can be used for error reporting.
	const llvm::Type* ToLLVMType( LLVMContext* ctx ) const;

	inline bool operator == ( const FullType& b ) const {
		return ( Qualifier == b.Qualifier ) && ( Specifier == b.Specifier );
	}

	inline bool operator != ( const FullType& b ) const {
		return ( Qualifier != b.Qualifier ) || ( Specifier != b.Specifier );
	}
};

//===========================================================================
/// \brief A structure recording the declaration of a variable.

struct VariableDeclaration {
	/// The LLVM value which points to the variable in memory.
	llvm::Value*      value;

	/// The Styx symbol associated with the variable's name.
	symbol            name;

	/// The variable's type.
	FullType          type;

	/// Is the variable initialised. i.e., has it been assigned to at least
	/// once.
	bool              initialised;
};

//===========================================================================
/// \brief A scoped symbol table
///
/// This class defines the interface to a scoped symbol table implementation.
/// Each symbol is associated with a VariableDeclaration. A backend uses this
/// table to record associations between a symbol's name and the declaration
/// of the variable associated with it.
///
/// The table supports nested 'scopes' which allows symbol definitions to
/// be removed, or 'popped', when the scope changes. 'Popping' the scope
/// causes all symbols added to the table since the matching 'push' to
/// be removed. Scope pushes/pops can be nested.

class SymbolTable
{

	public:
		SymbolTable();
		virtual ~SymbolTable();

		// Push the current scope onto the stack.
		void PushScope();

		// Pop the current scope from the stack. All symbols added since the
		// last (nested) call to PushScope are removed.
		void PopScope();

		// Find the variable declaration corresponding to the passed symbol.
		// If there is no matching declaration, return NULL.
		const VariableDeclaration* LookupSymbol( symbol s ) const;

		// Add the passed VariableDeclaration to the symbol table.
		void AddDeclaration( const VariableDeclaration& decl );

	private:
		// A vector of scopes, each scope is a map between a symbol
		// and a pointer to the associated value.
		std::vector< std::map<symbol, VariableDeclaration> > m_scopeStack;
};

//===========================================================================
/// \brief The LLVM backend class.
///
/// This class defines the interface to the LLVM output backend. A compiler
/// constructs an instance of this class with a Styx abstract depth grammer
/// and can query the LLVM module genereated therefrom.

class LLVMBackend
{

	public:
		/// Construct the backend by passing it the top-level translation
		/// unit node.
		LLVMBackend( firtree top_level_term );

		virtual ~LLVMBackend();

		/// Retrieve the LLVM module which was constructed.
		const llvm::Module* GetCompiledModule() const;

		/// Retrieve the compilation success flag: true on success, false
		/// otherwise.
		bool GetCompilationSucceeded() const;

		/// Retrieve the compilation log.
		const std::vector<std::string>& GetLog() const {
			return m_Log;
		}

		//===================================================================
		/// These methods are intended to be called by code generation
		/// objects.
		///@{

		/// Throw a compiler exception.
		void ThrowCompileErrorException(
		    const char* file, int line, const char* func,
		    bool is_ice, PT_Term term, const char* format, ... );

		/// Handle a compiler error exception by recording it in the error
		/// log.
		void HandleCompilerError(
		    const CompileErrorException& error_exception );

		/// Record a warning in the log.
		void RecordWarning( PT_Term term, const char* format, ... );

		/// Record an error or warning in the log.
		void RecordMessage( PT_Term term, bool is_error,
		                    const char* format, ... );

		///@}

	private:
		LLVMContext*       m_LLVMContext;     ///< The current LLVM context.

		std::vector<std::string>  m_Log;      ///< The compilation log.

		bool               m_Status;          ///< The compilation status.
};

} // namespace Firtree

#endif // __LLVM_BACKEND_H 

// vim:sw=4:ts=4:cindent:noet
