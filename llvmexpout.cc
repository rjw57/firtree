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
// Attempt to cast the passed value to a float and return the new value.
Value* getAsFloat(llvm_context* ctx, Value* val)
{
  const Type* val_type = val->getType();

  // Simple case, val is already a float
  if(val_type->getTypeID() == Type::FloatTyID)
  {
    return val;
  }

  // If an integer, cast it
  if(isa<IntegerType>(val_type))
  {
    return new SIToFPInst(val, Type::FloatTy, "tmpcast", ctx->BB);
  }

  cerr << "E: Cannot cast value to a float.\n";
  PANIC("cast error.");
  return NULL;
}

//==========================================================================
// Attempt to cast the passed value to an integer and return the new value.
Value* getAsInt(llvm_context* ctx, Value* val, 
    const IntegerType* dest_type = Type::Int32Ty)
{
  const Type* val_type = val->getType();
  unsigned int size = dest_type->getBitWidth();

  // If an integer, check size
  if(isa<IntegerType>(val_type))
  {
    const IntegerType* val_int_type = cast<IntegerType>(val_type);

    if(val_int_type->getBitWidth() < size)
    { 
      return new SExtInst(val, dest_type, "tmpcast", ctx->BB);
    } else if(val_int_type->getBitWidth() > size)
    {
      // Wont truncate!
      cerr << "E: Cannot truncate integer type.\n";
      PANIC("truncation");
    } else {
      // No casting needed!
      return val;
    }
  }

  // If a float, cast
  if(val_type->getTypeID() == Type::FloatTyID)
  {
    return new FPToSIInst(val, dest_type, "tmpcast", ctx->BB);
  }

  cerr << "E: Cannot cast value to an int.\n";
  PANIC("cast error.");
  return NULL;
}

//==========================================================================
// Attempt to cast the passed value to a specified vector.
Value* getAsVector(llvm_context* ctx, Value* val, unsigned int size)
{
  const Type* val_type = val->getType();

  // Simple case, val is already a vector
  if(isa<VectorType>(val_type)) {
    const VectorType* val_vec = cast<VectorType>(val_type);

    // Check value is right size.
    if(val_vec->getNumElements() == size)
    {
      return val;
    }
  } else {
    // If the value isn't a vector, see if we can promote
    // it to a broadcast float.
    Value* float_val = getAsFloat(ctx, val);

    if(float_val != NULL) {
      static Constant* const zeros[] = {
        ConstantFP::get(Type::FloatTy, APFloat(0.f)),
        ConstantFP::get(Type::FloatTy, APFloat(0.f)),
        ConstantFP::get(Type::FloatTy, APFloat(0.f)),
        ConstantFP::get(Type::FloatTy, APFloat(0.f)),
      };
      Value* new_vec = ConstantVector::get(zeros, size);
      
      for(unsigned int i=0; i<size; i++)
      {
        new_vec = new InsertElementInst(new_vec, float_val, i, \
            "tmpins", ctx->BB);
      }

      return new_vec;
    }
  }

  cerr << "E: Cannot cast value to a vector.\n";
  PANIC("cast error.");
  return NULL;
}

//==========================================================================
// Attempt to cast the value passed to the specified type.
Value* getAsType(llvm_context* ctx, Value* val, const Type* type)
{
  if(type->getTypeID() == Type::FloatTyID)
  {
    // Floating point
    return getAsFloat(ctx, val);
  } else if(isa<IntegerType>(type)) {
    // Integer
    return getAsInt(ctx, val, cast<IntegerType>(type));
  } else if(isa<VectorType>(type)) {
    // Vector
    return getAsVector(ctx, val, cast<VectorType>(type)->getNumElements());
  }

  cerr << "E: Unknown type in cast operation.\n";
  PANIC("unknown type");

  return NULL;
}

//==========================================================================
// Return true if the types of left and right are identical.
bool typesMatch(llvm_context* ctx, Value* left, Value* right)
{
  // Get the types of left and right
  const Type* left_type = left->getType();
  const Type* right_type = right->getType();

  // Both sides are integers.
  if(isa<IntegerType>(left_type) && isa<IntegerType>(right_type))
  {
    const IntegerType* left_int = cast<IntegerType>(left_type);
    const IntegerType* right_int = cast<IntegerType>(right_type);

    if(left_int->getBitWidth() == right_int->getBitWidth()) {
      return true;
    }
  }

  if((left_type->getTypeID() == Type::FloatTyID) &&
      (right_type->getTypeID() == Type::FloatTyID)) 
  {
    return true;
  }

  if(isa<VectorType>(left_type) && isa<VectorType>(right_type))
  {
    const VectorType* left_vec = cast<VectorType>(left_type);
    const VectorType* right_vec = cast<VectorType>(right_type);

    if(left_vec->getNumElements() == right_vec->getNumElements())
    {
      return true;
    }
  }

  return false;
}

