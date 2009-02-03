//===========================================================================
/// \file sampler_provider.h
///
/// This file defines the abstract base class all sampler providers should
/// derrive from.

#ifndef __SAMPLER_PROVIDER_H
#define __SAMPLER_PROVIDER_H

#include <firtree/main.h>
#include <firtree/compiler/llvm_compiled_kernel.h>

#include <vector>
#include <string>

namespace llvm { class Function; }

namespace Firtree { 
	class Value; 
	class Image;
}

namespace Firtree { namespace LLVM {

//===========================================================================
/// \brief A Sampler Provider
///
/// A sampler provider represents a single sampler as a set of LLVM 
/// functions. They are:
///
///  Sample Function: A LLVM function which gets the output of the sampler.
///                   It is passed any non-static sampler parameters.
///  Transform Function: A LLVM function which transforms from pixel
///                      co-ordinates to those expected to be returned by
///                      the destCoord() function.
///  Extent Function: A LLVM function which returns the extent of the sampler
///                   as a vector of floating point values giving the
///                   origin x, origin y, width and height.
class SamplerProvider : public ReferenceCounted, private Uncopiable
{
	protected:
		SamplerProvider();
		~SamplerProvider();

	public:
		/// An iterator over the kernel parameters.
		typedef KernelParameterList::const_iterator const_iterator;

		/// Create a sampler provider from a compiled firtree kernel source.
		/// Note that the state of the compiled kernel is copied on creation
		/// so that the compiler may be re-used. This has the effect that
		/// subsequent changes to the CompiledKernel passed to this function
		/// have no effect on this provider.
		///
		/// If kernel_name is non-empty, the kernel with that name will
		/// be used, otherwise the first kernel defined in the source
		/// will be used.
		static SamplerProvider* CreateFromCompiledKernel(
				const CompiledKernel* kernel,
				const std::string& kernel_name = "");

		/// Create a sampler provider from a Firtree image.
		static SamplerProvider* CreateFromImage(const Image* image);

		/// Create a LLVM module which is the result of linking all the
		/// samplers into one large module.
		/// The caller now 'owns' the returned module and must delete it.
		llvm::Module* LinkSamplerModule();

		/// Return a const iterator pointing to the parameter named
		/// 'name'. If no such parameter exists, return the
		/// same iterator as end().
		virtual const_iterator find(const std::string& name) const = 0;

		/// Return an iterator pointing to the start of the parameter list.
		virtual const_iterator begin() const = 0;

		/// Return an iterator pointing to the end of the parameter list.
		virtual const_iterator end() const = 0;

		/// Return a flag indicating if the sampler is in a valid state.
		/// This usually means that all the static parameters have been
		/// set to some non-NULL value. The return values of 
		/// Get{.*}Function() are undefined if this flag is false.
		virtual bool IsValid() const = 0;

		/// Set the parameter pointed to by a given iterator to be
		/// associated with the sampler provider 'provider'. It is an 
		/// error for the parameter iterator to point to a non-sampler
		/// parameter.
		///
		/// The provider can be NULL in which case the semantic is to 'unset'
		/// the parameter and release any references associated with it.
		/// Throws an error is param == end().
		virtual void SetParameterSampler(const const_iterator& param,
				SamplerProvider* value) = 0;

		/// Get the sampler associated with the referenced parameter. 
		/// Returns NULL if the
		/// parameter doesn't exist or is unset. Note that, therefore,
		/// the return value *can* be NULL even if the parameter *does*
		/// exist. If the caller is interested in keeping the value long 
		/// term, it should be Retain()-ed.
		virtual SamplerProvider* GetParameterSampler(
				const const_iterator& param) const = 0;

		/// Set the parameter pointed to by a given iterator to the passed
		/// value. Static parameters can cause the value of 
		/// Get{.*}Function() to change. As a convenience this should 
		/// return 'true' if a static parameter was changed.
		/// The Value can be NULL in which case the semantic is to 'unset'
		/// the parameter and release any references associated with it.
		/// Throws an error is param == end().
		virtual bool SetParameterValue(const const_iterator& param,
				const Value* value) = 0;

		/// Get the value of the referenced parameter. Returns NULL if the
		/// parameter doesn't exist or is unset. Note that, therefore,
		/// the return value *can* be NULL even if the parameter *does*
		/// exist. If the caller is interested in keeping the value long 
		/// term, it should be Retain()-ed or Clone()-ed.
		virtual const Value* GetParameterValue(
				const const_iterator& param) const = 0;

