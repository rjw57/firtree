#include "llvmout.h"

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

#define PANIC(msg) do { assert(false && (msg)); } while(0)

using namespace llvm;

struct llvm_context {
  Module*       M;  //< current module.
  Function*     F;  //< current function.
  BasicBlock*   BB; //< current block.
  std::stack<Value*>  val_stack;  //< value stack.

  // A multimap between function identifier and it's prototype. We use
  // a multimap because we can have overloaded functions in Firtree 
  // (although we keep quiet about it!).
  std::multimap<std::string, firtreeFunctionPrototype>  func_table;

  // A map between function prototype and the LLVM function.
  std::map<firtreeFunctionPrototype, Function*>         llvm_func_table;
};

bool haveFunctionDeclared(llvm_context* ctx, firtreeFunctionPrototype proto,
    firtreeFunctionPrototype* existing_proto = NULL);
void declareFunction(llvm_context* ctx, firtreeFunctionPrototype proto);

bool typesAreEqual(firtreeFullySpecifiedType a, firtreeFullySpecifiedType b);

const Type* getLLVMTypeForTerm(firtreeFullySpecifiedType type);
const Type* getLLVMTypeForTerm(firtreeTypeQualifier qual, firtreeTypeSpecifier spec);

void emitExpressionList(llvm_context* ctx, GLS_Lst(firtreeExpression) exprs);

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
}

void emitExpression(llvm_context* ctx, firtreeExpression expr)
{
  cerr << "exp: " << symbolToString(PT_product(expr)) << "\n";

  // simple case, nop
  if(firtreeExpression_nop(expr))
  {
    return;
  }

  firtreeExpression condition, left, right;
  GLS_Tok left_tok;
  GLS_Lst(firtreeExpression) rest;
  int found_expr = 0;
  char* func_name = NULL;
  firtreeFullySpecifiedType type;
  firtreeSingleDeclaration declaration;
  GLS_Lst(firtreeSingleDeclaration) rest_declarations;
  firtreeFunctionSpecifier func_spec;

  // Multiple expression
  if(firtreeExpression_expression(expr, &left, &rest))
  {
    emitExpression(ctx, left);
    GLS_Lst(firtreeExpression) tail;
    GLS_FORALL(tail, rest) {
      firtreeExpression e = GLS_FIRST(firtreeExpression, tail);
      emitExpression(ctx, e);
    }
    return;
  }

  // Binary expr.
# define BIN_OP(name) if(firtreeExpression_##name(expr, &left, &right)) { \
    found_expr = 1; func_name = #name; \
  } else

  BIN_OP(assign)
  BIN_OP(addassign)
  BIN_OP(subassign)
  BIN_OP(mulassign)
  BIN_OP(divassign)

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
  BIN_OP(logicalor)
  BIN_OP(logicaland)
  BIN_OP(logicalxor)
  /* else */ { found_expr = 0; }
  
  if(found_expr) {
    // Emit the left then right
    emitExpression(ctx, left);
    emitExpression(ctx, right);

    Value* right_val = ctx->val_stack.top();
    ctx->val_stack.pop();
    Value* left_val = ctx->val_stack.top();
    ctx->val_stack.pop();

    if(firtreeExpression_add(expr, NULL, NULL)) {
      Instruction* instr = BinaryOperator::create(Instruction::Add, 
          left_val, right_val, "tmpadd", ctx->BB);
      ctx->val_stack.push(instr);
    }
    if(firtreeExpression_sub(expr, NULL, NULL)) {
      Instruction* instr = BinaryOperator::create(Instruction::Sub, 
          left_val, right_val, "tmpsub", ctx->BB);
      ctx->val_stack.push(instr);
    }
    if(firtreeExpression_mul(expr, NULL, NULL)) {
      Instruction* instr = BinaryOperator::create(Instruction::Mul, 
          left_val, right_val, "tmpmul", ctx->BB);
      ctx->val_stack.push(instr);
    }
    if(firtreeExpression_div(expr, NULL, NULL)) {
      Instruction* instr = BinaryOperator::create(Instruction::FDiv, 
          left_val, right_val, "tmpdiv", ctx->BB);
      ctx->val_stack.push(instr);
    }
    if(firtreeExpression_logicaland(expr, NULL, NULL)) {
      Instruction* instr = BinaryOperator::create(Instruction::And, 
          left_val, right_val, "tmpand", ctx->BB);
      ctx->val_stack.push(instr);
    }
    if(firtreeExpression_logicalor(expr, NULL, NULL)) {
      Instruction* instr = BinaryOperator::create(Instruction::Or, 
          left_val, right_val, "tmpor", ctx->BB);
      ctx->val_stack.push(instr);
    }
    if(firtreeExpression_logicalxor(expr, NULL, NULL)) {
      Instruction* instr = BinaryOperator::create(Instruction::Xor, 
          left_val, right_val, "tmpxor", ctx->BB);
      ctx->val_stack.push(instr);
    }

    // printf("%s(\n", func_name);
    // printIndent(indent+2); // printExpression(indent+2, left); // printf("\n");
    // printIndent(indent+2); // printExpression(indent+2, right); // printf("\n");
    // printIndent(indent); // printf(")");
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
  /* else */ { found_expr = 0; }
  
  if(found_expr) {
    // printf("%s( ", func_name);
    // printExpression(indent, left);
    // printf(" )");
    return;
  }

  if(firtreeExpression_return(expr, &left))
  {
    // Emit the expression for the return value
    emitExpression(ctx, left);

    // If ret val is empty (nop), just return
    Value* retVal;
    if(!firtreeExpression_nop(left))
    {
      retVal = ctx->val_stack.top();
      ctx->val_stack.pop();
    }

    // Create the new instruction.
    new ReturnInst(retVal, ctx->BB);
    return;
  }

  // Primitive expressions
# define PRIMITIVE(name) if(firtreeExpression_##name(expr, &left_tok)) { \
    found_expr = 1; func_name = #name; \
  } else

  PRIMITIVE(variablenamed)
  /* else */ { found_expr = 0; }

  if(found_expr) {
    // printf("%s( \"%s\" )", func_name, GLS_Tok_string(left_tok));
    return;
  }

  // Primitive value types.
  if(firtreeExpression_int(expr, &left_tok))
  {
    int val = atoi(GLS_Tok_string(left_tok));
    ctx->val_stack.push(ConstantInt::get(Type::Int32Ty, val));
    return;
  }

  if(firtreeExpression_float(expr, &left_tok))
  {
    float val = atof(GLS_Tok_string(left_tok));
    ctx->val_stack.push(ConstantFP::get(Type::FloatTy, APFloat(val)));
    return;
  }
  
  if(firtreeExpression_bool(expr, &left_tok))
  {
    int val = 0;
    if(strcmp("true", GLS_Tok_string(left_tok)) == 0) 
    {
      val = 1;
    }
    ctx->val_stack.push(ConstantInt::get(Type::Int1Ty, val));
    return;
  }

  // Ternary expr.