//==========================================================================
// Attempt to promote types of values either side of a binary operator so
// that the types of the values returned in new_left and new_right are the
// same.
void promoteTypes(llvm_context* ctx, Value* left, Value* right, 
    Value** new_left, Value** new_right)
{
  // Get the types of left and right
  const Type* left_type = left->getType();
  const Type* right_type = right->getType();

  // Default case - no change. If we just return from the function, we
  // now assert that no conversion need be done.
  *new_left = left; *new_right = right;

  // Both sides are integers.
  if(isa<IntegerType>(left_type) && isa<IntegerType>(right_type))
  {
    const IntegerType* left_int = cast<IntegerType>(left_type);
    const IntegerType* right_int = cast<IntegerType>(right_type);

    // Sign extend one or other of the sides if necessary.
    if(left_int->getBitWidth() > right_int->getBitWidth()) {
      *new_right = new SExtInst(right, left_type, "tmpcast", ctx->BB);
    } else if(left_int->getBitWidth() < right_int->getBitWidth()) {
      *new_left = new SExtInst(left, right_type, "tmpcast", ctx->BB);
    } else {
      /* nothing to do! */
    }

    return;
  }

  // Both sides are floats.
  if((left_type->getTypeID() == Type::FloatTyID) &&
      (right_type->getTypeID() == Type::FloatTyID)) 
  {
    /* it is fine. */
    return;
  }

  // Left is integer and right is float.
  if(isa<IntegerType>(left_type) && 
      (right_type->getTypeID() == Type::FloatTyID))
  {
    // Promote int->float
    *new_left = getAsFloat(ctx, left);
    return;
  }

  // Right is integer and left is float.
  if(isa<IntegerType>(right_type) && 
      (left_type->getTypeID() == Type::FloatTyID))
  {
    // Promote int->float
    *new_right = getAsFloat(ctx, right);
    return;
  }

  // Left is vector
  if(isa<VectorType>(left_type))
  {
    const VectorType* vtype = cast<VectorType>(left_type);
    *new_right = getAsVector(ctx, right, vtype->getNumElements());
    return;
  }

  // Right is vector
  if(isa<VectorType>(right_type))
  {
    const VectorType* vtype = cast<VectorType>(right_type);
    *new_left = getAsVector(ctx, left, vtype->getNumElements());
    return;
  }

  cerr << "E: Types are incompatible either side of binary operator.\n";
  PANIC("incompatible binary op types.");
}

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

    const Type* required_type = getLLVMTypeForTerm(qual, spec);

    // Create a new memory location to store the variable.
    AllocaInst* new_value = new AllocaInst(
        required_type,
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
      new StoreInst(getAsType(ctx, pop_value(ctx), required_type),
          new_value, ctx->BB);
    }
    return;
  }

  PANIC("unknown single declaration");
}

//==========================================================================
// Emit a swizzle operation
void emitSwizzle(llvm_context* ctx, firtreeExpression expr,
    GLS_Tok swizzle)
{
  const char* swizzle_string = GLS_Tok_string(swizzle);
  int swizzle_spec[] = {-1, -1, -1, -1};

  PANIC("FIXME");
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

    // Check if there is a terminator in the current block,
    // if so we've wncountered a return statement (or similar)
    // and can (must) stop.
    if(ctx->BB->getTerminator() != NULL)
    {
      return;
    }
  }
}

//==========================================================================
// Emit a binary arithmetic expression.
void emitBinaryArithmeticExpression(llvm_context* ctx, firtreeExpression expr,
    firtreeExpression left, firtreeExpression right)
{
  // Emit the left then right
  emitExpression(ctx, left);
  emitExpression(ctx, right);
  Value* right_val = pop_value(ctx);
  Value* left_val = pop_value(ctx);

  // Promote types if necessary
  promoteTypes(ctx, left_val, right_val, &left_val, &right_val);

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
    llvm::Value* instr = BinaryOperator::create(Instruction::FDiv,
        getAsFloat(ctx, left_val),
        getAsFloat(ctx, right_val),
        "tmpdiv", ctx->BB);
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
}

