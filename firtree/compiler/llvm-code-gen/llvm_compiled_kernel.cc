//===========================================================================
/// \file compiler.cpp Interface to LLVM-based compiler.

#include <firtree/compiler/llvm_compiled_kernel.h>

#include "stdosx.h"  // General Definitions (for gcc)
#include "ptm_gen.h" // General Parsing Routines
#include "ptm_pp.h"  // Pretty Printer
#include "gls.h"     // General Language Services
#include "hmap.h"    // Datatype: Finite Maps
#include "symbols.h" // Datatype: Symbols

#include "../parser/firtree_int.h" // grammar interface
#include "../parser/firtree_lim.h" // scanner table
#include "../parser/firtree_pim.h" // parser  table

#include "llvm_frontend.h"

#include "llvm/PassManager.h"
#include "llvm/Bitcode/ReaderWriter.h"

#include "llvm/Analysis/Verifier.h"
#include "llvm/Analysis/LoopPass.h"
#include "llvm/Analysis/CallGraph.h"
#include "llvm/Target/TargetData.h"
#include "llvm/Transforms/Scalar.h"
#include "llvm/LinkAllPasses.h"

#include "llvm/Support/CommandLine.h"

#include <iostream>
#include <vector>

using namespace llvm;

namespace Firtree { namespace LLVM {

#include "../builtins.h"
	
//===========================================================================
// A class to take a set of source lines and return one character at a 
// time. Optionally it can concatenate it's output to that of one or more
// SourceReader-s providing a 'header'.
class SourceReader {
	public:
		SourceReader(const char* const* source_lines, 
				int source_line_count)
			:	m_CurrentLineIdx(0)
			,	m_CurrentCharIdx(0)
			,	m_SourceLines(source_lines)
			,	m_SourceLineCount(source_line_count)
		{
		}

		~SourceReader()
		{
		}

		void Reset()
		{
			m_CurrentLineIdx = 0;
			m_CurrentCharIdx = 0;
		}

		int GetNextChar()
		{
			// Handle being on final line.
			if(m_SourceLineCount >= 0)
			{
				if(m_CurrentLineIdx >= m_SourceLineCount)
				{
					return EOF;
				}
			}
			if(m_SourceLines[m_CurrentLineIdx] == NULL)
			{
				return EOF;
			}

			// If at end of line, return a newline character
			// and advance.
			if(m_SourceLines[m_CurrentLineIdx][m_CurrentCharIdx] == '\0')
			{
				m_CurrentCharIdx = 0;
				++m_CurrentLineIdx;
				return (int)('\n');
			}

			int retval = m_SourceLines[m_CurrentLineIdx][m_CurrentCharIdx];
			++m_CurrentCharIdx;
			return retval;
		}

	private:
		int 		m_CurrentLineIdx; 	// The line from which the next char.
										// is to be returned.
		int 		m_CurrentCharIdx; 	// The character to be returned next.

		const char* const*	m_SourceLines;
		int					m_SourceLineCount;
};

//===========================================================================
/// A class whcih manages the magic required to concatenate multiple 
/// scanners into one 'super' scanner.
class Scanner {
	private:
		typedef	std::vector<Scn_Stream> StreamVector;
		typedef	std::vector<Scn_Stream>::iterator StreamVectorIt;

	public:
		Scanner(StreamVector& scanners) 
			:	m_AbsScanner(NULL)
			,	m_Scanners(scanners)
		{
			m_AbsScanner = AS_init();
			AS_setScanner(m_AbsScanner, this);

			AS_setFunNextTok(m_AbsScanner, &Scanner::nextTok);
			AS_setFunTokID(m_AbsScanner, &Scanner::tokID);
			AS_setFunTokSym(m_AbsScanner, &Scanner::tokSym);
			AS_setFunStreamSym(m_AbsScanner, &Scanner::streamSym);
			AS_setFunTokRow(m_AbsScanner, &Scanner::tokRow);
			AS_setFunTokCol(m_AbsScanner, &Scanner::tokCol);
			AS_setFunUnicode(m_AbsScanner, &Scanner::unicode);
			AS_setFunDefEofID(m_AbsScanner, &Scanner::defEofID);
			AS_setFunDefErrID(m_AbsScanner, &Scanner::defErrID);
			AS_setFunDefTokID(m_AbsScanner, &Scanner::defTokID);
			AS_setFunDefKeyID(m_AbsScanner, &Scanner::defKeyID);
			AS_setFunDefWCKeyID(m_AbsScanner, &Scanner::defWCKeyID);
		}

