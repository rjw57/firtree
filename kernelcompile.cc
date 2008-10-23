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

#include "gen/firtree_int.h" // grammar interface
#include "gen/firtree_lim.h" // scanner table
#include "gen/firtree_pim.h" // parser  table

#include "llvmout.h"

#include "llvm_backend/llvm_backend.h"

#include "llvm/Bitcode/ReaderWriter.h"
#include <iostream>

/* Main Program ------------------------------------------------------------ */

void compile_kernel( const char* fileid )
/* initialize and get source */
{
	Scn_T scn;
	Scn_Stream cstream; // scanner table & configuration
	PLR_Tab plr;
	PT_Cfg PCfg;      // parser  table & configuration
	PT_Term srcterm;               // the source term
	//
	// init modules
	//
	MAP_init();
	initSymbols();
	firtree_initSymbols();
	GLS_init();
	//
	// Parse the source file
	//
	Scn_get_firtree( &scn );                         // Get scanner table
	cstream = Stream_file( scn,"",( char* )fileid,"" ); // Open source file
	plr     = PLR_get_firtree();                     // Get parser table
	PCfg    = PT_init( plr,cstream );            // Create parser
	srcterm = PT_PARSE( PCfg,"TranslationUnit" );        // Parse
	PT_setErrorCnt( PT_synErrorCnt( PCfg ) );    // Save error count
	PT_quit( PCfg );                             // Free parser
	Stream_close( cstream );                     // Close source stream
	Stream_free( cstream );                      // Free source stream
	Scn_free( scn );                             // Free scanner table
	PLR_delTab( plr );                           // Free parser table
	//
	// done parsing, proceed if no syntax errors
	//

	if ( PT_errorCnt() == 0 ) {
#if 1
		Firtree::LLVMBackend llvm_backend(( firtree )srcterm );

		if ( llvm_backend.GetCompilationSucceeded() ) {
			// Output the bitcode file to stdout
			WriteBitcodeToFile( llvm_backend.GetCompiledModule(), std::cout );
		}

		// Write the log (if any).
		const std::vector<std::string>& log = llvm_backend.GetLog();

		std::vector<std::string>::const_iterator i = log.begin();

		for ( ; i != log.end(); i++ ) {
			fprintf( stderr, "%s\n", i->c_str() );
		}

#else
		GLS_Lst( firtreeExternalDeclaration ) decls;

		// get tree for start symbol
		bug0( firtree_Start_TranslationUnit(( firtree )srcterm,&decls ), "Program expected" );

		// check & execute program
		emitDeclList( decls );

		//StaticSemantic(src);
		//if (PT_errorCnt() == 0) DynamicSemantic(src);
#endif
	}

	if ( PT_errorCnt() > 0 ) {
		fprintf( stderr,"Total %d errors.\n",PT_errorCnt() );
		STD_ERREXIT;
	}

	//
	// release allocated objects
	//
	PT_delT( srcterm );

	firtree_quitSymbols();

	freeSymbols();

	MAP_quit();
}

int main( int argc, const char* argv[] )
{
	if ( argc > 1 ) compile_kernel( argv[1] );
	else fprintf( stderr,"missing source\n" );

	BUG_CORE; // check for object left over

	return 0;
}

/* vim:sw=2:ts=2:cindent:et
 */
