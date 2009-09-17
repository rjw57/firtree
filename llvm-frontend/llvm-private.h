//===========================================================================
/// \file llvm-private.h Internal data structures for the LLVM code
/// generator.

#ifndef __LLVM_PRIVATE_H
#define __LLVM_PRIVATE_H

#include "llvm-frontend.h"

#include "common/common.h"

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

class LLVMFrontend;
struct LLVMContext;

//===========================================================================
/// \brief A structure decribing a parameter to a function.

struct FunctionParameter {
	/// The possible parameter qualifiers.
	enum ParameterQualifier {
		FuncParamIn,
		FuncParamOut,
		FuncParamInOut,
		FuncParamInvalid = -1,
	};

	/// The parse tree term which defined this parameter.
	firtreeParameterDeclaration Term;

	/// The type of the parameter
	FullType                    Type;

	/// The name of the parameter (as a symbol) or NULL if the
	/// parameter is anonymous.
	symbol                      Name;

	/// The direction qualifier for the parameter.
	ParameterQualifier          Direction;
};

//===========================================================================
/// \brief A structure decribing a prototype.

struct FunctionPrototype {
	/// The possible function qualifiers.
	enum FunctionQualifier {
		FunctionQualifierNone				= 0x00,
		FunctionQualifierKernel				= 0x01,
		FunctionQualifierReduce				= 0x02,
		FunctionQualifierRender				= 0x04,
		FunctionQualifierIntrinsic			= 0x08,
		FunctionQualifierStateful			= 0x10, // Function has side-effects.

		FuncQualInvalid = -1,
	};

	inline bool is_kernel() const { return Qualifier & FunctionQualifierKernel; }
	inline bool is_reduce_only() const { return Qualifier & FunctionQualifierReduce; }
	inline bool is_render_only() const { return Qualifier & FunctionQualifierRender; }
	inline bool is_intrinsic() const { return Qualifier & FunctionQualifierIntrinsic; }
	inline bool is_normal_function() const { return !is_kernel() && !is_intrinsic(); }
	inline bool is_stateful() const { return Qualifier & FunctionQualifierStateful; }

	/// The parse tree term which defined this prototype.
	firtreeFunctionPrototype    PrototypeTerm;

	/// The parse tree term containing the definition of this
	/// function. At the end of code generation, the list of function
	/// prototypes are swept and any non-intrinsic functions without
	/// a definition are reported as errors.
	firtreeFunctionDefinition   DefinitionTerm;

	/// The LLVM function associated with this prototype, or NULL if
	/// there is nont
	llvm::Function*             LLVMFunction;

	/// The function qualifiers
	unsigned int	            Qualifier;

	/// The name of the function (as a symbol).
	symbol                      Name;

	/// The return type of the function.
	FullType                    ReturnType;

	/// A vector of function parameters.
	std::vector<FunctionParameter>  Parameters;

	/// Return the mangled name for this function
	std::string GetMangledName( LLVMContext* context ) const;
};

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

	///					  The BasicBlock which is the entry block to the
	///                   function.
	llvm::BasicBlock*     EntryBB;

	///                   The backend which is generating the code.
	LLVMFrontend*         Backend;

	///                   The current symbol table for the function.
	SymbolTable*          Variables;

	///					  A pointer to the FunctionPrototype structure
	///					  associated with the current function.
	FunctionPrototype*	  CurrentPrototype;

	///					  A pointer to a list of KernelFunction structs
	///					  which should be populated with information on
	///                   the passed kernels. This can be NULL in which
	///                   case it is ignored.
	LLVM::KernelFunctionList* KernelList;

	/// A multimap between function identifier (as a symbol) and it's
	/// prototype. We use a multimap because we can have overloaded
	/// functions in Firtree (although we keep quiet about it!).
	std::multimap<symbol, FunctionPrototype>  FuncTable;

	/// Constructor defining default values.
	LLVMContext() : Module( NULL ), Function( NULL ), BB( NULL ) { }
};

#if (LLVM_MAJOR_VER > 2) || ((LLVM_MAJOR_VER == 2) && (LLVM_MINOR_VER >= 2))
#  define LLVM_AT_LEAST_2_2 1
#else
#  define LLVM_AT_LEAST_2_2 0
#endif
#if (LLVM_MAJOR_VER > 2) || ((LLVM_MAJOR_VER == 2) && (LLVM_MINOR_VER >= 3))
#  define LLVM_AT_LEAST_2_3 1
#else
#  define LLVM_AT_LEAST_2_3 0
#endif
#if (LLVM_MAJOR_VER > 2) || ((LLVM_MAJOR_VER == 2) && (LLVM_MINOR_VER >= 4))
#  define LLVM_AT_LEAST_2_4 1
#else
#  define LLVM_AT_LEAST_2_4 0
#endif

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
#if (LLVM_MAJOR_VER > 2) || ((LLVM_MAJOR_VER == 2) && (LLVM_MINOR_VER > 2))
# define LLVM_CREATE(type, ...) ( type::Create(__VA_ARGS__) )
#else
# define LLVM_CREATE(type, ...) ( new type(__VA_ARGS__) )
#endif

/// Use new Foo() idiom for versions of LLVM <= 2.3
#if LLVM_AT_LEAST_2_4
# define LLVM_NEW_2_3(type, ...) ( type::Create(__VA_ARGS__) )
#else
# define LLVM_NEW_2_3(type, ...) ( new type(__VA_ARGS__) )
#endif

} // namespace Firtree

#endif // __LLVM_PRIVATE_H 

// vim:sw=4:ts=4:cindent:noet