//==========================================================================
// Emit a binary assignment expression.
void emitBinaryAssignmentExpression(llvm_context* ctx, firtreeExpression expr,
    firtreeExpression left, firtreeExpression right)
{
  // Emit the expression
  emitExpression(ctx, left);
  emitExpression(ctx, right);

  llvm::Value* right_val = pop_value(ctx);
  firtree_value left_val = pop_full_value(ctx);

  // Attempt to promote the right-hand type
  right_val = getAsType(ctx, right_val, left_val.value->getType());

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
        getAsFloat(ctx, left_val.value),
        getAsFloat(ctx, right_val),
        "tmpdiv", ctx->BB);
    new StoreInst(new_val, left_val.lvalue, ctx->BB);
  }
  
  // Push lvalue back on stack
  push_value(ctx, left_val.value, left_val.lvalue);
}

//==========================================================================
// Emit a binary comparison expression.
void emitBinaryComparisonExpression(llvm_context* ctx, firtreeExpression expr,
    firtreeExpression left, firtreeExpression right)
{
  // Emit the left then right
  emitExpression(ctx, left);
  emitExpression(ctx, right);
  Value* right_val = pop_value(ctx);
  Value* left_val = pop_value(ctx);

  // Promote types if necessary
  promoteTypes(ctx, left_val, right_val, &left_val, &right_val);

  // Work out which comparison we want and set the predicate appropriately.
  // Note that each instruction (fcmp/icmp) have different predicates
  // for the same comparison.
  ICmpInst::Predicate icmp;
  FCmpInst::Predicate fcmp;

  if(firtreeExpression_greater(expr, NULL, NULL)) {
    icmp = ICmpInst::ICMP_SGT;
    fcmp = FCmpInst::FCMP_OGT;
  } else if(firtreeExpression_greaterequal(expr, NULL, NULL)) {
    icmp = ICmpInst::ICMP_SGE;
    fcmp = FCmpInst::FCMP_OGT;
  } else if(firtreeExpression_less(expr, NULL, NULL)) {
    icmp = ICmpInst::ICMP_SLT;
    fcmp = FCmpInst::FCMP_OLT;
  } else if(firtreeExpression_lessequal(expr, NULL, NULL)) {
    icmp = ICmpInst::ICMP_SLE;
    fcmp = FCmpInst::FCMP_OLE;
  } else if(firtreeExpression_equal(expr, NULL, NULL)) {
    icmp = ICmpInst::ICMP_EQ;
    fcmp = FCmpInst::FCMP_OEQ;
  } else if(firtreeExpression_notequal(expr, NULL, NULL)) {
    icmp = ICmpInst::ICMP_NE;
    fcmp = FCmpInst::FCMP_ONE;
  } else {
    cerr << "E: Unknown comparison operator.\n";
    PANIC("unknown operator.");
  }

  // NOTE: We can only handle integer and floating point comparisons
  
  const Type* left_type = left_val->getType(); //< right val has same type
                                               //< due to promoteTypes().
  if(left_type->getTypeID() == Type::FloatTyID)
  {
    // Floating point comparison!
    push_value(ctx, new FCmpInst(fcmp, left_val, right_val, 
          "tmpcmp", ctx->BB));
  } else if(isa<IntegerType>(left_type)) {
    // Integer comparison
    push_value(ctx, new ICmpInst(icmp, left_val, right_val, 
          "tmpcmp", ctx->BB));
  } else {
    // Can't handle this, throw error.
    cerr << "E: Invalid types for camparison operator.\n";
    PANIC("can't compare");
  }
}

