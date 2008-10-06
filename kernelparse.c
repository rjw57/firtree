/* ------------------------------------------------------------------------ */
/*                                                                          */
/* [firtree.c]                      PL0 Interpreter                             */
/*                                                                          */
/* Copyright (c) 2000 by Doelle, Manns                                      */
/* ------------------------------------------------------------------------ */

#include "stdosx.h"  // General Definitions (for gcc)
#include "ptm_gen.h" // General Parsing Routines
#include "ptm_pp.h"  // Pretty Printer
#include "gls.h"     // General Language Services
#include "hmap.h"    // Datatype: Finite Maps
#include "symbols.h" // Datatype: Symbols

#include "firtree_int.h" // grammar interface
#include "firtree_lim.h" // scanner table
#include "firtree_pim.h" // parser  table

void printFuncDef(firtreeFunctionDefinition def)
{
  printf("def\n");
}

void printFuncProto(firtreeFunctionPrototype proto)
{
  printf("proto\n");
}

void printDecl(firtreeExternalDeclaration decl)
{
  firtreeFunctionDefinition func_def;
  firtreeFunctionPrototype func_proto;

  if(firtreeExternalDeclaration_definefuntion(decl, &func_def)) 
  {
    printFuncDef(func_def); 
  } else if(firtreeExternalDeclaration_declarefunction(decl, &func_proto)) {
    printFuncProto(func_proto); 
  } else {
    printf("Argh!\n");
  }
}

void printDecls(GLS_Lst(firtreeExternalDeclaration) decls)
{
  firtreeExternalDeclaration dit;

  GLS_FORALL(dit, decls) {
    firtreeExternalDeclaration decl = GLS_FIRST(firtreeExternalDeclaration,dit);
    printDecl(decl);
  }
}

/* Main Program ------------------------------------------------------------ */

void Kernel(string fileid)
/* initialize and get source */
{ Scn_T scn; Scn_Stream cstream; // scanner table & configuration
  PLR_Tab plr; PT_Cfg PCfg;      // parser  table & configuration
  PT_Term srcterm;               // the source term
  //
  // init modules
  //
  MAP_init(); initSymbols(); firtree_initSymbols();
  GLS_init();
  //
  // Parse the source file
  //
  Scn_get_firtree(&scn);                           // Get scanner table
  cstream = Stream_file(scn,"",fileid,"");     // Open source file
  plr     = PLR_get_firtree();                     // Get parser table
  PCfg    = PT_init(plr,cstream);              // Create parser
  srcterm = PT_PARSE(PCfg,"TranslationUnit");          // Parse
  PT_setErrorCnt(PT_synErrorCnt(PCfg));        // Save error count
  PT_quit(PCfg);                               // Free parser
  Stream_close(cstream);                       // Close source stream
  Stream_free(cstream);                        // Free source stream
  Scn_free(scn);                               // Free scanner table
  PLR_delTab(plr);                             // Free parser table
  //
  // done parsing, proceed if no syntax errors
  //
  if (PT_errorCnt() == 0)
  { GLS_Lst(firtreeExternalDeclaration) decls;
    // get tree for start symbol
    bug0( firtree_Start_TranslationUnit((firtree)srcterm,&decls), "Program expected");
    // check & execute program
    printDecls(decls);
    //StaticSemantic(src);
    //if (PT_errorCnt() == 0) DynamicSemantic(src);
  }
  if (PT_errorCnt() > 0)
  {
    fprintf(stderr,"Total %d errors.\n",PT_errorCnt());
    STD_ERREXIT;
  }
  //
  // release allocated objects
  //
  PT_delT(srcterm);
  firtree_quitSymbols();
  freeSymbols();
  MAP_quit();
}

int main(int argc, string argv[])
{
  if( argc > 1 ) Kernel(argv[1]);
  else fprintf(stderr,"missing source\n");
  BUG_CORE; // check for object left over
  return 0;
}
