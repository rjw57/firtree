#ifndef __LLVM_OUT_H
#define __LLVM_OUT_H

#include "gls.h"     // General Language Services
#include "firtree_int.h" // grammar interface

#ifdef __cpluplus
extern "C" {
#endif

void printDeclList(GLS_Lst(firtreeExternalDeclaration) decls);

#ifdef __cpluplus
}
#endif

#endif /* __LLVM_OUT_H */

/* vim:sw=2:ts=2:cindent:et 
 */
