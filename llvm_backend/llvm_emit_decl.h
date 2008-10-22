//===========================================================================
/// \file llvm_emit_decl.h Emit top-level declarations.

#ifndef __LLVM_EMIT_DECL_H
#define __LLVM_EMIT_DECL_H

#include "llvm_backend.h"
#include "llvm_private.h"

namespace Firtree {

//===========================================================================
/// \brief A structure decribing a parameter to a function.
struct FunctionParameter {
  /// The possible parameter qualifiers.
  enum ParameterQualifier {
    FuncParamIn,
    FuncParamOut,
    FuncParamInOut,
    FuncParamInvalid = -1,
  };

  /// The parse tree term which defined this parameter.
  firtreeParameterDeclaration Term;

  /// The type of the parameter
  FullType                    Type;

  /// The name of the parameter (as a symbol) or NULL if the
  /// parameter is anonymous.
  symbol                      Name;

  /// The direction qualifier for the parameter.
  ParameterQualifier          Direction;          
};

//===========================================================================
/// \brief A structure decribing a prototype.
struct FunctionPrototype {
  /// The possible function types.
  enum FunctionQualifier {
    FuncQualFunction,
    FuncQualKernel,
    FuncQualIntrinsic,    ///< Intrinsic functions do not need a definition.
    FuncQualInvalid = -1,
  };

  /// The parse tree term which defined this prototype.
  firtreeFunctionPrototype    PrototypeTerm;

  /// The parse tree term containing the definition of this
  /// function. At the end of code generation, the list of function
  /// prototypes are swept and any non-intrinsic functions without
  /// a definition are reported as errors.
  firtreeFunctionDefinition   DefinitionTerm;

  /// The LLVM function associated with this prototype, or NULL if
  /// there is nont
  llvm::Function*             LLVMFunction;

  /// The function qualifier
  FunctionQualifier           Qualifier;

  /// The name of the function (as a symbol).
  symbol                      Name;

  /// The return type of the function.
  FullType                    ReturnType;

  /// A vector of function parameters.
  std::vector<FunctionParameter>  Parameters;
};

//===========================================================================
/// \brief An object which knows how to emit a list of declarations.
class EmitDeclarations {
  public:
    EmitDeclarations(LLVMContext* ctx);
    virtual ~EmitDeclarations();

    /// Emit a single firtree declaration.
    void emitDeclaration(firtreeExternalDeclaration decl);

    /// Emit a list of firtree declarations.
    void emitDeclarationList(
        GLS_Lst(firtreeExternalDeclaration) decl_list);

    /// Emit a function prototype.
    void emitPrototype(firtreeFunctionPrototype proto);

    /// Emit a function definition.
    void emitFunction(firtreeFunctionDefinition func);

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
    /// iterator pointing to the matched prototype into it. If no prototype
    /// is matched, matched_proto_iter_ptr has m_FuncTable.end() written
    /// into it.
    bool existsConflictingPrototype(const FunctionPrototype& proto,
        std::multimap<symbol, FunctionPrototype>::iterator*
          matched_proto_iter_ptr = NULL);

    /// Construct a FunctionPrototype struct from a 
    /// firtreeFunctionPrototype parse-tree term.
    void constructPrototypeStruct(FunctionPrototype& to_construct,
        firtreeFunctionPrototype proto);

  private:
    /// The current LLVM context.
    LLVMContext*      m_Context;
      
    // A multimap between function identifier (as a symbol) and it's
    // prototype. We use a multimap because we can have overloaded functions
    // in Firtree (although we keep quiet about it!).
    std::multimap<symbol, FunctionPrototype>  m_FuncTable;
};

} // namespace Firtree

#endif // __LLVM_EMIT_DECL_H 

// vim:sw=2:ts=2:cindent:et 
