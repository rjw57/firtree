/* ------------------------------------------------------------------------ */
/*                                                                          */
/* [firtree.c]                      PL0 Interpreter                             */
/*                                                                          */
/* Copyright (c) 2000 by Doelle, Manns                                      */
/* ------------------------------------------------------------------------ */

#include "../compiler.h"

#include "llvm/Bitcode/ReaderWriter.h"
#include <iostream>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>

/* Main Program ------------------------------------------------------------ */

int compile_kernel( const char* fileid )
/* initialize and get source */
{
  int fd = open(fileid, O_RDONLY);
  if(fd == -1)
  {
    fprintf(stderr, "Could not open file '%s' for reading.\n",
        fileid);
    return 0;
  }
  off_t file_size = lseek(fd, 0, SEEK_END);
  lseek(fd, 0, SEEK_SET);
  char* file_contents = new char[file_size];
  read(fd, file_contents, file_size);
  close(fd);

  Firtree::Compiler* compiler = Firtree::Compiler::Create();

  bool status = compiler->Compile(&file_contents, 1);
  std::cerr << "Status: " << status << "\n";

  std::cerr << "Log:\n";
  const char* const * log = compiler->GetCompileLog();
  while((log != NULL) && (*log != NULL))
  {
    std::cerr << *log << '\n';
    log++;
  }

  if(status)
  {
    std::cerr << "LLVM:\n";
    compiler->DumpInitialLLVM();
  }

  FIRTREE_SAFE_RELEASE(compiler);

  delete file_contents;

//	int return_value = 0;
//
//	if ( PT_errorCnt() == 0 ) {
//		Firtree::LLVMFrontend llvm_frontend(( firtree )srcterm );
//
//		if ( llvm_frontend.GetCompilationSucceeded() ) {
//			// Output the bitcode file to stdout
//			WriteBitcodeToFile( llvm_frontend.GetCompiledModule(), std::cout );
//		} else {
//			return_value = 1;
//		}
//
//		// Write the log (if any).
//		const std::vector<std::string>& log = llvm_frontend.GetLog();
//
//		std::vector<std::string>::const_iterator i = log.begin();
//
//		for ( ; i != log.end(); i++ ) {
//			fprintf( stderr, "%s\n", i->c_str() );
//		}
//	}
//
//	if ( PT_errorCnt() > 0 ) {
//		fprintf( stderr,"Total %d errors.\n",PT_errorCnt() );
//		STD_ERREXIT;
//	}
//
	return 0;
}

int main( int argc, const char* argv[] )
{
	int rv = 1;
	if ( argc > 1 ) {
		rv = compile_kernel( argv[1] );
	}	else {
		fprintf( stderr,"missing source\n" );
	}

	return rv;
}

/* vim:sw=2:ts=2:cindent:et
 */
