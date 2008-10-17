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
void EmitDeclarations::emitPrototype(firtreeFunctionPrototype proto)
{
}

//===========================================================================
/// Emit a function definition.
void EmitDeclarations::emitFunction(firtreeFunctionDefinition func)
{
}

} // namespace Firtree

// vim:sw=2:ts=2:cindent:et