# define TERN_OP(name) if(firtreeExpression_##name(expr, &condition, &left, &right)) { \
    found_expr = 1; func_name = #name; \
  } else
 
  TERN_OP(ternary)
  /* else */ { found_expr = 0; }

  if(found_expr) {
    // printf("%s( ", func_name);
    // printIndent(indent+2); // printExpression(indent+2, condition); // printf("\n");
    // printIndent(indent+2); // printExpression(indent+2, left); // printf("\n");
    // printIndent(indent+2); // printExpression(indent+2, right); // printf("\n");
    // printIndent(indent); // printf(")");
    return;
  }

  // fieldselect
  if(firtreeExpression_fieldselect(expr, &left, &left_tok))
  {
    // printf("fieldselect( ");
    // printExpression(indent, left);
    // printf(" , \"%s\" )", GLS_Tok_string(left_tok));
    return;
  }

  // compound
  if(firtreeExpression_compound(expr, &rest))
  {
    BasicBlock *BB = new BasicBlock("block", ctx->F);
    ctx->BB = BB;
    emitExpressionList(ctx, rest);
    return;
  }

  // initdeclaratorlist
  if(firtreeExpression_initdeclaratorlist(expr, &type, &declaration, &rest_declarations))
  {
    GLS_Lst(firtreeSingleDeclaration) decl_tail;

    // printf("declare( ");
    // printFullySpecifiedType(type);
    // printf("\n");

    // printIndent(indent+2); 
    // printSingleDeclaration(indent+2, declaration);
    // printf("\n");

    GLS_FORALL(decl_tail, rest_declarations) {
      firtreeSingleDeclaration d = GLS_FIRST(firtreeSingleDeclaration, decl_tail);
      // printIndent(indent+2); 
      // printSingleDeclaration(indent+2, d);
      // printf("\n");
    }

    // printIndent(indent); // printf(")");
    return;
  }

  // functioncall
  if(firtreeExpression_functioncall(expr, &func_spec, &rest))
  {
    firtreeTypeSpecifier type_spec;
    GLS_Lst(firtreeExpression) tail;

    // printf("functioncall( ");

    if(firtreeFunctionSpecifier_constructorfor(func_spec, &type_spec))
    {
      // printf("\"");
      // printTypeSpecifier(type_spec);
      // printf("\"");
    } else if(firtreeFunctionSpecifier_functionnamed(func_spec, &left_tok)) {
      // printf("\"%s\"", GLS_Tok_string(left_tok));
    } else {
      // printf("/* unknown func specifier */");
    }

    // Now see if there are any parameters
    if(GLS_EMPTY(rest))
    {
      // printf(" )");
      return;
    }

    // printf("\n");
    GLS_FORALL(tail, rest) {
      firtreeExpression e = GLS_FIRST(firtreeExpression, tail);
      // printIndent(indent+2); // printExpression(indent+2, e); // printf("\n");
    }
    // printIndent(indent); // printf(")");

    return;
  }

  // for
  {
    firtreeExpression init, cond, iter, body;
    if(firtreeExpression_for(expr, &init, &cond, &iter, &body))
    {
      // printf("for(\n");
      // printIndent(indent+2); // printExpression(indent+2, init); // printf(" /* initializer */\n");
      // printIndent(indent+2); // printExpression(indent+2, cond); // printf(" /* loop cond. */\n");
      // printIndent(indent+2); // printExpression(indent+2, iter); // printf(" /* loop iter. */\n");
      // printIndent(indent+2); // printExpression(indent+2, body); // printf("\n");
      // printIndent(indent); // printf(")");
      return;
    }
  }

  // selection
  {
    firtreeExpression cond, thenblock, elseblock;
    if(firtreeExpression_selection(expr, &cond, &thenblock, &elseblock))
    {
      // printf("if(\n");
      // printIndent(indent+2); // printExpression(indent+2, cond); // printf(" /* condition */\n");
      // printIndent(indent+2); // printExpression(indent+2, thenblock); // printf(" /* then */\n");
      // printIndent(indent+2); // printExpression(indent+2, elseblock); // printf(" /* else */\n");
      // printIndent(indent); // printf(")");
      return;
    }
  }

  // while
  if(firtreeExpression_while(expr, &left, &right))
  {
    // printf("loop_precond(\n");
    // printIndent(indent+2); // printExpression(indent+2, left); // printf(" /* condition */\n");
    // printIndent(indent+2); // printExpression(indent+2, right); // printf("\n");
    // printIndent(indent); // printf(")");
    return;
  }

  // do
  if(firtreeExpression_do(expr, &left, &right))
  {
    // printf("loop_postcond(\n");
    // printIndent(indent+2); // printExpression(indent+2, right); // printf(" /* condition */\n");
    // printIndent(indent+2); // printExpression(indent+2, left); // printf("\n");
    // printIndent(indent); // printf(")");
    return;
  }

  cerr << "unknown expr: " << symbolToString(PT_product(expr)) << "\n";
  PANIC("unknown expr");
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
// Emit a list of expressions.
void emitExpressionList(llvm_context* ctx, GLS_Lst(firtreeExpression) exprs)
{
  // HACK: push something on the value stack
  ctx->val_stack.push(ConstantInt::get(Type::Int32Ty, 2));

  GLS_Lst(firtreeExpression) tail;
  GLS_FORALL(tail, exprs) {
    firtreeExpression e = GLS_FIRST(firtreeExpression, tail);
    emitExpression(ctx, e);
  }
}

