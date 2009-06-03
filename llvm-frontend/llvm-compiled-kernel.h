//===========================================================================
/// \file llvm-compiled-kernel.h LLVM output from the firtree kernel language.
///
/// This file defines the interface to the LLVM output backend. The backend
/// also takes care of checking the well-formedness of the passed abstract
/// depth grammar.

#ifndef __LLVM_COMPILED_KERNEL_H
#define __LLVM_COMPILED_KERNEL_H

#include <common/common.h>

// STL templates
#include <vector>
#include <map>

#include "llvm/Module.h"

namespace Firtree
{

class LLVMFrontend;	

namespace LLVM 
{

//===========================================================================
/// \brief A structure decribing a kernel parameter.
///
/// A particular kernel function may take zero or more kernel parameters.
/// This structure describes them.
struct KernelParameter {
	/// The name of this parameter.
	std::string						Name;

	/// The type of this parameter.
	KernelTypeSpecifier				Type;

	/// A flag indicating whether this parameter is static.
	bool							IsStatic;
};

/// A vector of KernelParameters.
typedef std::vector<KernelParameter> KernelParameterList;

//===========================================================================
/// \brief A structure describing a kernel.
struct KernelFunction {
	/// The name of this kernel.
	std::string						Name;

	/// The LLVM function which represents this kernel.
	llvm::Function*					Function;

	/// A list of kernel parameters in the order they
	/// should be passed to the kernel function.
	KernelParameterList 			Parameters;

	/// Assignment operator (possibly overkill).
	inline const KernelFunction& operator = (const KernelFunction& f)
	{
		Name = f.Name; 
		Function = f.Function; 
		Parameters = f.Parameters;
		return *this;
	}
};

/// A list of KernelFunctions.
typedef std::vector<KernelFunction> KernelFunctionList;

//===========================================================================
/// \brief Main LLVM-based compiler interface.
class CompiledKernel : public ReferenceCounted
{
	protected:
		CompiledKernel();
		virtual ~CompiledKernel();

	public:
		/// A const iterator over the kernels. Each entry is a pair mapping
		/// kernel name to kernel function structure.
		typedef KernelFunctionList::const_iterator const_iterator;

		/// Create a new CompiledKernelFrontend. Call Release() to free
		/// it.
		static CompiledKernel* Create();

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

		/// @{
		/// \brief Control whether optimisation is performed.
		/// By default an optimisation pass over the compiled LLVM
		/// is performed.
		void SetDoOptimization(bool flag);
		bool GetDoOptimization() const;
		/// @}

		/// Get the compiled LLVM module.
		llvm::Module* GetCompiledModule() const;

		/// Get a reference to a vector of kernels defined by the compiled
		/// source.
		inline const KernelFunctionList& GetKernels() const
		{
			return m_KernelList;
		}

		/// Return an iterator pointing to the start of the kernel 
		/// function list.
		inline const_iterator begin() const { 
			return m_KernelList.begin();
		}

		/// Return an iterator pointing to the end of the kernel 
		/// function list.
		inline const_iterator end() const { 
			return m_KernelList.end();
		}

		/// Return an iterator pointing to the kernel function map
		/// entry with name 'name'. If no such function exists, returns
		/// the same iterator as end().
		inline const_iterator find(const std::string& name) const {
			for(const_iterator i=begin(); i!=end(); ++i)
			{
				if(i->Name == name) { return i; }
			}
			return end();
		}

	private:
		void RunOptimiser();

		LLVMFrontend*			m_CurrentFrontend;
		const char**			m_Log;
		uint32_t				m_LogSize;

		bool					m_OptimiseLLVM;
		bool					m_CompileStatus;

		KernelFunctionList		m_KernelList;
};

} // namespace Firtree::LLVM

} // namespace Firtree

#endif // __LLVM_COMPILED_KERNEL_H 

// vim:sw=4:ts=4:cindent:noet
