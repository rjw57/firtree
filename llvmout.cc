#include "llvmout.h"
#include "llvmout_priv.h"
#include "llvmutil.h"

#include "stdosx.h"  // General Definitions (for gcc)
#include "ptm_gen.h" // General Parsing Routines
#include "ptm_pp.h"  // Pretty Printer
#include "gls.h"     // General Language Services
#include "hmap.h"    // Datatype: Finite Maps
#include "symbols.h" // Datatype: Symbols

#include "llvm/Module.h"
#include "llvm/DerivedTypes.h"
#include "llvm/Constants.h"
#include "llvm/Instructions.h"
#include "llvm/Bitcode/ReaderWriter.h"

#include "gen/firtree_int.h" // grammar interface
#include "gen/firtree_lim.h" // scanner table
#include "gen/firtree_pim.h" // parser  table

#include <iostream>
#include <assert.h>

#include <map>
#include <stack>

using namespace llvm;

void printIndent(int indent)
{
  int i;
  for(i=0; i<indent; i++) { printf(" "); }
}

void printFuncProto(firtreeFunctionPrototype proto);
void printExpressionList(int indent, GLS_Lst(firtreeExpression) exprs);
void printExpression(int indent, firtreeExpression expr);

void printTypeQualifier(firtreeTypeQualifier qual)
{
}

