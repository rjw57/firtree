//===========================================================================
/// \file compiler.cpp Interface to LLVM-based compiler.

#include "compiler.h"

#include "stdosx.h"  // General Definitions (for gcc)
#include "ptm_gen.h" // General Parsing Routines
#include "ptm_pp.h"  // Pretty Printer
#include "gls.h"     // General Language Services
#include "hmap.h"    // Datatype: Finite Maps
#include "symbols.h" // Datatype: Symbols

#include "styx/firtree_int.h" // grammar interface
#include "styx/firtree_lim.h" // scanner table
#include "styx/firtree_pim.h" // parser  table

#include "llvm_frontend.h"

#include "llvm/PassManager.h"
#include "llvm/Bitcode/ReaderWriter.h"

#include "llvm/Analysis/Verifier.h"
#include "llvm/Analysis/LoopPass.h"
#include "llvm/Analysis/CallGraph.h"
#include "llvm/Target/TargetData.h"
#include "llvm/Transforms/Scalar.h"
#include "llvm/LinkAllPasses.h"

#include <iostream>

using namespace llvm;

namespace Firtree
{
	
//===========================================================================
// A class to take a set of source lines and return one character at a 
// time.
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
Compiler::Compiler()
	:	m_CurrentFrontend(NULL)
	,	m_Log(NULL)
	,	m_LogSize(0)
	,	m_OptimiseLLVM(true)
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
Compiler::~Compiler()
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
	} else {
		--g_ModuleInitCount;
	}
}

//===========================================================================
Compiler* Compiler::Create()
{
	return new Compiler();
}

//===========================================================================
bool Compiler::Compile(const char* const* source_lines, 
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

	SourceReader source_reader(source_lines, source_line_count);

	Scn_T scn;
	Scn_Stream cstream; // scanner table & configuration
	PLR_Tab plr;
	PT_Cfg PCfg;      // parser  table & configuration
	PT_Term srcterm;               // the source term
	
	//
	// Parse the source file
	//
	Scn_get_firtree( &scn );                         // Get scanner table
	// Open source file
	cstream = Stream_line( scn, &source_reader, GetNextChar, "streamsource");
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

	bool return_value = false;

	if ( PT_errorCnt() == 0 ) {
		m_CurrentFrontend = new Firtree::LLVMFrontend(( firtree )srcterm );

		return_value = m_CurrentFrontend->GetCompilationSucceeded();

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
	}

	if ( PT_errorCnt() > 0 ) {
		fprintf( stderr,"Total %d errors.\n",PT_errorCnt() );
	}

	//
	// release allocated objects
	//
	PT_delT( srcterm );

	// If compilation succeeded, run the optimiser if required.
	if(return_value && m_OptimiseLLVM)
	{
		RunOptimiser();
	}

	return return_value;
}

//===========================================================================
bool Compiler::GetCompileStatus() const
{
	if(m_CurrentFrontend == NULL)
	{
		return false;
	}

	return m_CurrentFrontend->GetCompilationSucceeded();
}

//===========================================================================
const char* const* Compiler::GetCompileLog(uint32_t* log_line_count)
{
	if(log_line_count != NULL)
	{
		*log_line_count = m_LogSize;
	}
	return m_Log;
}

//===========================================================================
void Compiler::DumpInitialLLVM() const
{
	llvm::Module* m = GetCompiledModule();
	if(m == NULL)
	{
		return;
	}

	m->dump();
}

//===========================================================================
llvm::Module* Compiler::GetCompiledModule() const
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
void Compiler::SetDoOptimization(bool flag) 
{
	m_OptimiseLLVM = flag;
}

//===========================================================================
bool Compiler::GetDoOptimization() const
{
	return m_OptimiseLLVM;
}

//===========================================================================
void Compiler::RunOptimiser() 
{
	llvm::Module* m = m_CurrentFrontend->GetCompiledModule();
	if(m == NULL)
	{
		return;
	}

    PassManager PM;

	PM.add(new TargetData(m));

    PM.add(createVerifierPass());

	PM.add(createStripSymbolsPass(true));
	PM.add(createRaiseAllocationsPass());     // call %malloc -> malloc inst
	PM.add(createCFGSimplificationPass());    // Clean up disgusting code
	PM.add(createPromoteMemoryToRegisterPass());// Kill useless allocas
	PM.add(createGlobalOptimizerPass());      // Optimize out global vars
	PM.add(createGlobalDCEPass());            // Remove unused fns and globs
	PM.add(createIPConstantPropagationPass());// IP Constant Propagation
	PM.add(createDeadArgEliminationPass());   // Dead argument elimination
	PM.add(createInstructionCombiningPass()); // Clean up after IPCP & DAE
	PM.add(createCFGSimplificationPass());    // Clean up after IPCP & DAE

	PM.add(createPruneEHPass());              // Remove dead EH info

	PM.add(createFunctionInliningPass());   // Inline small functions
	PM.add(createArgumentPromotionPass());    // Scalarize uninlined fn args

	PM.add(createTailDuplicationPass());      // Simplify cfg by copying code
	PM.add(createInstructionCombiningPass()); // Cleanup for scalarrepl.
	PM.add(createCFGSimplificationPass());    // Merge & remove BBs
	PM.add(createScalarReplAggregatesPass()); // Break up aggregate allocas
	PM.add(createInstructionCombiningPass()); // Combine silly seq's
	PM.add(createCondPropagationPass());      // Propagate conditionals

	PM.add(createTailCallEliminationPass());  // Eliminate tail calls
	PM.add(createCFGSimplificationPass());    // Merge & remove BBs
	PM.add(createReassociatePass());          // Reassociate expressions
	PM.add(createLoopRotatePass());
	PM.add(createLICMPass());                 // Hoist loop invariants
	PM.add(createLoopUnswitchPass());         // Unswitch loops.
	PM.add(createLoopIndexSplitPass());       // Index split loops.
	PM.add(createInstructionCombiningPass()); // Clean up after LICM/reassoc
	PM.add(createIndVarSimplifyPass());       // Canonicalize indvars
	PM.add(createLoopUnrollPass());           // Unroll small loops
	PM.add(createInstructionCombiningPass()); // Clean up after the unroller
	PM.add(createGVNPass());                  // Remove redundancies
	PM.add(createSCCPPass());                 // Constant prop with SCCP

	// Run instcombine after redundancy elimination to exploit opportunities
	// opened up by them.
	PM.add(createInstructionCombiningPass());
	PM.add(createCondPropagationPass());      // Propagate conditionals

	PM.add(createDeadStoreEliminationPass()); // Delete dead stores
	PM.add(createAggressiveDCEPass());        // SSA based 'Aggressive DCE'
	PM.add(createCFGSimplificationPass());    // Merge & remove BBs
	PM.add(createSimplifyLibCallsPass());     // Library Call Optimizations
	PM.add(createDeadTypeEliminationPass());  // Eliminate dead types
	PM.add(createConstantMergePass());        // Merge dup global constants

	PM.run(*m);
}

} // namespace Firtree

// vim:sw=4:ts=4:cindent:noet