		/// Convenience wrapper around SetParameterValue().
		inline bool SetParameterValue(const std::string& param_name,
				const Value* value) 
		{
			return this->SetParameterValue(this->find(param_name), value);
		}

		/// Convenience wrapper around GetParameterSampler().
		inline void SetParameterSampler(const std::string& param_name,
				SamplerProvider* value) 
		{
			this->SetParameterSampler(this->find(param_name), value);
		}

		/// Convenience wrapper around GetParameterValue().
		inline const Value* GetParameterValue(
				const std::string& param_name) const
		{
			return this->GetParameterValue(this->find(param_name));
		}

		/// Convenience wrapper around GetParameterSampler().
		inline SamplerProvider* GetParameterSampler(
				const std::string& param_name) const
		{
			return this->GetParameterSampler(this->find(param_name));
		}

	protected:
		/// Create a LLVM module which *only* has three exported functions:
		///
		/// vec4 ${prefix}Sample(vec2 coord, ... /* free vars */)
		/// vec2 ${prefix}Transform(vec2 coord)
		/// vec4 ${prefix}Extent()
		///
		/// The caller now 'owns' the returned module and must call 'delete'
		/// on it.
		virtual llvm::Module* CreateSamplerModule(
				const std::string& prefix) = 0;

		friend class SamplerLinker;
};

//===========================================================================
/// \brief Link multiple SamplerProviders into one module.
///
/// The linked module implements the linked sampler in one function
/// called 'kernel' which takes a single 2-dimensional vector of floats
/// to indicate which output pixel is required.
///
/// The module in addition has declarations for any intrinsic functions
/// used by the kernel and a set of 'freeparam' functions.
///
/// The freeparam functions implement a target agnostic way of accessing
/// the parameters of the kernel which may be set without recompiling the
/// kernel. There is a function called freeparam_v4 for example which takes
/// a parameter index and returns a 4-d vector of floats with the
/// parameter value. 
///
/// Individual targets may implement these in whichever manner they
/// see fit.
class SamplerLinker {
	public:
		SamplerLinker();
		~SamplerLinker();

		/// Return true is the passed sampler provider is suitable for 
		/// linking. A sampler is suitable if all of it's static
		/// parameters are set, all samplers have been associated with
		/// a sampler provider and each of the samplers is itself
		/// suitable for linking.
		bool CanLinkSampler(SamplerProvider* sampler);

		/// Link the passed sampler. Throws an error if the sampler is
		/// not suitable. Callers should check the sampler with
		/// CanLinkSampler() beforehand.
		void LinkSampler(SamplerProvider* sampler);

		/// Return the module that was linked by LinkSampler(). The
		/// linker retains ownership of the module and it will be 
		/// destroyed with the linker.
		inline llvm::Module* GetModule() const
		{
			return m_LinkedModule;
		}

		/// Like GetModule() except that ownership passes to the caller
		/// and the module will *not* be deleted when the linker is
		/// destroyed. Callers should release the module via 'delete' when
		/// they are finished with it.
		inline llvm::Module* ReleaseModule()
		{
			llvm::Module* tmp = m_LinkedModule;
			m_LinkedModule = NULL;
			Reset();
			return tmp;
		}

		/// Debug only: return a flag indicating if the LLVM optimiser
		/// is run over the linked module.
		inline bool GetDoOptimization() const { return m_RunOptimiser; }

		/// Debug only: set a flag indicating if the LLVM optimiser
		/// is run over the linked module.
		inline void SetDoOptimization(bool flag) {
			m_RunOptimiser = flag;
		}

	private:
		void Reset();

		void WriteFreeParamFuncs();
		void RunOptimiser();

		typedef std::pair<SamplerProvider*, std::string> ParamSpec;
		typedef std::vector<ParamSpec> ParamSpecList;
		typedef std::vector<SamplerProvider*> SamplerList;

		bool			m_RunOptimiser;
		llvm::Module*   m_LinkedModule;
		ParamSpecList   m_FreeParameters;
		SamplerList		m_SamplerTable;
};

} } // namespace Firtree::LLVM

#endif // __SAMPLER_PROVIDER_H 

// vim:sw=4:ts=4:cindent:noet