		~Scanner()
		{
			if(m_AbsScanner) {
				AS_quit(m_AbsScanner);
				m_AbsScanner = NULL;
			}
		}

		AbsScn_T GetScannerObject() const 
		{
			return m_AbsScanner;
		}

	protected:
		Scn_Stream CurrentStream() const { 
			return m_Scanners.front();
		}

		static void nextTok(Abs_T scanner) 
		{ 
			Scanner* self = reinterpret_cast<Scanner*>(scanner);
			Stream_next(self->CurrentStream());

			// Are we at EOF of current stream?
			if(Stream_ctnam(self->CurrentStream()) == NULL) {
				// If we have another scanner to try, do so.
				if(self->m_Scanners.size() > 1) {
					self->m_Scanners.erase(self->m_Scanners.begin());
					nextTok(scanner);
				}
			}
		}

		static short tokID(Abs_T scanner)
		{ 
			Scanner* self = reinterpret_cast<Scanner*>(scanner);
			return Stream_ctid(self->CurrentStream());
		}

		static symbol tokSym(Abs_T scanner)
		{ 
			Scanner* self = reinterpret_cast<Scanner*>(scanner);
			return Stream_csym(self->CurrentStream());
		}

		static symbol streamSym(Abs_T scanner)
		{ 
			Scanner* self = reinterpret_cast<Scanner*>(scanner);
			return Stream_cfil(self->CurrentStream());
		}

		static long tokRow(Abs_T scanner)
		{ 
			Scanner* self = reinterpret_cast<Scanner*>(scanner);
			return Stream_clin(self->CurrentStream());
		}

		static long tokCol(Abs_T scanner)
		{ 
			Scanner* self = reinterpret_cast<Scanner*>(scanner);
			return Stream_ccol(self->CurrentStream());
		}

		static c_bool unicode(Abs_T scanner)
		{ 
			Scanner* self = reinterpret_cast<Scanner*>(scanner);
			return Stream_unicode(self->CurrentStream());
		}

		static void defEofID(Abs_T scanner, short id) 
		{ 
			Scanner* self = reinterpret_cast<Scanner*>(scanner);
			for(StreamVectorIt i=self->m_Scanners.begin(); i!=self->m_Scanners.end(); ++i)
			{
				Stream_defEofId(*i, id);
			}
		}

		static void defErrID(Abs_T scanner, short id) 
		{ 
			Scanner* self = reinterpret_cast<Scanner*>(scanner);
			for(StreamVectorIt i=self->m_Scanners.begin(); i!=self->m_Scanners.end(); ++i)
			{
				Stream_defErrId(*i, id);
			}
		}

		static void defTokID(Abs_T scanner, c_string text, short id) 
		{ 
			Scanner* self = reinterpret_cast<Scanner*>(scanner);
			for(StreamVectorIt i=self->m_Scanners.begin(); i!=self->m_Scanners.end(); ++i)
			{
				Stream_defTokId(*i, text, id);
			}
		}

		static void defKeyID(Abs_T scanner, c_string text, short id) 
		{ 
			Scanner* self = reinterpret_cast<Scanner*>(scanner);
			for(StreamVectorIt i=self->m_Scanners.begin(); i!=self->m_Scanners.end(); ++i)
			{
				Stream_defKeyId(*i, text, id);
			}
		}

		static void defWCKeyID(Abs_T scanner, wc_string text, short id) 
		{ 
			Scanner* self = reinterpret_cast<Scanner*>(scanner);
			for(StreamVectorIt i=self->m_Scanners.begin(); i!=self->m_Scanners.end(); ++i)
			{
				Stream_defWCKeyId(*i, text, id);
			}
		}

	private:

