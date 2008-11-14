#ifndef __LLVM_OUT_PRIV_H
#define __LLVM_OUT_PRIV_H

#include "ptm_gen.h" // General Parsing Routines
#include "ptm_pp.h"  // Pretty Printer
#include "gls.h"     // General Language Services
#include "symbols.h" // Datatype: Symbols

#include "gen/firtree_int.h" // grammar interface

#include <map>
#include <stack>

#include "llvm/Module.h"
#include "llvm/DerivedTypes.h"
#include "llvm/Constants.h"
#include "llvm/Instructions.h"
#include "llvm/Bitcode/ReaderWriter.h"

#include "llvmout.h"

struct llvm_context;

struct variable {
  llvm::Value*          value;
  symbol                name;
  firtreeTypeSpecifier  type_spec;
  firtreeTypeQualifier  type_qual;
};

typedef void (*lvalue_cb_t) (llvm_context* ctx, 
    llvm::Value* new_value, void* lvalue_context);

// A value is a llvm value along with, if it is also an lvalue, a call-back
// plus context for setting it.
struct firtree_value {
  llvm::Value*          value;
  lvalue_cb_t           lvalue_cb;
  void*                 lvalue_context;
};

struct llvm_context {
  llvm::Module*       M;  //< current module.
  llvm::Function*     F;  //< current function.
  llvm::BasicBlock*   BB; //< current block.

  std::stack<firtree_value>  val_stack;  //< value (and ptr to said if lvalue) stack.

  // A 'stack' of scopes, each scope is a map between a symbol
  // and a pointer to the associated value. This isn't a true
  // stack because we want to search up the 'stack' to find symbols
  // in parent scopes.
  std::vector< std::map<symbol, variable> > scope_stack;

  // A multimap between function identifier and it's prototype. We use
  // a multimap because we can have overloaded functions in Firtree 
  // (although we keep quiet about it!).
  std::multimap<std::string, firtreeFunctionPrototype>  func_table;

  // A map between function prototype and the LLVM function.
  std::map<firtreeFunctionPrototype, llvm::Function*>   llvm_func_table;
};

#define PANIC(msg) do { assert(false && (msg)); } while(0)

#ifdef __cpluplus
extern "C" {
#endif

void emitExpressionList(llvm_context* ctx, GLS_Lst(firtreeExpression) exprs);
void emitExpression(llvm_context* ctx, firtreeExpression expr);

#ifdef __cpluplus
}
#endif

inline llvm::Value* pop_value(llvm_context* ctx)
{
  firtree_value rv = ctx->val_stack.top();
  ctx->val_stack.pop();
  return rv.value;
}

inline firtree_value pop_full_value(llvm_context* ctx)
{
  firtree_value rv = ctx->val_stack.top();
  ctx->val_stack.pop();
  return rv;
}

inline void push_value(llvm_context* ctx, llvm::Value* val,
    lvalue_cb_t cb = NULL, void* context = NULL)
{
  firtree_value v = { val, cb, context };
  ctx->val_stack.push(v);
}

inline void push_scope(llvm_context* ctx)
{
  ctx->scope_stack.push_back( std::map<symbol, variable> () );
}

inline void pop_scope(llvm_context* ctx)
{
  ctx->scope_stack.pop_back();
}

inline void declare_variable(llvm_context* ctx, const variable& var)
{
  ctx->scope_stack.back().insert(
      std::pair<symbol, variable>(var.name, var));
}

inline const variable& find_symbol(llvm_context* ctx, symbol sym)
{
  static variable invalid_var = {
    NULL, NULL, NULL, NULL,
  };

  std::vector< std::map<symbol, variable> >::const_reverse_iterator i =
    ctx->scope_stack.rbegin();
  for( ; i != ctx->scope_stack.rend(); i++)
  {
    const std::map<symbol, variable>& sym_table = *i;

    if(sym_table.count(sym) != 0)
    {
      return sym_table.find(sym)->second;
    }
  }

  return invalid_var;
}

inline bool is_valid_variable(const variable& var)
{
  return (var.value != NULL);
}

inline void crack_fully_specified_type(firtreeFullySpecifiedType type,
    firtreeTypeSpecifier* spec, firtreeTypeQualifier* qual)
{
  if(firtreeFullySpecifiedType_unqualifiedtype(type, spec))
  { 
    *qual = NULL;
  } else if(firtreeFullySpecifiedType_qualifiedtype(type, qual, spec))
  { /* nop */
  } else {
    PANIC("unknown fully specified type.");
  }
}

#endif /* __LLVM_OUT_PRIV_H */

/* vim:sw=2:ts=2:cindent:et 
 */