//==========================================================================
// Check if two types are equivalent.
bool typesAreEqual(firtreeFullySpecifiedType a, firtreeFullySpecifiedType b)
{
  firtreeTypeSpecifier a_spec, b_spec;
  firtreeTypeQualifier a_qual = NULL, b_qual = NULL;

  if(firtreeFullySpecifiedType_unqualifiedtype(a, &a_spec))
  { /* nop */ 
  } else if(firtreeFullySpecifiedType_qualifiedtype(a, &a_qual, &a_spec))
  { /* nop */
  } else {
    PANIC("unknown fully specified type.");
  }

  if(firtreeFullySpecifiedType_unqualifiedtype(b, &b_spec))
  { /* nop */ 
  } else if(firtreeFullySpecifiedType_qualifiedtype(b, &b_qual, &b_spec))
  { /* nop */
  } else {
    PANIC("unknown fully specified type.");
  }

  if(PT_product(a_spec) != PT_product(b_spec))
  {
    return false;
  }

  if((a_qual != NULL) && (b_qual != NULL))
  {
    if(PT_product(a_qual) != PT_product(b_qual))
    {
      return false;
    }
  }

  if((a_qual == NULL) && (b_qual != NULL) && !(firtreeTypeQualifier_none(b_qual)))
  {
    return false;
  }

  if((a_qual != NULL) && (b_qual == NULL) && !(firtreeTypeQualifier_none(a_qual)))
  {
    return false;
  }

  return true;
}

