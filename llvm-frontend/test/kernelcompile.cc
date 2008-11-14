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

#include "../styx/firtree_int.h" // grammar interface
#include "../styx/firtree_lim.h" // scanner table
#include "../styx/firtree_pim.h" // parser  table

#include "../llvm_frontend.h"

#include "llvm/Bitcode/ReaderWriter.h"
#include <iostream>

/* Main Program ------------------------------------------------------------ */

int compile_kernel( const char* fileid )
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

	int return_value = 0;

	if ( PT_errorCnt() == 0 ) {
		Firtree::LLVMFrontend llvm_frontend(( firtree )srcterm );

		if ( llvm_frontend.GetCompilationSucceeded() ) {
			// Output the bitcode file to stdout
			WriteBitcodeToFile( llvm_frontend.GetCompiledModule(), std::cout );
		} else {
			return_value = 1;
		}

		// Write the log (if any).
		const std::vector<std::string>& log = llvm_frontend.GetLog();

		std::vector<std::string>::const_iterator i = log.begin();

		for ( ; i != log.end(); i++ ) {
			fprintf( stderr, "%s\n", i->c_str() );
		}
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

	return return_value;
}

int main( int argc, const char* argv[] )
{
	int rv = 1;
	if ( argc > 1 ) {
		rv = compile_kernel( argv[1] );
	}	else {
		fprintf( stderr,"missing source\n" );
	}

	BUG_CORE; // check for object left over

	return rv;
}

/* vim:sw=2:ts=2:cindent:et
 */
