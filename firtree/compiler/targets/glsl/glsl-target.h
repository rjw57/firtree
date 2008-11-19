//===========================================================================
/// \file glsl-target.h LLVM GLSL target.

#ifndef __FIRTREE_GLSL_TARGET_H
#define __FIRTREE_GLSL_TARGET_H

#include <firtree/main.h>

namespace llvm { class Module; }

namespace Firtree
{

//===========================================================================
/// \brief GLSL target.
class GLSLTarget : public ReferenceCounted
{
	protected:
		GLSLTarget();
		virtual ~GLSLTarget();

	public:
		/// Create a new GLSLTarget. Call Release() to free it.
		static GLSLTarget* Create();

		/// Process the LLVM module passed.
		const std::string& ProcessModule(llvm::Module* module,
				bool optimize = true);

		/// Return a reference to the compiled GLSL
		const std::string& GetCompiledGLSL() const;

	private:
		std::string m_CompiledGLSL;
};

} // namespace Firtree

#endif // __FIRTREE_GLSL_TARGET_H 

// vim:sw=4:ts=4:cindent:noet
