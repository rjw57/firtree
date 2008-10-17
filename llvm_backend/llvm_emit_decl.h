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
    FuncQualInvalid = -1,
  };

  /// The parse tree term which defined this prototype.
  firtreeFunctionPrototype    Term;

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

  private:
    /// The current LLVM context.
    LLVMContext*      m_Context;
      
    // A multimap between function identifier and it's prototype. We use
    // a multimap because we can have overloaded functions in Firtree 
    // (although we keep quiet about it!).
    std::multimap<std::string, FunctionPrototype>  func_table;

    // A map between function prototype and the LLVM function.
    std::map<FunctionPrototype, llvm::Function*>   llvm_func_table;
};

} // namespace Firtree

#endif // __LLVM_EMIT_DECL_H 

// vim:sw=2:ts=2:cindent:et 