//==========================================================================
// Emit a unary assignment expression ({pre,pos}{in,de}crement).
void emitUnaryAssignmentExpression(llvm_context* ctx, firtreeExpression expr,
    firtreeExpression operand)
{
    // Emit the expression
    emitExpression(ctx, operand);
    firtree_value prev_val = pop_full_value(ctx);

    // check this is an lvalue.
    if(prev_val.lvalue == NULL)
    {
      cerr << "E: Attempt to modify a non-lvalue.\n";
      PANIC("cannot modify non lvalue.");
    }

    // Get an appropriately typed '1'.
    // FIXME: make sure this is typed
    Value* one = NULL;
    const Type* prev_type = prev_val.value->getType();
    if(isa<IntegerType>(prev_type)) {
      one = ConstantInt::get(prev_type, 1);
    } else if(prev_type->getTypeID() == Type::FloatTyID) {
      one = ConstantFP::get(Type::FloatTy, APFloat(1.f));
    } else {
      cerr << "E: Cannot inc/decrement a vector type.\n";
      PANIC("Cannot inc/decrement a vector type.");
    }

    Value* result = prev_val.value;
    Value* returned_value = prev_val.value;

    if(firtreeExpression_inc(expr, NULL)) {
      result = BinaryOperator::create(Instruction::Add,
          prev_val.value, one, "tmpinc", ctx->BB);
      returned_value = result;
    }
    if(firtreeExpression_dec(expr, NULL)) {
      result = BinaryOperator::create(Instruction::Sub,
          prev_val.value, one, "tmpdec", ctx->BB);
      returned_value = result;
    }
    if(firtreeExpression_postinc(expr, NULL)) {
      result = BinaryOperator::create(Instruction::Add,
          prev_val.value, one, "tmpinc", ctx->BB);
    }
    if(firtreeExpression_postdec(expr, NULL)) {
      result = BinaryOperator::create(Instruction::Sub,
          prev_val.value, one, "tmpdec", ctx->BB);
    }

    // Store the result and push the new value on the stack
    new StoreInst(result, prev_val.lvalue, ctx->BB);
    push_value(ctx, returned_value, prev_val.lvalue);
}

// Emit a constructor for the specified type
void emitConstructor(llvm_context* ctx, firtreeTypeSpecifier spec,
    const std::vector<llvm::Value*>& param_vec)
{
  static Constant* const zeros[] = {
    ConstantFP::get(Type::FloatTy, APFloat(0.f)),
    ConstantFP::get(Type::FloatTy, APFloat(0.f)),
    ConstantFP::get(Type::FloatTy, APFloat(0.f)),
    ConstantFP::get(Type::FloatTy, APFloat(0.f)),
  };

  // FIXME: Handle vector parameters.
# define PROCESS_PARAMS(count) do { \
    unsigned int __tmp_count = (count); \
    if(param_vec.size() != __tmp_count) { \
      cerr << "E: Constructor expects " << __tmp_count << " parameters.\n"; \
      PANIC("wrong parameter count."); \
    } \
    for(unsigned int i=0; i<__tmp_count; i++) { \
      vec_val = new InsertElementInst(vec_val, param_vec[i], i, \
          "tmpins", ctx->BB); \
    } \
  } while(0)

  // Vector types
  if(firtreeTypeSpecifier_vec4(spec) || firtreeTypeSpecifier_color(spec)) {
    Value* vec_val = ConstantVector::get(zeros, 4);
    PROCESS_PARAMS(4);
    push_value(ctx, vec_val);
    return;
  } else if(firtreeTypeSpecifier_vec3(spec)) {
    Value* vec_val = ConstantVector::get(zeros, 3);
    PROCESS_PARAMS(3);
    push_value(ctx, vec_val);
    return;
  } else if(firtreeTypeSpecifier_vec2(spec)) {
    Value* vec_val = ConstantVector::get(zeros, 2);
    PROCESS_PARAMS(2);
    push_value(ctx, vec_val);
    return;
  }

  // Float
  if(firtreeTypeSpecifier_float(spec)) {
    if(param_vec.size() != 1) {
      cerr << "E: Conversion to float requires one parameter.\n";
      PANIC("invalid conversion.");
    }
    push_value(ctx, getAsFloat(ctx, param_vec.front()));
    return;
  }

  // Int
  if(firtreeTypeSpecifier_int(spec)) {
    if(param_vec.size() != 1) {
      cerr << "E: Conversion to int requires one parameter.\n";
      PANIC("invalid conversion.");
    }
    push_value(ctx, getAsInt(ctx, param_vec.front(), Type::Int32Ty));
    return;
  }

  // Bool
  if(firtreeTypeSpecifier_bool(spec)) {
    if(param_vec.size() != 1) {
      cerr << "E: Conversion to bool requires one parameter.\n";
      PANIC("invalid conversion.");
    }
    push_value(ctx, getAsInt(ctx, param_vec.front(), Type::Int1Ty));
    return;
  }

  cerr << "E: No constructor available for type '" << symbolToString(PT_product(spec)) << "'\n";
  PANIC("unimplemented constructor.");
}