		AbsScn_T 				m_AbsScanner;
		StreamVector			m_Scanners;
};

//===========================================================================
static int GetNextChar(void* reader)
{
	return reinterpret_cast<SourceReader*>(reader)->GetNextChar();
}

//===========================================================================
// Static counter for module initialisation. If this is zero, the global
// Styx module initialisation functions are called on compiler construction
// and is incremented. On compiler destruction, it is decremented and, if
// equal to zero, the module initialisation is finished.
// 
// FIXME: This makes the whole thing thread unsafe :(
static uint32_t g_ModuleInitCount = 0;

//===========================================================================
CompiledKernel::CompiledKernel()
	:	ReferenceCounted()
	,	m_CurrentFrontend(NULL)
	,	m_Log(NULL)
	,	m_LogSize(0)
	,	m_OptimiseLLVM(true)
	,	m_CompileStatus(false)
{
	++g_ModuleInitCount;
	if(g_ModuleInitCount == 1)
	{
		MAP_init();
		initSymbols();
		firtree_initSymbols();
		GLS_init();
	}
}

//===========================================================================
CompiledKernel::~CompiledKernel()
{
	if(m_Log != NULL)
	{
		delete m_Log;
	}

	if(m_CurrentFrontend != NULL)
	{
		delete m_CurrentFrontend;
	}

	if(g_ModuleInitCount == 1)
	{
		firtree_quitSymbols();
		freeSymbols();
		MAP_quit();
	}

	--g_ModuleInitCount;
}

//===========================================================================
CompiledKernel* CompiledKernel::Create()
{
	return new CompiledKernel();
}

//===========================================================================
bool CompiledKernel::Compile(const char* const* source_lines, 
		int source_line_count)
{
	// Free any existing frontend.
	if(m_Log != NULL)
	{
		delete m_Log;
	}

	if(m_CurrentFrontend != NULL)
	{
		delete m_CurrentFrontend;
	}

	m_KernelList.clear();

	SourceReader builtin_reader(&_firtree_builtins, 1);

	SourceReader source_reader(source_lines, source_line_count);

	Scn_T scn;
	Scn_Stream cstream, builtinstream; // scanner table & configuration
	PLR_Tab plr;
	PT_Cfg PCfg;      // parser  table & configuration
	PT_Term srcterm;               // the source term
	
	//
	// Parse the source file
	//
	Scn_get_firtree( &scn );                         // Get scanner table

	builtinstream = Stream_line( scn, &builtin_reader, GetNextChar, const_cast<char*>("_builtins_"));

	// Open source file
	cstream = Stream_line( scn, &source_reader, GetNextChar, const_cast<char*>("input"));

	std::vector<Scn_Stream> streams;
	streams.push_back(builtinstream);
	streams.push_back(cstream);

	Scanner total_scanner(streams);

	plr     = PLR_get_firtree();                     // Get parser table
	PCfg    = PT_init_extscn( plr, total_scanner.GetScannerObject() );            // Create parser
	srcterm = PT_PARSE( PCfg,const_cast<char*>("TranslationUnit") );        // Parse
	PT_setErrorCnt( PT_synErrorCnt( PCfg ) );    // Save error count
	PT_quit( PCfg );                             // Free parser
	Stream_close( cstream );                     // Close source stream
	Stream_free( cstream );                      // Free source stream
	Stream_close( builtinstream );                     // Close source stream
	Stream_free( builtinstream );                      // Free source stream
	Scn_free( scn );                             // Free scanner table
	PLR_delTab( plr );                           // Free parser table

	//
	// done parsing, proceed if no syntax errors
	//

	m_CompileStatus = false;
	if ( PT_errorCnt() == 0 ) {
		m_CurrentFrontend = new Firtree::LLVMFrontend(( firtree )srcterm,
				&m_KernelList );

		m_CompileStatus = m_CurrentFrontend->GetCompilationSucceeded();

		// Note the log (if any).
		const std::vector<std::string>& log = m_CurrentFrontend->GetLog();

		m_Log = new const char*[log.size() + 1];
		m_LogSize = log.size();

		std::vector<std::string>::const_iterator i = log.begin();
		int idx = 0;
		for ( ; i != log.end(); i++, idx++ ) {
			m_Log[idx] = i->c_str();
		}
		m_Log[idx] = NULL;
	} else {
		fprintf( stderr,"Total %d errors.\n",PT_errorCnt() );
	}

	//
	// release allocated objects
	//
	PT_delT( srcterm );

	// If compilation succeeded, run the optimiser if required.
	if(m_CompileStatus && m_OptimiseLLVM)
	{
		RunOptimiser();
	}

	return m_CompileStatus;
}

//===========================================================================
bool CompiledKernel::GetCompileStatus() const
{
	return m_CompileStatus;
}

//===========================================================================
const char* const* CompiledKernel::GetCompileLog(uint32_t* log_line_count)
{
	if(log_line_count != NULL)
	{
		*log_line_count = m_LogSize;
	}
	return m_Log;
}

//===========================================================================
void CompiledKernel::DumpInitialLLVM() const
{
	llvm::Module* m = GetCompiledModule();
	if(m == NULL)
	{
		return;
	}

	m->dump();
}

//===========================================================================
llvm::Module* CompiledKernel::GetCompiledModule() const
{
	if(m_CurrentFrontend == NULL)
	{
		return NULL;
	}

	if(!m_CurrentFrontend->GetCompilationSucceeded())
	{
		return NULL;
	}

	llvm::Module* m = m_CurrentFrontend->GetCompiledModule();
	return m;
}

//===========================================================================
void CompiledKernel::SetDoOptimization(bool flag) 
{
	m_OptimiseLLVM = flag;
}

//===========================================================================
bool CompiledKernel::GetDoOptimization() const
{
	return m_OptimiseLLVM;
}

//===========================================================================
void CompiledKernel::RunOptimiser() 
{
	llvm::Module* m = m_CurrentFrontend->GetCompiledModule();
	if(m == NULL)
	{
		return;
	}

#if 1
	// Nasty, nasty hack to set an option to unroll loops
	// aggressively.
	static bool set_opt = false;
	static const char* opts[] = {
		"progname",
		"-unroll-threshold",
		"10000000",
	};
	if(!set_opt) {
		cl::ParseCommandLineOptions(3, const_cast<char**>(opts));
		set_opt = true;
	}
#endif

    PassManager PM;

	PM.add(new TargetData(m));

    PM.add(createVerifierPass());

	PM.add(createStripSymbolsPass(true));

	PM.add(createCFGSimplificationPass());    // Clean up disgusting code
	PM.add(createPromoteMemoryToRegisterPass());// Kill useless allocas
	PM.add(createIPConstantPropagationPass());// IP Constant Propagation
	PM.add(createInstructionCombiningPass()); // Clean up after IPCP & DAE
	PM.add(createCFGSimplificationPass());    // Clean up after IPCP & DAE

	PM.add(createCondPropagationPass());      // Propagate conditionals
	PM.add(createReassociatePass());          // Reassociate expressions

	PM.add(createLoopRotatePass());
	PM.add(createLoopSimplifyPass());
	PM.add(createIndVarSimplifyPass());       

	PM.add(createInstructionCombiningPass()); 
	PM.add(createCFGSimplificationPass());    

	PM.add(createLoopRotatePass());
	PM.add(createLoopSimplifyPass());
	PM.add(createIndVarSimplifyPass());     
	PM.add(createLoopStrengthReducePass());
	PM.add(createLoopIndexSplitPass());
#if LLVM_AT_LEAST_2_3
	PM.add(createLoopDeletionPass());          
#endif

	PM.add(createInstructionCombiningPass()); 
	PM.add(createCFGSimplificationPass());   

	PM.add(createLoopUnrollPass());           // Unroll small loops

	PM.add(createInstructionCombiningPass()); 
	PM.add(createCFGSimplificationPass());    

	PM.add(createSCCPPass());                 // Constant prop with SCCP

	// Run instcombine after redundancy elimination to exploit opportunities
	// opened up by them.
	PM.add(createInstructionCombiningPass());
	PM.add(createCondPropagationPass());      // Propagate conditionals

	PM.add(createDeadStoreEliminationPass()); // Delete dead stores
	PM.add(createAggressiveDCEPass());        // SSA based 'Aggressive DCE'
	PM.add(createCFGSimplificationPass());    // Merge & remove BBs

	PM.run(*m);
}

} } // namespace Firtree::LLVM

// vim:sw=4:ts=4:cindent:noet
