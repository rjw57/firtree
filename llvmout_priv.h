#ifndef __LLVM_OUT_PRIV_H
#define __LLVM_OUT_PRIV_H

#include "gls.h"     // General Language Services
#include "gen/firtree_int.h" // grammar interface

#include <map>
#include <stack>

#include "llvm/Module.h"
#include "llvm/DerivedTypes.h"
#include "llvm/Constants.h"
#include "llvm/Instructions.h"
#include "llvm/Bitcode/ReaderWriter.h"

struct llvm_context {
  llvm::Module*       M;  //< current module.
  llvm::Function*     F;  //< current function.
  llvm::BasicBlock*   BB; //< current block.
  std::stack<llvm::Value*>  val_stack;  //< value stack.

  // A multimap between function identifier and it's prototype. We use
  // a multimap because we can have overloaded functions in Firtree 
  // (although we keep quiet about it!).
  std::multimap<std::string, firtreeFunctionPrototype>  func_table;

  // A map between function prototype and the LLVM function.
  std::map<firtreeFunctionPrototype, llvm::Function*>   llvm_func_table;
};

#ifdef __cpluplus
extern "C" {
#endif

void emitExpressionList(llvm_context* ctx, GLS_Lst(firtreeExpression) exprs);
void emitExpression(llvm_context* ctx, firtreeExpression expr);

#ifdef __cpluplus
}
#endif

#define PANIC(msg) do { assert(false && (msg)); } while(0)

#endif /* __LLVM_OUT_PRIV_H */

/* vim:sw=2:ts=2:cindent:et 
 */
