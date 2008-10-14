#include "llvmout.h"
#include "llvmout_priv.h"
#include "llvmutil.h"

#include "llvm/ADT/SmallVector.h"

#include "stdosx.h"  // General Definitions (for gcc)
#include "ptm_gen.h" // General Parsing Routines
#include "ptm_pp.h"  // Pretty Printer
#include "gls.h"     // General Language Services
#include "hmap.h"    // Datatype: Finite Maps
#include "symbols.h" // Datatype: Symbols

#include "gen/firtree_int.h" // grammar interface
#include "gen/firtree_lim.h" // scanner table
#include "gen/firtree_pim.h" // parser  table

#include <iostream>
#include <assert.h>

using namespace llvm;

//==========================================================================
// Emit a variable declaraion, recording it in the symbol table
void emitSingleDeclaration(llvm_context* ctx, 
    firtreeTypeSpecifier spec, firtreeTypeQualifier qual,
    firtreeSingleDeclaration decl)
{
  GLS_Tok name;
  firtreeExpression expr;

  if(firtreeSingleDeclaration_variableinitializer(decl, &name, &expr))
  {
    // Check for conflicts
    const variable& existing_var = find_symbol(ctx,
        stringToSymbol(GLS_Tok_string(name)));
    if(is_valid_variable(existing_var)) {
      cerr << "E: Variable '" << GLS_Tok_string(name) <<
        "' already exists.\n";
      PANIC("duplicated variable.");
    }

    // Create a new memory location to store the variable.
    AllocaInst* new_value = new AllocaInst(
        getLLVMTypeForTerm(qual, spec),
        GLS_Tok_string(name),
        ctx->BB);

    // Record the new variable
    variable new_variable = {
      decl, new_value, stringToSymbol(GLS_Tok_string(name)), spec, qual
    };
    declare_variable(ctx, new_variable);

    // Check special case of nop
    if(!firtreeExpression_nop(expr))
    {
      // Emit an expression for the initialiser
      emitExpression(ctx, expr);

      // Store this into our variable
      new StoreInst(pop_value(ctx), new_value, ctx->BB);
    }
    return;
  }

  PANIC("unknown single declaration");
}

//==========================================================================
// Emit a list of expressions.
void emitExpressionList(llvm_context* ctx, GLS_Lst(firtreeExpression) exprs)
{
  // HACK: push something on the value stack
  push_value(ctx, ConstantInt::get(Type::Int32Ty, 2));

  GLS_Lst(firtreeExpression) tail;
  GLS_FORALL(tail, exprs) {
    firtreeExpression e = GLS_FIRST(firtreeExpression, tail);
    emitExpression(ctx, e);
  }
}

//==========================================================================
// Emit an individual expression.
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
    found_expr = 1; \
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
  BIN_OP(logicalor)
  BIN_OP(logicaland)
  BIN_OP(logicalxor)
  /* else */ { found_expr = 0; }
  
  if(found_expr) {
    // Emit the left then right
    emitExpression(ctx, left);
    emitExpression(ctx, right);

    Value* right_val = pop_value(ctx);
    Value* left_val = pop_value(ctx);

    if(firtreeExpression_add(expr, NULL, NULL)) {
      Instruction* instr = BinaryOperator::create(Instruction::Add, 
          left_val, right_val, "tmpadd", ctx->BB);
      push_value(ctx, instr);
    }
    if(firtreeExpression_sub(expr, NULL, NULL)) {
      Instruction* instr = BinaryOperator::create(Instruction::Sub, 
          left_val, right_val, "tmpsub", ctx->BB);
      push_value(ctx, instr);
    }
    if(firtreeExpression_mul(expr, NULL, NULL)) {
      Instruction* instr = BinaryOperator::create(Instruction::Mul, 
          left_val, right_val, "tmpmul", ctx->BB);
      push_value(ctx, instr);
    }
    if(firtreeExpression_div(expr, NULL, NULL)) {
      Instruction* instr = BinaryOperator::create(Instruction::FDiv, 
          left_val, right_val, "tmpdiv", ctx->BB);
      push_value(ctx, instr);
    }
    if(firtreeExpression_logicaland(expr, NULL, NULL)) {
      Instruction* instr = BinaryOperator::create(Instruction::And, 
          left_val, right_val, "tmpand", ctx->BB);
      push_value(ctx, instr);
    }
    if(firtreeExpression_logicalor(expr, NULL, NULL)) {
      Instruction* instr = BinaryOperator::create(Instruction::Or, 
          left_val, right_val, "tmpor", ctx->BB);
      push_value(ctx, instr);
    }
    if(firtreeExpression_logicalxor(expr, NULL, NULL)) {
      Instruction* instr = BinaryOperator::create(Instruction::Xor, 
          left_val, right_val, "tmpxor", ctx->BB);
      push_value(ctx, instr);
    }

    return;
  }

  BIN_OP(assign)
  BIN_OP(addassign)
  BIN_OP(subassign)
  BIN_OP(mulassign)
  BIN_OP(divassign)
  /* else */ { found_expr = 0; }

  // Unary expr.