//==========================================================================
// Emit an individual expression.
void emitExpression(llvm_context* ctx, firtreeExpression expr)
{
  //cerr << "exp: " << symbolToString(PT_product(expr)) << "\n";

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

  // Multiple part expression, pop all but the last value
  if(firtreeExpression_expression(expr, &left, &rest))
  {
    emitExpression(ctx, left);
    GLS_Lst(firtreeExpression) tail;
    GLS_FORALL(tail, rest) {
      // Pop prior value
      pop_value(ctx);

      firtreeExpression e = GLS_FIRST(firtreeExpression, tail);
      emitExpression(ctx, e);
    }
    return;
  }

  // Binary expr.
# define BIN_OP(name) if(firtreeExpression_##name(expr, &left, &right)) { \
    found_expr = 1; \
  } else

  BIN_OP(add)
  BIN_OP(sub)
  BIN_OP(mul)
  BIN_OP(div)
  BIN_OP(logicalor)
  BIN_OP(logicaland)
  BIN_OP(logicalxor)
  /* else */ { found_expr = 0; }
  
  if(found_expr) {
    emitBinaryArithmeticExpression(ctx, expr, left, right);
    return;
  }

  BIN_OP(greater)
  BIN_OP(greaterequal)
  BIN_OP(less)
  BIN_OP(lessequal)
  BIN_OP(equal)
  BIN_OP(notequal)
  /* else */ { found_expr = 0; }
 
  if(found_expr) {
    emitBinaryComparisonExpression(ctx, expr, left, right);
    return;
  }

  BIN_OP(assign)
  BIN_OP(addassign)
  BIN_OP(subassign)
  BIN_OP(mulassign)
  BIN_OP(divassign)
  /* else */ { found_expr = 0; }
 
  if(found_expr) {
    emitBinaryAssignmentExpression(ctx, expr, left, right);
    return;
  }

  // Special case, negation
  if(firtreeExpression_negate(expr, &left)) {
    Value* negone = ConstantFP::get(Type::FloatTy, APFloat(-1.f));
    push_value(ctx, BinaryOperator::create(Instruction::Mul,
          pop_value(ctx), negone, "tmpdec", ctx->BB));
    return;
  }

  // Unary expr.