void printParameterQualifier(firtreeParameterQualifier qual)
{
# define PARAM_QUAL(name) if(firtreeParameterQualifier_##name(qual)) { \
    printf("%s", #name); return; }

  PARAM_QUAL(in)
  PARAM_QUAL(out)
  PARAM_QUAL(inout)

  /* else */ { printf("/* unknown parameter qualifier */"); }  
}

void printTypeSpecifier(firtreeTypeSpecifier spec)
{
# define TYPE_SPEC(name) if(firtreeTypeSpecifier_##name(spec)) { \
    printf("%s", #name); return; }

  TYPE_SPEC(vec4)
  TYPE_SPEC(vec3)
  TYPE_SPEC(vec2)
  TYPE_SPEC(color)
  TYPE_SPEC(sampler)
  TYPE_SPEC(int)
  TYPE_SPEC(bool)
  TYPE_SPEC(float)
  TYPE_SPEC(void)

  /* else */ { printf("/* unknown type specifier */"); }
}

void printFullySpecifiedType(firtreeFullySpecifiedType type)
{
  firtreeTypeQualifier qual;
  firtreeTypeSpecifier spec;

  if(firtreeFullySpecifiedType_qualifiedtype(type, &qual, &spec))
  {
    printTypeQualifier(qual);
    printTypeSpecifier(spec);
  } else if(firtreeFullySpecifiedType_unqualifiedtype(type, &spec))
  {
    printTypeSpecifier(spec);
  } else {
    printf("/* unknwon type */");
  }
}

void printSingleDeclaration(int indent, firtreeSingleDeclaration decl)
{
  GLS_Tok name;
  firtreeExpression expr;

  if(firtreeSingleDeclaration_variableinitializer(decl, &name, &expr))
  {
    // Check special case of nop
    if(firtreeExpression_nop(expr))
    {
      printf("\"%s\"", GLS_Tok_string(name));
    } else {
      printf("\"%s\" := ", GLS_Tok_string(name));
      printExpression(indent, expr);
    }
    return;
  }

  printf("/* unknown dingle declaration */");
}

void printExpression(int indent, firtreeExpression expr)
{
}

void printExpressionList(int indent, GLS_Lst(firtreeExpression) exprs)
{
  GLS_Lst(firtreeExpression) tail;
  GLS_FORALL(tail, exprs) {
    firtreeExpression e = GLS_FIRST(firtreeExpression, tail);
    printIndent(indent+2); printExpression(indent+2,e); printf("\n");
  }
}

//==========================================================================
// Emit LLVM code for a function definition
void emitFunction(llvm_context* ctx, firtreeFunctionDefinition def)
{
  firtreeFunctionPrototype proto;
  GLS_Lst(firtreeExpression) expressions;

  if(!firtreeFunctionDefinition_functiondefinition(def, &proto, &expressions))
  {
    PANIC("unknown function definition");
    return;
  }

  // Create a new scope for the function
  push_scope(ctx);

  // Extract prototype.
  firtreeFunctionQualifier qual;
  firtreeFullySpecifiedType return_type;
  GLS_Tok name;
  GLS_Lst(firtreeParameterDeclaration) params;

  if(!firtreeFunctionPrototype_functionprototype(proto, &qual, &return_type, &name, &params))
  {
    PANIC("unknown function prototype");
  }

  // Extract the name of the function from this prototype.
  std::string func_name(GLS_Tok_string(name));
  cerr << "I: Define function '" << func_name << "'.\n";

  // Firstly, check to see if we already have a declaration for this
  // function.
  firtreeFunctionPrototype existing_prototype;
  if(!haveFunctionDeclared(ctx, proto, &existing_prototype))
  {
    // If not, declare it
    declareFunction(ctx, proto);
  } else {
    // Otherwise, check compatibility
    firtreeFullySpecifiedType existing_return_type;
    if(!firtreeFunctionPrototype_functionprototype(existing_prototype, 
          NULL, &existing_return_type, NULL, NULL))
    {
      PANIC("unknown function prototype");
    }
    if(!typesAreEqual(existing_return_type, return_type))
    {
      cerr << "E: Return types differ between prototype and definition.\n";
      PANIC("return types differ.");
    }
  }

  // Now we've done the housekeeping, create a LLVM function for
  // this function.
  
  // Firstly, create a vector of the parameter types
  std::vector<const Type*> param_llvm_types;
  GLS_Lst(firtreeParameterDeclaration) params_tail;
  GLS_FORALL(params_tail, params) {
    firtreeParameterDeclaration param_decl = 
      GLS_FIRST(firtreeParameterDeclaration, params_tail);

    firtreeTypeQualifier qual;
    firtreeParameterQualifier param_qual;
    firtreeTypeSpecifier spec;
    if(!firtreeParameterDeclaration_parameterdeclaration(param_decl, &qual, &param_qual, 
          &spec, NULL))
    {
      PANIC("unknown param decl.");
    }

    const Type* parm_type = getLLVMTypeForTerm(qual, spec);

    // 'out' types pass by reference.
    if(!firtreeParameterQualifier_in(param_qual))
    {
      parm_type = PointerType::get(parm_type, 0);
    }

    param_llvm_types.push_back(parm_type);
  }

  FunctionType *FT = FunctionType::get(getLLVMTypeForTerm(return_type),
      param_llvm_types, false);
  Function *F = NULL;

  if(firtreeFunctionQualifier_kernel(qual)) {
    F = new Function(FT, Function::ExternalLinkage, func_name.c_str(), ctx->M);
  } else if(firtreeFunctionQualifier_function(qual)) {
    F = new Function(FT, Function::InternalLinkage, func_name.c_str(), ctx->M);
  } else {
    PANIC("unknown function qualifier");
  }

  // Record this new function in the llvm function table which associates
  // prototypes -> llvm functions.
  ctx->llvm_func_table.insert(std::pair<firtreeFunctionPrototype, Function*>(proto, F));

  ctx->F = F;
  
  // Create a basic block for this list
  BasicBlock *BB = new BasicBlock("entry", ctx->F);
  ctx->BB = BB;

  emitExpressionList(ctx, expressions);

  // Create a default return inst if there was no terminator.
  if(ctx->BB->getTerminator() == NULL)
  {
    // FIXME: Check function return type is void.
    new ReturnInst(NULL, ctx->BB);
  }

  ctx->BB = NULL;
  ctx->F = NULL;

  // Pop the function scope.
  pop_scope(ctx);
}

void printFunctionQualifier(firtreeFunctionQualifier qual)
{
  if(firtreeFunctionQualifier_function(qual)) {
    printf("function");
  } else if(firtreeFunctionQualifier_kernel(qual)) {
    printf("kernel");
  } else {
    printf("/* unknown function qualifier */");
  }
}

void printParameterDeclaration(int indent, firtreeParameterDeclaration param)
{
  firtreeTypeQualifier typequal;
  firtreeParameterQualifier paramqual;
  firtreeTypeSpecifier typespec;
  firtreeParameterIdentifierOpt identifier;
  GLS_Tok name;

  if(!firtreeParameterDeclaration_parameterdeclaration(param, &typequal, &paramqual, &typespec, &identifier))
  {
    printf("/* unknown parameter declaration */");
    return;
  }

  printf("parameter( ");
  printTypeQualifier(typequal); printf(" , ");
  printParameterQualifier(paramqual); printf(" , ");
  printTypeSpecifier(typespec); printf(" , ");

  if(firtreeParameterIdentifierOpt_anonymous(identifier))
  {
    printf("anonymous");
  } else if(firtreeParameterIdentifierOpt_named(identifier, &name)) {
    printf("\"%s\"", GLS_Tok_string(name));
  } else {
    printf("/* unknown parameter identifier */");
  }

  //printIndent(indent); 
  printf(" )");
}

void printParameterDeclarationList(int indent, GLS_Lst(firtreeParameterDeclaration) params)
{
  GLS_Lst(firtreeParameterDeclaration) tail;
  GLS_FORALL(tail, params) {
    firtreeParameterDeclaration p = GLS_FIRST(firtreeParameterDeclaration, tail);
    printIndent(indent+2); printParameterDeclaration(indent+2,p); printf("\n");
  }
}

void printFuncProto(firtreeFunctionPrototype proto)
{
  firtreeFunctionQualifier qual;
  firtreeFullySpecifiedType type;
  GLS_Tok name;
  GLS_Lst(firtreeParameterDeclaration) params;

  if(!firtreeFunctionPrototype_functionprototype(proto, &qual, &type, &name, &params))
  {
    printf("/* unknown function prototype */");
  }

  printf("\"%s\"\n", GLS_Tok_string(name));
  
  printIndent(2); printFunctionQualifier(qual); printf(" /* Function kind */\n");
  printIndent(2); printFullySpecifiedType(type); printf(" /* Return type */\n");
  printIndent(2); printf("parameters(\n");
  printParameterDeclarationList(2, params);
  printIndent(2); printf(")\n");
}

//==========================================================================
// Create a LLVM module and emit code for each function defined within it.
void emitDecl(llvm_context* ctx, firtreeExternalDeclaration decl)
{
  firtreeFunctionDefinition func_def;
  firtreeFunctionPrototype func_proto;

  if(firtreeExternalDeclaration_definefuntion(decl, &func_def)) 
  {
    // Emit a definition for this function
    emitFunction(ctx, func_def);
  } else if(firtreeExternalDeclaration_declarefunction(decl, &func_proto)) {
    // Record this prototype in the function table
    declareFunction(ctx, func_proto);
  } else {
    PANIC("unknown declaration type");
  }
}

void emitDeclList(GLS_Lst(firtreeExternalDeclaration) decls)
{
  llvm_context ctx;

  // Create the LLVM module...
  ctx.M = new Module("firtree_test");

  firtreeExternalDeclaration dit;
  GLS_FORALL(dit, decls) {
    firtreeExternalDeclaration decl = GLS_FIRST(firtreeExternalDeclaration,dit);
    emitDecl(&ctx, decl);
  }

  // Output the bitcode file to stdout
  WriteBitcodeToFile(ctx.M, std::cout);

  // Delete the module and all of its contents.
  delete ctx.M;
}

/* vim:sw=2:ts=2:cindent:et 
 */
