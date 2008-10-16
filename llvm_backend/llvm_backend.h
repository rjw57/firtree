//===========================================================================
/// \file llvm_backend.h LLVM output backend for the firtree kernel language.
///
/// This file defines the interface to the LLVM output backend. The backend
/// also takes care of checking the well-formedness of the passed abstract
/// depth grammar.

#ifndef __LLVM_OUT_H
#define __LLVM_OUT_H

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

namespace Firtree {

//===========================================================================
/// \brief A structure defining a fully qualified type.
struct Type {
  //=========================================================================
  /// \brief The possible type qualifiers.
  enum TypeQualfier {
    TyQualNone,           ///< The 'default' qualifier.
    TyQualConstant,       ///< The value is const (i.e. non-assignable).
    TyQualInvalid = -1,   ///< An 'invalid' qualifier.
  };

  //=========================================================================
  /// \brief The possible types in the Firtree kernel language. 
  ///
  /// Note that during code generation, the '__color' type is aliased to vec4 
  /// and the sampler type is aliased to const int.
  enum TypeSpecifier {
    TySpecFloat,          ///< A 32-bit floating point.
    TySpecInt,            ///< A 32-bit signed integet.
    TySpecBool,           ///< A 1-bit boolean.
    TySpecVec2,           ///< A 2 component floating point vector.
    TySpecVec3,           ///< A 3 component floating point vector.
    TySpecVec4,           ///< A 4 component floating point vector.
    TySpecSampler,        ///< An image sampler.
    TySpecColor,          ///< A colour.
    TySpecInvalid = -1,   ///< An 'invalid' type.
  };

  TypeQualfier    qualifier;
  TypeSpecifier   specifier;

  /// The constructor defines the default values.
  inline Type() : qualifier(TyQualNone), specifier(TySpecInvalid) { }
};

//===========================================================================
/// \brief A structure recording the declaration of a variable.
struct VariableDeclaration {
  llvm::Value*      value;    ///< The LLVM value which points to the 
                              ///< variable in memory.
  symbol            name;     ///< The Styx symbol associated with the 
                              ///< variable's name.
  Type              type;     ///< The variable's type.
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
class SymbolTable {
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
    const VariableDeclaration* LookupSymbol(symbol s) const;

    // Add the passed VariableDeclaration to the symbol table.
    void AddDeclaration(const VariableDeclaration& decl);

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
class LLVMBackend {
  public:

  private:
};

} // namespace Firtree

#endif // __LLVM_OUT_H 

// vim:sw=2:ts=2:cindent:et 
