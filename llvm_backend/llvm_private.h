//===========================================================================
/// \file llvm_private.h Internal data structures for the LLVM code
/// generator.

#ifndef __LLVM_PRIVATE_H
#define __LLVM_PRIVATE_H

#include "llvm_backend.h"

#include <firtree/main.h>

namespace Firtree
{

//===========================================================================
/// \brief An exception for a user-visible compilation error. Set the
/// 'is_ice' flag if the error is an Internal Compiler Errror and,
/// in principle, should never have been seen.
///
/// Errors are reported via exceptions so that the compilation progress
/// can be 'unwound' through the call chain to the point where it should
/// be re-started.

class CompileErrorException : public Exception
{

	public:
		CompileErrorException( std::string message_str,
		                       const char* file, int line, const char* func,
		                       PT_Term term, bool is_ice );
		virtual ~CompileErrorException();

		bool IsIce() const {
			return m_IsIce;
		}

		PT_Term GetTerm() const {
			return m_Term;
		}

	private:
		bool      m_IsIce;
		PT_Term   m_Term;
};

//===========================================================================
/// \brief A macro for asserting which calls FIRTREE_LLVM_ERROR( ) if
/// the assertion fails.
#define FIRTREE_LLVM_ASSERT(ctx, term, assertion) do { \
		if(!(assertion)) { \
			FIRTREE_LLVM_ICE(ctx, term, "Assertion failed: " #assertion); \
		} \
	} while(0)

//===========================================================================
/// \brief A macro for reporting compile warnings (i.e. warnings
/// which should be reported to user). 'term' is the parse-tree
/// terminal which contains the error or NULL if there is none.
#define FIRTREE_LLVM_WARNING(ctx, term, ...) do { \
		(ctx)->Backend->RecordWarning( term, __VA_ARGS__ ); \
	} while(0)

//===========================================================================
/// \brief A macro for reporting compile errors (i.e. errors
/// which should be reported to user). 'term' is the parse-tree
/// terminal which contains the error or NULL if there is none.
///
/// Under the covers this throws an exception and so the thread
/// of execution is unwound back to the exception handler.
#define FIRTREE_LLVM_ERROR(ctx, term, ...) do { \
		(ctx)->Backend->ThrowCompileErrorException( \
		        __FILE__, __LINE__, \
		        __PRETTY_FUNCTION__, false, term, __VA_ARGS__); \
	} while(0)

//===========================================================================
/// \brief A macro for reporting internal compiler errors (i.e. errors
/// which, in principle, should never be encountered).
#define FIRTREE_LLVM_ICE(ctx, term, ...) do { \
		(ctx)->Backend->ThrowCompileErrorException( \
		        __FILE__, __LINE__, \
		        __PRETTY_FUNCTION__, true, term, __VA_ARGS__); \
	} while(0)

class LLVMBackend;
struct FunctionPrototype;

//===========================================================================
/// \brief A structure defining the current LLVM context.
struct LLVMContext {
	///                   The current LLVM module we're inserting functions
	///                   into.
	llvm::Module*         Module;

	///                   The current LLVM function blocks are inserted into.
	llvm::Function*       Function;

	///                   The current BasicBlock instructions are inserted
	///                   into.
	llvm::BasicBlock*     BB;

	///                   The backend which is generating the code.
	LLVMBackend*          Backend;

	///                   The current symbol table for the function.
	SymbolTable*          Variables;

	///					  A pointer to the FunctionPrototype structure
	///					  associated with the current function.
	FunctionPrototype*	  CurrentPrototype;

	/// Constructor defining default values.
	LLVMContext() : Module( NULL ), Function( NULL ), BB( NULL ) { }
};

//===========================================================================
/// \brief A macro to create a LLVM object.
///
/// Prior to LLVM 2.3, object creation was done via new Foo(). Since
/// LLVM 2.3, the preferred method is Foo::Create(). This macro selects
/// the appropriate mechanism based upon the LLVM_MAJOR_VER and
/// LLVM_MINOR_VER macros.
///
/// FIXME: The way this macro is constructed *requires* that the constructor
/// takes arguments. Hell, it also requires variadic macro support but then
/// I'm just that sort of crazy mo-fo.
#if (LLVM_MAJOR_VER > 2) || (LLVM_MINOR_VER > 2)
# define LLVM_CREATE(type, ...) ( type::Create(__VA_ARGS__) )
#else
# define LLVM_CREATE(type, ...) ( new type(__VA_ARGS__) )
#endif

} // namespace Firtree

#endif // __LLVM_PRIVATE_H 

// vim:sw=4:ts=4:cindent:noet
