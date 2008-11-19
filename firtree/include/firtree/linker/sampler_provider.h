//===========================================================================
/// \file sampler_provider.h
///
/// This file defines the abstract base class all sampler providers should
/// derrive from.

#ifndef __SAMPLER_PROVIDER_H
#define __SAMPLER_PROVIDER_H

#include <firtree/main.h>

#include <vector>
#include <string>

namespace llvm { class Function; }

namespace Firtree { class Value; }

namespace Firtree { namespace LLVM {

class CompiledKernel;

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
class SamplerProvider : public ReferenceCounted
{
	protected:
		SamplerProvider();
		~SamplerProvider();

	public:
		/// Create a sampler provider from a compiled kernel. Note that
		/// the state of the compiled kernel is copied on creation so
		/// that the compiler may be re-used. This has the effect that
		/// subsequent changes to the CompiledKernel passed to this function
		/// have no effect on this provider.
		static SamplerProvider* CreateFromCompiledKernel(
				const CompiledKernel* kernel);

		/// Get the sample function. The returned value may be 
		/// invalidated after a static parameter is set.
		virtual const llvm::Function* GetSampleFunction() const = 0;

		/// Get the transform function. The returned value may be 
		/// invalidated after a static parameter is set.
		virtual const llvm::Function* GetTransformFunction() const = 0;

		/// Get the extent function. The returned value may be 
		/// invalidated after a static parameter is set.
		virtual const llvm::Function* GetExtentFunction() const = 0;

		/// Set the named parameter to the passed value. Throws an error
		/// if the parameter doesn't exist. Static parameters cause
		/// a re-compile.
		/// As a convenience this should return 'true' if a static
		/// parameter was changed.
		virtual bool SetParameterValue(const char* name,
				const Value* value) = 0;

		/// Get the value of the named parameter. Throws an error if
		/// the parameter doesn't exist. 
		///
		/// *The value should be Release()-ed when you are finished with it.
		virtual Value* GetParameterValue(const char* name) = 0;

		/// Return the type of the named parameter. Throws an error if
		/// the parameter doesn't exist. 
		virtual KernelTypeSpecifier GetParameterType(
				const char* name) const = 0;

		/// Return a flag which is true if the named parameter is static.
		virtual bool IsParameterStatic(const char* name) const = 0;

		/// Return a reference to a vector of parameter names.
		virtual const std::vector<std::string>& GetParameterNames() 
			const = 0;
	private:
};

} } // namespace Firtree::LLVM

#endif // __SAMPLER_PROVIDER_H 

// vim:sw=4:ts=4:cindent:noet
