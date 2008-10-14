#ifndef __LLVM_UTIL_PRIV_H
#define __LLVM_UTIL_PRIV_H

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

#include "llvmout_priv.h"

#ifdef __cpluplus
extern "C" {
#endif

bool haveFunctionDeclared(llvm_context* ctx, firtreeFunctionPrototype proto,
    firtreeFunctionPrototype* existing_proto = NULL);
void declareFunction(llvm_context* ctx, firtreeFunctionPrototype proto);
bool typesAreEqual(firtreeFullySpecifiedType a, firtreeFullySpecifiedType b);
bool prototypesCollide(firtreeFunctionPrototype a, firtreeFunctionPrototype b);
const llvm::Type* getLLVMTypeForTerm(firtreeFullySpecifiedType type);
const llvm::Type* getLLVMTypeForTerm(firtreeTypeQualifier qual, firtreeTypeSpecifier spec);

#ifdef __cpluplus
}
#endif

#endif /* __LLVM_UTIL_PRIV_H */

/* vim:sw=2:ts=2:cindent:et 
 */