# define UN_OP(name) if(firtreeExpression_##name(expr, &left)) { \
    found_expr = 1; \
  } else
 
  if(found_expr) {
    // Emit the expression
    emitExpression(ctx, left);
    emitExpression(ctx, right);

    llvm::Value* right_val = pop_value(ctx);
    firtree_value left_val = pop_full_value(ctx);

    // check this is an lvalue.
    if(left_val.lvalue == NULL)
    {
      cerr << "E: Attempt to modify a non-lvalue.\n";
      PANIC("cannot modify non lvalue.");
    }

    if(firtreeExpression_assign(expr, NULL, NULL)) {
      new StoreInst(right_val, left_val.lvalue, ctx->BB);
    }
    if(firtreeExpression_addassign(expr, NULL, NULL)) {
      llvm::Value* new_val = BinaryOperator::create(Instruction::Add,
          left_val.value, right_val, "tmpadd", ctx->BB);
      new StoreInst(new_val, left_val.lvalue, ctx->BB);
    }
    if(firtreeExpression_subassign(expr, NULL, NULL)) {
      llvm::Value* new_val = BinaryOperator::create(Instruction::Sub,
          left_val.value, right_val, "tmpsub", ctx->BB);
      new StoreInst(new_val, left_val.lvalue, ctx->BB);
    }
    if(firtreeExpression_mulassign(expr, NULL, NULL)) {
      llvm::Value* new_val = BinaryOperator::create(Instruction::Mul,
          left_val.value, right_val, "tmpmul", ctx->BB);
      new StoreInst(new_val, left_val.lvalue, ctx->BB);
    }
    if(firtreeExpression_divassign(expr, NULL, NULL)) {
      llvm::Value* new_val = BinaryOperator::create(Instruction::FDiv,
          left_val.value, right_val, "tmpdiv", ctx->BB);
      new StoreInst(new_val, left_val.lvalue, ctx->BB);
    }
    
    // Push lvalue back on stack
    push_value(ctx, left_val.value, left_val.lvalue);

    return;
  }

  UN_OP(negate)
  UN_OP(inc)
  UN_OP(dec)
  UN_OP(postinc)
  UN_OP(postdec)
  /* else */ { found_expr = 0; }
  
  if(found_expr) {
    // Emit the expression
    emitExpression(ctx, left);
    firtree_value prev_val = pop_full_value(ctx);

    // Get an appropriately typed '1' and '-1'.
    // FIXME: make sure this is typed
    Value* one = ConstantFP::get(Type::FloatTy, APFloat(1.f));
    Value* negone = ConstantFP::get(Type::FloatTy, APFloat(-1.f));

    // Special case, negation
    if(firtreeExpression_negate(expr, NULL)) {
      push_value(ctx, BinaryOperator::create(Instruction::Mul,
          prev_val.value, negone, "tmpdec", ctx->BB));
      return;
    }

    // check this is an lvalue.
    if(prev_val.lvalue == NULL)
    {
      cerr << "E: Attempt to modify a non-lvalue.\n";
      PANIC("cannot modify non lvalue.");
    }

    Value* result = prev_val.value;

    if(firtreeExpression_inc(expr, NULL)) {
      result = BinaryOperator::create(Instruction::Add,
          prev_val.value, one, "tmpinc", ctx->BB);
    }
    if(firtreeExpression_dec(expr, NULL)) {
      result = BinaryOperator::create(Instruction::Sub,
          prev_val.value, one, "tmpdec", ctx->BB);
    }
    if(firtreeExpression_postinc(expr, NULL)) {
      BinaryOperator::create(Instruction::Add,
          prev_val.value, one, "tmpinc", ctx->BB);
    }
    if(firtreeExpression_postdec(expr, NULL)) {
      BinaryOperator::create(Instruction::Sub,
          prev_val.value, one, "tmpdec", ctx->BB);
    }

    // Store the result and push it on the stack
    new StoreInst(result, prev_val.lvalue, ctx->BB);
    push_value(ctx, result, prev_val.lvalue);

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
      retVal = pop_value(ctx);
    }

    // Create the new instruction.
    new ReturnInst(retVal, ctx->BB);
    return;
  }

  // Primitive expressions
  if(firtreeExpression_variablenamed(expr, &left_tok))
  {
    // Find the variable in the symbol table.
    const variable& existing_var = find_symbol(ctx,
        stringToSymbol(GLS_Tok_string(left_tok)));
    if(!is_valid_variable(existing_var))
    {
      cerr << "E: Unknown variable '" << GLS_Tok_string(left_tok) <<
        "'.\n";
      PANIC("unknown variable");
    }

    // Load the variable from memory
    LoadInst* value = new LoadInst(existing_var.value,
        "tmpvarload", ctx->BB);
    push_value(ctx, value, existing_var.value);
    
    return;
  }

  // Primitive value types.
  if(firtreeExpression_int(expr, &left_tok))
  {
    int val = atoi(GLS_Tok_string(left_tok));
    push_value(ctx, ConstantInt::get(Type::Int32Ty, val));
    return;
  }

  if(firtreeExpression_float(expr, &left_tok))
  {
    float val = atof(GLS_Tok_string(left_tok));
    push_value(ctx, ConstantFP::get(Type::FloatTy, 
          APFloat(static_cast<float>(val))));
    return;
  }
  
  if(firtreeExpression_bool(expr, &left_tok))
  {
    int val = 0;
    if(strcmp("true", GLS_Tok_string(left_tok)) == 0) 
    {
      val = 1;
    }
    push_value(ctx, ConstantInt::get(Type::Int1Ty, val));
    return;
  }

  // Ternary expr.
  if(firtreeExpression_ternary(expr, &condition, &left, &right)) {
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
    //BasicBlock *BB = new BasicBlock("block", ctx->F);
    //ctx->BB = BB;
    push_scope(ctx);
    emitExpressionList(ctx, rest);
    pop_scope(ctx);
    return;
  }

  // initdeclaratorlist
  if(firtreeExpression_initdeclaratorlist(expr, &type, &declaration, &rest_declarations))
  {
    GLS_Lst(firtreeSingleDeclaration) decl_tail;

    firtreeTypeSpecifier spec;
    firtreeTypeQualifier qual;
    crack_fully_specified_type(type, &spec, &qual);

    emitSingleDeclaration(ctx, spec, qual, declaration);

    GLS_FORALL(decl_tail, rest_declarations) {
      firtreeSingleDeclaration d = GLS_FIRST(firtreeSingleDeclaration, decl_tail);
      emitSingleDeclaration(ctx, spec, qual, d);
    }

    return;
  }

  // functioncall
  if(firtreeExpression_functioncall(expr, &func_spec, &rest))
  {
    firtreeTypeSpecifier type_spec;
    GLS_Lst(firtreeExpression) tail;

    // Emit each parameter
    //llvm::SmallVector<llvm::Value*> param_vec;
    llvm::SmallVector<llvm::Value*, 8> param_vec;
    GLS_FORALL(tail, rest) {
      firtreeExpression e = GLS_FIRST(firtreeExpression, tail);
      emitExpression(ctx, e);

      // FIXME: Handle in/out params
      param_vec.push_back(pop_value(ctx));
    }

    if(firtreeFunctionSpecifier_constructorfor(func_spec, &type_spec))
    {
      // printf("\"");
      // printTypeSpecifier(type_spec);
      // printf("\"");
    } else if(firtreeFunctionSpecifier_functionnamed(func_spec, &left_tok)) {
      if(ctx->func_table.count(GLS_Tok_string(left_tok)) == 0)
      {
        cerr << "E: Unknown function '" << GLS_Tok_string(left_tok) << "'\n";
        PANIC("unknown function");
      }

      // FIXME: Match prototypes.
      const firtreeFunctionPrototype proto = 
        ctx->func_table.find(GLS_Tok_string(left_tok))->second;

      llvm::Function* F = ctx->llvm_func_table.find(proto)->second;

      llvm::Value* retVal = new CallInst(
          F, param_vec.begin(), param_vec.end(), 
          GLS_Tok_string(left_tok), ctx->BB);
      push_value(ctx, retVal);
    } else {
      PANIC("unknown func specifier");
    }

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

/* vim:sw=2:ts=2:cindent:et 
 */