//==========================================================================
// Get a LLVM type corresponding to a particular firtree type.
const Type* getLLVMTypeForTerm(firtreeFullySpecifiedType type)
{
  firtreeTypeSpecifier spec;
  firtreeTypeQualifier qual = NULL;

  if(firtreeFullySpecifiedType_unqualifiedtype(type, &spec))
  { /* nop */ 
  } else if(firtreeFullySpecifiedType_qualifiedtype(type, &qual, &spec))
  { /* nop */
  } else {
    PANIC("unknown fully specified type.");
  }

  return getLLVMTypeForTerm(qual, spec);
}

const Type* getLLVMTypeForTerm(firtreeTypeQualifier qual, firtreeTypeSpecifier spec)
{
  if(firtreeTypeSpecifier_int(spec)) {
    return Type::Int32Ty;
  } else if(firtreeTypeSpecifier_void(spec)) {
    return Type::VoidTy;
  } else if(firtreeTypeSpecifier_bool(spec)) {
    return Type::Int1Ty;
  } else if(firtreeTypeSpecifier_float(spec)) {
    return Type::FloatTy;
  } else if(firtreeTypeSpecifier_sampler(spec)) {
    return Type::Int32Ty;
  } else if(firtreeTypeSpecifier_vec2(spec)) {
    return VectorType::get(Type::FloatTy, 2);
  } else if(firtreeTypeSpecifier_vec3(spec)) {
    return VectorType::get(Type::FloatTy, 3);
  } else if(firtreeTypeSpecifier_vec4(spec)) {
    return VectorType::get(Type::FloatTy, 4);
  } else if(firtreeTypeSpecifier_color(spec)) {
    return VectorType::get(Type::FloatTy, 4);
  } else {
    PANIC("unknown type");
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

  ctx->BB = NULL;
  ctx->F = NULL;
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
// Check whether two function prototypes 'clash', i.e. have the same name
// and same parameter types in same order.
bool prototypesCollide(firtreeFunctionPrototype a, firtreeFunctionPrototype b)
{
  // Extract info on both prototypes.
  firtreeFunctionQualifier a_qual;
  firtreeFullySpecifiedType a_type;
  GLS_Tok a_name;
  GLS_Lst(firtreeParameterDeclaration) a_params;

  if(!firtreeFunctionPrototype_functionprototype(a, &a_qual, &a_type, &a_name, &a_params))
  {
    PANIC("unknown function prototype");
  }

  firtreeFunctionQualifier b_qual;
  firtreeFullySpecifiedType b_type;
  GLS_Tok b_name;
  GLS_Lst(firtreeParameterDeclaration) b_params;

  if(!firtreeFunctionPrototype_functionprototype(b, &b_qual, &b_type, &b_name, &b_params))
  {
    PANIC("unknown function prototype");
  }

  // Simplest check, do they differ in name?
  if(strcmp(GLS_Tok_string(a_name), GLS_Tok_string(b_name)) != 0)
  {
    // They do.. no problem
    return false;
  }

  // Next check, do they differ in parameter count?
  if(GLS_LENGTH(a_params) != GLS_LENGTH(b_params))
  {
    return false;
  }

  // Now iterate through both parameter lists, looking for parameters which
  // differ.
  GLS_Lst(firtreeParameterDeclaration) a_tail = a_params, b_tail = b_params;

  while(!GLS_EMPTY(a_tail) && !GLS_EMPTY(b_tail))
  {
    firtreeParameterDeclaration a_param = GLS_FIRST(firtreeParameterDeclaration, a_tail);
    firtreeTypeQualifier a_type_qual;
    firtreeParameterQualifier a_param_qual;
    firtreeTypeSpecifier a_type_spec;
    firtreeParameterIdentifierOpt a_parm_ident;
    if(!firtreeParameterDeclaration_parameterdeclaration(a_param, &a_type_qual, &a_param_qual,
          &a_type_spec, &a_parm_ident))
    {
      PANIC("unknown parameter declaration");
    }

    firtreeParameterDeclaration b_param = GLS_FIRST(firtreeParameterDeclaration, b_tail);
    firtreeTypeQualifier b_type_qual;
    firtreeParameterQualifier b_param_qual;
    firtreeTypeSpecifier b_type_spec;
    firtreeParameterIdentifierOpt b_parm_ident;
    if(!firtreeParameterDeclaration_parameterdeclaration(b_param, &b_type_qual, &b_param_qual,
          &b_type_spec, &b_parm_ident))
    {
      PANIC("unknown parameter declaration");
    }

    // Check the product (vec4, vec3, etc) of the type specifiers to ensure they differ
    if(PT_product(a_type_spec) != PT_product(b_type_spec))
    {
      return false;
    }

    // FIXME: Do we allow differing type qualifiers? Prob not.

    a_tail = GLS_REST(firtreeParameterDeclaration, a_tail);
    b_tail = GLS_REST(firtreeParameterDeclaration, b_tail);
  }

  return true;
}

//==========================================================================
// Return true if a function matching this prototype has been declared
// previously.
bool haveFunctionDeclared(llvm_context* ctx, firtreeFunctionPrototype proto,
    firtreeFunctionPrototype* existing_proto)
{
  firtreeFunctionQualifier qual;
  firtreeFullySpecifiedType type;
  GLS_Tok name;
  GLS_Lst(firtreeParameterDeclaration) params;

  if(!firtreeFunctionPrototype_functionprototype(proto, &qual, &type, &name, &params))
  {
    PANIC("unknown function prototype");
  }

  // Extract the name of the function from this prototype.
  std::string func_name(GLS_Tok_string(name));

  // Do we have any existing functions by that name?
  if(ctx->func_table.count(func_name) != 0)
  {
    // Must check each function for collision.
    std::multimap<std::string, firtreeFunctionPrototype>::const_iterator possible_collisions_it =
        ctx->func_table.find(func_name);
    for( ; possible_collisions_it != ctx->func_table.end(); possible_collisions_it++)
    {
      if(prototypesCollide(proto, possible_collisions_it->second))
      {
        if(existing_proto != NULL)
        {
          *existing_proto = possible_collisions_it->second;
        }
        return true;
      }
    }
  }

  return false;
}

//==========================================================================
// Record a function prototype in the global function table.
void declareFunction(llvm_context* ctx, firtreeFunctionPrototype proto)
{
  firtreeFunctionQualifier qual;
  firtreeFullySpecifiedType type;
  GLS_Tok name;
  GLS_Lst(firtreeParameterDeclaration) params;

  if(!firtreeFunctionPrototype_functionprototype(proto, &qual, &type, &name, &params))
  {
    PANIC("unknown function prototype");
  }

  // Extract the name of the function from this prototype.
  std::string func_name(GLS_Tok_string(name));

  std::cerr << "I: Declare function '" << func_name << "'.\n";

  // Do we have any existing functions by that name?
  if(haveFunctionDeclared(ctx, proto))
  {
    std::cerr << "E: Function '" << func_name << "' already exists with clashing prototype.\n";
    PANIC("existing prototype");
  }

  // Ensure we have no void parameter types
  GLS_Lst(firtreeParameterDeclaration) params_tail;
  GLS_FORALL(params_tail, params) {
    firtreeParameterDeclaration param_decl = 
      GLS_FIRST(firtreeParameterDeclaration, params_tail);
  
    firtreeTypeSpecifier spec;
    if(!firtreeParameterDeclaration_parameterdeclaration(param_decl, NULL, NULL, &spec, NULL))
    {
      PANIC("unknown param decl.");
    }

    if(firtreeTypeSpecifier_void(spec))
    {
      cerr << "E: Parameters to function cannot have void type.\n";
      PANIC("cannot have void type params.");
    }
  }

  // Record prototype
  ctx->func_table.insert(std::pair<std::string const, firtreeFunctionPrototype>(
        func_name, proto));
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
