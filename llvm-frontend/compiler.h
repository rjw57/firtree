//===========================================================================
/// \file compiler.h Interface to LLVM-based compiler.

#ifndef __FIRTREE_COMPILER_H
#define __FIRTREE_COMPILER_H

#include <firtree/main.h>

namespace llvm { class Module; }

namespace Firtree
{

class LLVMFrontend;

//===========================================================================
/// \brief Main LLVM-based compiler interface.
class Compiler : public ReferenceCounted
{
	protected:
		Compiler();
		virtual ~Compiler();

	public:
		/// Create a new CompilerFrontend. Call Release() to free it.
		static Compiler* Create();

		/// \brief Attempt to compile source code. 
		/// The source code consists of one or more lines pointed to 
		/// by entries in the source_lines array. If source_line_count
		/// is -1 then the source_lines array is assumed to be terminated
		/// by a NULL entry. If source_line_count >= 0 no more than
		/// that many lines will be parsed but a terminating NULL in
		/// source_lines will still be honoured.
		/// \returns true if the compilation succeeded, false otherwise.
		bool Compile(const char* const* source_lines, 
				int source_line_count = -1);

		/// \brief Access the compile log.
		/// \returns A pointer to an array of log lines terminated by
		/// a NULL character. If log_line_count is non-NULL, the value it
		/// points to is updated with the number of lines in the log. The
		/// log persists until the next call to Compile().
		const char* const* GetCompileLog(uint32_t* log_line_count = NULL);

		/// \brief Get last compile status.
		bool GetCompileStatus() const;

		/// \brief Dump the compiled LLVM
		void DumpInitialLLVM() const;

		/// \brief Control whether optimisation is performed.
		/// By default an optimisation pass over the compiled LLVM
		/// is performed.
		void SetDoOptimization(bool flag);
		bool GetDoOptimization() const;

		/// Get the compiled LLVM module.
		llvm::Module* GetCompiledModule() const;

	private:
		void RunOptimiser();

		LLVMFrontend*			m_CurrentFrontend;
		const char**			m_Log;
		uint32_t				m_LogSize;

		bool					m_OptimiseLLVM;
};

} // namespace Firtree

#endif // __FIRTREE_COMPILER_H 

// vim:sw=4:ts=4:cindent:noet
