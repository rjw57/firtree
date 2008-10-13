#include "stdosx.h"  // General Definitions (for gcc)
#include "ptm_gen.h" // General Parsing Routines
#include "ptm_pp.h"  // Pretty Printer
#include "gls.h"     // General Language Services
#include "hmap.h"    // Datatype: Finite Maps
#include "symbols.h" // Datatype: Symbols

#include "llvm/Module.h"
#include "llvm/Bitcode/ReaderWriter.h"

#include "gen/firtree_int.h" // grammar interface
#include "gen/firtree_lim.h" // scanner table
#include "gen/firtree_pim.h" // parser  table

#include "llvmout.h"

#include <iostream>

using namespace llvm;

struct llvm_context {
  Module*       the_module;
};

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
# define TYPE_QUAL(name) if(firtreeTypeQualifier_##name(qual)) { \
    printf("%s", #name); return; }

  TYPE_QUAL(const)
  TYPE_QUAL(none)

  /* else */ { printf("/* unknown type qualifier */"); }  
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
  firtreeExpression condition, left, right;
  GLS_Tok left_tok;
  GLS_Lst(firtreeExpression) rest;
  int found_expr = 0;
  char* func_name = NULL;
  firtreeFullySpecifiedType type;
  firtreeSingleDeclaration declaration;
  GLS_Lst(firtreeSingleDeclaration) rest_declarations;
  firtreeFunctionSpecifier func_spec;

  // nop
  if(firtreeExpression_nop(expr))
  {
    printf("/* nop */");
    return;
  }

  // Multiple expression
  if(firtreeExpression_expression(expr, &left, &rest))
  {
    GLS_Lst(firtreeExpression) tail;
    printExpression(indent, left);
    GLS_FORALL(tail, rest) {
      firtreeExpression e = GLS_FIRST(firtreeExpression, tail);
      printf(", "); printExpression(indent, e);
    }
    return;
  }

  // Binary expr.
# define BIN_OP(name) if(firtreeExpression_##name(expr, &left, &right)) { \
    found_expr = 1; func_name = #name; \
  } else

  BIN_OP(greater)
  BIN_OP(greaterequal)
  BIN_OP(less)
  BIN_OP(lessequal)
  BIN_OP(equal)
  BIN_OP(notequal)
  BIN_OP(add)
  BIN_OP(sub)
  BIN_OP(mul)
  BIN_OP(div)
  BIN_OP(assign)
  BIN_OP(addassign)
  BIN_OP(subassign)
  BIN_OP(mulassign)
  BIN_OP(divassign)
  BIN_OP(logicalor)
  BIN_OP(logicaland)
  BIN_OP(logicalxor)
  /* else */ { found_expr = 0; }
  
  if(found_expr) {
    printf("%s(\n", func_name);
    printIndent(indent+2); printExpression(indent+2, left); printf("\n");
    printIndent(indent+2); printExpression(indent+2, right); printf("\n");
    printIndent(indent); printf(")");
    return;
  }

  // Unary expr.
# define UN_OP(name) if(firtreeExpression_##name(expr, &left)) { \
    found_expr = 1; func_name = #name; \
  } else

  UN_OP(negate)
  UN_OP(inc)
  UN_OP(dec)
  UN_OP(postinc)
  UN_OP(postdec)
  UN_OP(return)
  /* else */ { found_expr = 0; }
  
  if(found_expr) {
    printf("%s( ", func_name);
    printExpression(indent, left);
    printf(" )");
    return;
  }

  // Primitive expressions
# define PRIMITIVE(name) if(firtreeExpression_##name(expr, &left_tok)) { \
    found_expr = 1; func_name = #name; \
  } else

  PRIMITIVE(variablenamed)
  PRIMITIVE(int)
  PRIMITIVE(bool)
  PRIMITIVE(float)
  /* else */ { found_expr = 0; }

  if(found_expr) {
    printf("%s( \"%s\" )", func_name, GLS_Tok_string(left_tok));
    return;
  }

  // Ternary expr.
