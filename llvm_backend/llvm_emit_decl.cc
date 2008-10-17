//===========================================================================
/// \file llvm_emit_decl.cc Implementation of Firtree::EmitDeclarations.

#include "llvm_backend.h"
#include "llvm_private.h"

#include <firtree/main.h>

#include "llvm_emit_decl.h"

using namespace llvm;

namespace Firtree {

//===========================================================================
EmitDeclarations::EmitDeclarations(LLVMContext* ctx)
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
void EmitDeclarations::emitDeclaration(firtreeExternalDeclaration decl)
{
  firtreeFunctionDefinition func_def;
  firtreeFunctionPrototype func_proto;

  // Decide if this declaration is a prototype or definition and
  // act accordingly.
  if(firtreeExternalDeclaration_definefuntion(decl, &func_def))
  {
    emitFunction(func_def);
  } else if(firtreeExternalDeclaration_declarefunction(decl, &func_proto)) {
    emitPrototype(func_proto);
  } else {
    FIRTREE_LLVM_ICE(m_Context, decl, "Unknown declaration node.");
  }
}

//===========================================================================
/// Emit a list of firtree declarations.
void EmitDeclarations::emitDeclarationList(
    GLS_Lst(firtreeExternalDeclaration) decl_list)
{
  GLS_Lst(firtreeExternalDeclaration) tail = decl_list;
  firtreeExternalDeclaration decl;

  GLS_FORALL(tail, decl_list) {
    decl = GLS_FIRST(firtreeExternalDeclaration, tail);

    // Wrap each external declaration in an exception handler so 
    // we can re-start from here.
    try {
      emitDeclaration(decl);
    } catch (CompileErrorException e) {
      m_Context->Backend->HandleCompilerError(e);
    }
  }
}

//===========================================================================
/// Emit a function prototype.
void EmitDeclarations::emitPrototype(firtreeFunctionPrototype proto_term)
{
  firtreeFunctionQualifier qual;
  firtreeFullySpecifiedType type;
  GLS_Tok name;
  GLS_Lst(firtreeParameterDeclaration) params;

  if(!firtreeFunctionPrototype_functionprototype(proto_term,
        &qual, &type, &name, &params))
  {
    FIRTREE_LLVM_ICE(m_Context, proto_term, "Invalid function prototype.");
  }

  // Form a FunctionPrototype structure for the function
  FunctionPrototype prototype;

  prototype.Term = proto_term;

  if(firtreeFunctionQualifier_function(qual)) {
    prototype.Qualifier = FunctionPrototype::FuncQualFunction;
  } else if(firtreeFunctionQualifier_kernel(qual)) {
    prototype.Qualifier = FunctionPrototype::FuncQualKernel;
  } else {
    FIRTREE_LLVM_ICE(m_Context, qual, "Invalid function qualifier.");
  }

  prototype.Name = name;
  if(prototype.Name == NULL) {
    FIRTREE_LLVM_ICE(m_Context, type, "Invalid name.");
  }

  prototype.ReturnType = FullType::FromFullySpecifiedType(type);
  if(!FullType::IsValid(prototype.ReturnType)) {
    FIRTREE_LLVM_ICE(m_Context, type, "Invalid type.");
  }

  GLS_Lst(firtreeParameterDeclaration) params_tail;
  GLS_FORALL(params_tail, params) {
    firtreeParameterDeclaration param_decl = 
      GLS_FIRST(firtreeParameterDeclaration, params_tail);

    firtreeTypeQualifier param_type_qual;
    firtreeParameterQualifier param_qual;
    firtreeTypeSpecifier param_type_spec;
    firtreeParameterIdentifierOpt param_identifier;

    if(firtreeParameterDeclaration_parameterdeclaration(param_decl,
          &param_type_qual, &param_qual, &param_type_spec,
          &param_identifier))
    {
    } else {
      FIRTREE_LLVM_ICE(m_Context, param_decl,
          "Invalid parameter declaration.");
    }
  }
}

//===========================================================================
/// Emit a function definition.
void EmitDeclarations::emitFunction(firtreeFunctionDefinition func)
{
}

} // namespace Firtree

// vim:sw=2:ts=2:cindent:et
