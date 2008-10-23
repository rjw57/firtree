//===========================================================================
/// \file symbol_table.cc Implementation of Firtree::SymbolTable.

#include "llvm_backend.h"

#include <firtree/main.h>

using namespace llvm;

namespace Firtree
{

//===========================================================================
SymbolTable::SymbolTable()
{
	// Push the default (empty) scope.
	PushScope();
}

//===========================================================================
SymbolTable::~SymbolTable()
{
	// nothing to do here.
}

//===========================================================================
// Push the current scope onto the stack.
void SymbolTable::PushScope()
{
	m_scopeStack.push_back( std::map<symbol, VariableDeclaration> () );
}

//===========================================================================
// Pop the current scope from the stack. All symbols added since the
// last (nested) call to PushScope are removed.
void SymbolTable::PopScope()
{
	m_scopeStack.pop_back();
}

//===========================================================================
// Find the variable declaration corresponding to the passed symbol.
// If there is no matching declaration, return NULL.
const VariableDeclaration* SymbolTable::LookupSymbol( symbol sym ) const
{
	std::vector< std::map<symbol, VariableDeclaration> >::
	const_reverse_iterator i = m_scopeStack.rbegin();

	for ( ; i != m_scopeStack.rend(); i++ ) {
		const std::map<symbol, VariableDeclaration>& sym_table = *i;

		if ( sym_table.count( sym ) != 0 ) {
			return &( sym_table.find( sym )->second );
		}
	}

	return NULL;
}

//===========================================================================
// Add the passed VariableDeclaration to the symbol table.
void SymbolTable::AddDeclaration( const VariableDeclaration& decl )
{
	// Check we don't have this synbol already.
	if ( LookupSymbol( decl.name ) != NULL ) {
		FIRTREE_ERROR( "Symbol named '%s' already exists in symbol table.",
		               symbolToString( decl.name ) );
		return;
	}

	m_scopeStack.back().insert(

	    std::pair<symbol, VariableDeclaration>( decl.name, decl ) );
}

}

// vim:sw=4:ts=4:cindent:noet