# define UN_OP(name) if(firtreeExpression_##name(expr, &left)) { \
    found_expr = 1; \
  } else

  UN_OP(inc)
  UN_OP(dec)
  UN_OP(postinc)
  UN_OP(postdec)
  /* else */ { found_expr = 0; }
  
  if(found_expr) {
    emitUnaryAssignmentExpression(ctx, expr, left);
    return;
  }

  // Return statement.
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
    // Emit the condition test
    emitExpression(ctx, condition);
    Value* cond_val = pop_value(ctx);

    BasicBlock* thenBB = new BasicBlock("true", ctx->F);
    BasicBlock* elseBB = new BasicBlock("false", ctx->F);
    BasicBlock* contBB = new BasicBlock("continue", ctx->F);

    BasicBlock* orig_bb = ctx->BB;

    // Emit the true and false blocks (ahead of time so we can
    // get the values).
    ctx->BB = thenBB;
    emitExpression(ctx, left);
    Value* left_val = pop_value(ctx);
    ctx->BB = elseBB;
    emitExpression(ctx, right);
    Value* right_val = pop_value(ctx);

    if(!typesMatch(ctx, left_val, right_val))
    {
      cerr << "E: Both values of ternary expression must be of same type.\n";
      PANIC("invalid ternary");
    }

    ctx->BB = orig_bb;

    // Create a temporary to hold the result
    AllocaInst* ret_val = new AllocaInst(
      left_val->getType(), "ternarytmp", ctx->BB);

    // Emit a branch instruction.
    new BranchInst(thenBB, elseBB, cond_val, ctx->BB);

    // Store the result and branch out
    ctx->BB = thenBB;
    new StoreInst(left_val, ret_val, ctx->BB);
    ctx->BB->getInstList().push_back(new BranchInst(contBB));
    ctx->BB = elseBB;
    new StoreInst(right_val, ret_val, ctx->BB);
    ctx->BB->getInstList().push_back(new BranchInst(contBB));

    ctx->BB = contBB;
    push_value(ctx, new LoadInst(ret_val, "tmpload", ctx->BB));

    return;
  }

  // fieldselect
  if(firtreeExpression_fieldselect(expr, &left, &left_tok))
  {
    emitSwizzle(ctx, left, left_tok);
    return;
  }

  // compound
  if(firtreeExpression_compound(expr, &rest))
  {
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
    std::vector<llvm::Value*> param_vec;
    GLS_FORALL(tail, rest) {
      firtreeExpression e = GLS_FIRST(firtreeExpression, tail);
      emitExpression(ctx, e);

      // FIXME: Handle in/out params
      param_vec.push_back(pop_value(ctx));
    }

    if(firtreeFunctionSpecifier_constructorfor(func_spec, &type_spec))
    {
      // Emit the constructor for this value.
      emitConstructor(ctx, type_spec, param_vec);
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
      // Emit the initialiser
      emitExpression(ctx, init);

      BasicBlock* loopBB = new BasicBlock("loop", ctx->F);
      BasicBlock* bodyBB = new BasicBlock("body", ctx->F);
      BasicBlock* contBB = new BasicBlock("afterloop", ctx->F);

      ctx->BB->getInstList().push_back(new BranchInst(loopBB));
      ctx->BB = loopBB;

      // Emit code for calculating condition
      emitExpression(ctx, cond);
      Value* cond_val = pop_value(ctx);

      new BranchInst(bodyBB, contBB, cond_val, ctx->BB);

      ctx->BB = bodyBB;

      emitExpression(ctx, body);

      // Add a terminator if required
      if(ctx->BB->getTerminator() == NULL)
      {
        emitExpression(ctx, iter);
        
        if(ctx->BB->getTerminator() != NULL)
        {
          PANIC("iteration expression cannot be terminal.");
        }

        ctx->BB->getInstList().push_back(new BranchInst(loopBB));
      }

      ctx->BB = contBB;
      return;
    }
  }

  // selection
  {
    firtreeExpression cond, thenblock, elseblock;
    if(firtreeExpression_selection(expr, &cond, &thenblock, &elseblock))
    {
      // Emit code for calculating condition
      emitExpression(ctx, cond);

      Value* cond_val = pop_value(ctx);

      BasicBlock* thenBB = new BasicBlock("then", ctx->F);
      BasicBlock* elseBB = new BasicBlock("else", ctx->F);
      BasicBlock* contBB = new BasicBlock("continue", ctx->F);

      new BranchInst(thenBB, elseBB, cond_val, ctx->BB);

      // Emit the then branch
      ctx->BB = thenBB;
      emitExpression(ctx, thenblock);

      // Add a terminator if required
      if(ctx->BB->getTerminator() == NULL)
      {
        ctx->BB->getInstList().push_back(new BranchInst(contBB));
      }

      // Emit the else branch
      ctx->BB = elseBB;
      emitExpression(ctx, elseblock);
      elseBB->getInstList().push_back(new BranchInst(contBB));

      // Add a terminator if required
      if(ctx->BB->getTerminator() == NULL)
      {
        ctx->BB->getInstList().push_back(new BranchInst(contBB));
      }

      ctx->BB = contBB;
      return;
    }
  }

  // while
  if(firtreeExpression_while(expr, &left, &right))
  {
    BasicBlock* loopBB = new BasicBlock("loop", ctx->F);
    BasicBlock* bodyBB = new BasicBlock("body", ctx->F);
    BasicBlock* contBB = new BasicBlock("afterloop", ctx->F);

    ctx->BB->getInstList().push_back(new BranchInst(loopBB));
    ctx->BB = loopBB;

    // Emit code for calculating condition
    emitExpression(ctx, left);
    Value* cond_val = pop_value(ctx);

    new BranchInst(bodyBB, contBB, cond_val, ctx->BB);

    ctx->BB = bodyBB;

    emitExpression(ctx, right);

    // Add a terminator if required
    if(ctx->BB->getTerminator() == NULL)
    {
      ctx->BB->getInstList().push_back(new BranchInst(loopBB));
    }

    ctx->BB = contBB;
    return;
  }

  // do
  if(firtreeExpression_do(expr, &left, &right))
  {
    BasicBlock* loopBB = new BasicBlock("loop", ctx->F);
    BasicBlock* contBB = new BasicBlock("afterloop", ctx->F);

    ctx->BB->getInstList().push_back(new BranchInst(loopBB));
    ctx->BB = loopBB;

    // Wmit body
    emitExpression(ctx, left);

    // Emit code for calculating condition
    emitExpression(ctx, right);
    Value* cond_val = pop_value(ctx);

    new BranchInst(loopBB, contBB, cond_val, ctx->BB);

    ctx->BB = contBB;
    return;
  }

  cerr << "unknown expr: " << symbolToString(PT_product(expr)) << "\n";
  PANIC("unknown expr");
}

/* vim:sw=2:ts=2:cindent:et 
 */