# define TERN_OP(name) if(firtreeExpression_##name(expr, &condition, &left, &right)) { \
    found_expr = 1; func_name = #name; \
  } else
 
  TERN_OP(ternary)
  /* else */ { found_expr = 0; }

  if(found_expr) {
    printf("%s( ", func_name);
    printIndent(indent+2); printExpression(indent+2, condition); printf("\n");
    printIndent(indent+2); printExpression(indent+2, left); printf("\n");
    printIndent(indent+2); printExpression(indent+2, right); printf("\n");
    printIndent(indent); printf(")");
    return;
  }

  // fieldselect
  if(firtreeExpression_fieldselect(expr, &left, &left_tok))
  {
    printf("fieldselect( ");
    printExpression(indent, left);
    printf(" , \"%s\" )", GLS_Tok_string(left_tok));
    return;
  }

  // compound
  if(firtreeExpression_compound(expr, &rest))
  {
    printf("compound(\n");
    printExpressionList(indent, rest);
    printIndent(indent); printf(")");
    return;
  }

  // initdeclaratorlist
  if(firtreeExpression_initdeclaratorlist(expr, &type, &declaration, &rest_declarations))
  {
    GLS_Lst(firtreeSingleDeclaration) decl_tail;

    printf("declare( ");
    printFullySpecifiedType(type);
    printf("\n");

    printIndent(indent+2); 
    printSingleDeclaration(indent+2, declaration);
    printf("\n");

    GLS_FORALL(decl_tail, rest_declarations) {
      firtreeSingleDeclaration d = GLS_FIRST(firtreeSingleDeclaration, decl_tail);
      printIndent(indent+2); 
      printSingleDeclaration(indent+2, d);
      printf("\n");
    }

    printIndent(indent); printf(")");
    return;
  }

  // functioncall
  if(firtreeExpression_functioncall(expr, &func_spec, &rest))
  {
    firtreeTypeSpecifier type_spec;
    GLS_Lst(firtreeExpression) tail;

    printf("functioncall( ");

    if(firtreeFunctionSpecifier_constructorfor(func_spec, &type_spec))
    {
      printf("\"");
      printTypeSpecifier(type_spec);
      printf("\"");
    } else if(firtreeFunctionSpecifier_functionnamed(func_spec, &left_tok)) {
      printf("\"%s\"", GLS_Tok_string(left_tok));
    } else {
      printf("/* unknown func specifier */");
    }

    // Now see if there are any parameters
    if(GLS_EMPTY(rest))
    {
      printf(" )");
      return;
    }

    printf("\n");
    GLS_FORALL(tail, rest) {
      firtreeExpression e = GLS_FIRST(firtreeExpression, tail);
      printIndent(indent+2); printExpression(indent+2, e); printf("\n");
    }
    printIndent(indent); printf(")");

    return;
  }

  // for
  {
    firtreeExpression init, cond, iter, body;
    if(firtreeExpression_for(expr, &init, &cond, &iter, &body))
    {
      printf("for(\n");
      printIndent(indent+2); printExpression(indent+2, init); printf(" /* initializer */\n");
      printIndent(indent+2); printExpression(indent+2, cond); printf(" /* loop cond. */\n");
      printIndent(indent+2); printExpression(indent+2, iter); printf(" /* loop iter. */\n");
      printIndent(indent+2); printExpression(indent+2, body); printf("\n");
      printIndent(indent); printf(")");
      return;
    }
  }

  // selection
  {
    firtreeExpression cond, thenblock, elseblock;
    if(firtreeExpression_selection(expr, &cond, &thenblock, &elseblock))
    {
      printf("if(\n");
      printIndent(indent+2); printExpression(indent+2, cond); printf(" /* condition */\n");
      printIndent(indent+2); printExpression(indent+2, thenblock); printf(" /* then */\n");
      printIndent(indent+2); printExpression(indent+2, elseblock); printf(" /* else */\n");
      printIndent(indent); printf(")");
      return;
    }
  }

  // while
  if(firtreeExpression_while(expr, &left, &right))
  {
    printf("loop_precond(\n");
    printIndent(indent+2); printExpression(indent+2, left); printf(" /* condition */\n");
    printIndent(indent+2); printExpression(indent+2, right); printf("\n");
    printIndent(indent); printf(")");
    return;
  }

  // do
  if(firtreeExpression_do(expr, &left, &right))
  {
    printf("loop_postcond(\n");
    printIndent(indent+2); printExpression(indent+2, right); printf(" /* condition */\n");
    printIndent(indent+2); printExpression(indent+2, left); printf("\n");
    printIndent(indent); printf(")");
    return;
  }

  printf("/* unknown expr */");
}

void printExpressionList(int indent, GLS_Lst(firtreeExpression) exprs)
{
  GLS_Lst(firtreeExpression) tail;
  GLS_FORALL(tail, exprs) {
    firtreeExpression e = GLS_FIRST(firtreeExpression, tail);
    printIndent(indent+2); printExpression(indent+2,e); printf("\n");
  }
}

void printFuncDef(firtreeFunctionDefinition def)
{
  firtreeFunctionPrototype proto;
  GLS_Lst(firtreeExpression) expressions;

  if(!firtreeFunctionDefinition_functiondefinition(def, &proto, &expressions))
  {
    printf("/* unknown function definition */\n\n");
    return;
  }

  printf("definefunction( ");
  printFuncProto(proto); printf("\n");
  printExpressionList(0, expressions);
  printf(")\n\n");
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

void printDecl(firtreeExternalDeclaration decl)
{
  firtreeFunctionDefinition func_def;
  firtreeFunctionPrototype func_proto;

  if(firtreeExternalDeclaration_definefuntion(decl, &func_def)) 
  {
    printFuncDef(func_def); 
  } else if(firtreeExternalDeclaration_declarefunction(decl, &func_proto)) {
    printf("declarefunction( ");
    printFuncProto(func_proto); 
    printf(")\n\n"); 
  } else {
    printf("Argh!\n");
  }
}

void emitDeclList(GLS_Lst(firtreeExternalDeclaration) decls)
{
  // Create the LLVM module...
  Module *M = new Module("firtree_test");

  firtreeExternalDeclaration dit;
  GLS_FORALL(dit, decls) {
    firtreeExternalDeclaration decl = GLS_FIRST(firtreeExternalDeclaration,dit);
    //printDecl(decl);
  }

  // Output the bitcode file to stdout
  WriteBitcodeToFile(M, std::cout);

  // Delete the module and all of its contents.
  delete M;
}

/* vim:sw=2:ts=2:cindent:et 
 */
