//===========================================================================
/// \file llvm_frontend.cc

#include "llvm_frontend.h"
#include "llvm_private.h"

#include <firtree/main.h>

#include "llvm_emit_decl.h"

using namespace llvm;

namespace Firtree
{

//===========================================================================
FullType FullType::FromQualiferAndSpecifier( firtreeTypeQualifier qual,
        firtreeTypeSpecifier spec )
{
	FullType rv;

	if ( qual == NULL ) {
		rv.Qualifier = FullType::TyQualNone;
	} else if ( firtreeTypeQualifier_none( qual ) ) {
		rv.Qualifier = FullType::TyQualNone;
	} else if ( firtreeTypeQualifier_const( qual ) ) {
		rv.Qualifier = FullType::TyQualConstant;
	} else if ( firtreeTypeQualifier_static( qual ) ) {
		rv.Qualifier = FullType::TyQualStatic;
	}

	if ( firtreeTypeSpecifier_float( spec ) ) {
		rv.Specifier = FullType::TySpecFloat;
	} else if ( firtreeTypeSpecifier_int( spec ) ) {
		rv.Specifier = FullType::TySpecInt;
	} else if ( firtreeTypeSpecifier_bool( spec ) ) {
		rv.Specifier = FullType::TySpecBool;
	} else if ( firtreeTypeSpecifier_vec2( spec ) ) {
		rv.Specifier = FullType::TySpecVec2;
	} else if ( firtreeTypeSpecifier_vec3( spec ) ) {
		rv.Specifier = FullType::TySpecVec3;
	} else if ( firtreeTypeSpecifier_vec4( spec ) ) {
		rv.Specifier = FullType::TySpecVec4;
	} else if ( firtreeTypeSpecifier_sampler( spec ) ) {
		rv.Specifier = FullType::TySpecSampler;
	} else if ( firtreeTypeSpecifier_color( spec ) ) {
		rv.Specifier = FullType::TySpecColor;
	} else if ( firtreeTypeSpecifier_void( spec ) ) {
		rv.Specifier = FullType::TySpecVoid;
	}

	return rv;
}

//===========================================================================
FullType FullType::FromFullySpecifiedType( firtreeFullySpecifiedType t )
{
	firtreeTypeQualifier qual = NULL;
	firtreeTypeSpecifier spec = NULL;

	if ( firtreeFullySpecifiedType_unqualifiedtype( t, &spec ) ) {
		/* nop */
	} else if ( firtreeFullySpecifiedType_qualifiedtype(
	                t, &qual, &spec ) ) {
		/* nop */
	}

	return FromQualiferAndSpecifier( qual, spec );
}

//===========================================================================
/// Convert this type to the matching LLVM type. Pass a LLVM context
/// which can be used for error reporting.
const llvm::Type* FullType::ToLLVMType( LLVMContext* ctx ) const
{
	switch ( Specifier ) {

		case TySpecFloat:
			return Type::FloatTy;

		case TySpecInt:

		case TySpecSampler:
			return Type::Int32Ty;

		case TySpecBool:
			return Type::Int1Ty;

		case TySpecVec2:
			return VectorType::get( Type::FloatTy, 2 );

		case TySpecVec3:
			return VectorType::get( Type::FloatTy, 3 );

		case TySpecVec4:

		case TySpecColor:
			return VectorType::get( Type::FloatTy, 4 );

		case TySpecVoid:
			return Type::VoidTy;

		default:
			FIRTREE_LLVM_ICE( ctx, NULL, "Unknown type." );
	}

	return NULL;
}

//===========================================================================
CompileErrorException::CompileErrorException( std::string message_str,
        const char* file, int line, const char* func,
        PT_Term term, bool is_ice )
		: Exception::Exception( message_str.c_str(), file, line, func )
		, m_IsIce( is_ice )
		, m_Term( term )
{
}

//===========================================================================
CompileErrorException::~CompileErrorException()
{
}

//===========================================================================
LLVMFrontend::LLVMFrontend( firtree top_level_term )
		: m_Status( true )
{
	// Create the context
	m_LLVMContext = new LLVMContext();

	m_LLVMContext->Backend = this;

	// Create the LLVM module.
	m_LLVMContext->Module = new Module( "kernel_module" );

	try {
		EmitDeclarations emit_decls( m_LLVMContext );

		// Start the compilation
		GLS_Lst( firtreeExternalDeclaration ) decls;

		if ( firtree_Start_TranslationUnit( top_level_term, &decls ) ) {
			emit_decls.emitDeclarationList( decls );

			emit_decls.checkEmittedDeclarations();
		} else {
			FIRTREE_LLVM_ICE( m_LLVMContext, top_level_term,
			                  "Program expected." );
		}
	} catch ( CompileErrorException e ) {
		HandleCompilerError( e );
	}
}

//===========================================================================
LLVMFrontend::~LLVMFrontend()
{
	// Commented out because throwing an error inside the Function
	// emission will trigger this too.
#if 0
	// Check that we don't think we're in the middle of a function/block.
	if ( m_LLVMContext->BB != NULL ) {
		FIRTREE_LLVM_ICE( m_LLVMContext, NULL,
		                  "Finished compilation within a basic block." );
	}

	if ( m_LLVMContext->Function != NULL ) {
		FIRTREE_LLVM_ICE( m_LLVMContext, NULL,
		                  "Finished compilation within a function." );
	}
#endif

	// Delete the LLVM module.
	delete m_LLVMContext->Module;

	// Delete the context.
	delete m_LLVMContext;
}

//===========================================================================
/// Retrieve the LLVM module constructed from compilation.
llvm::Module* LLVMFrontend::GetCompiledModule() const
{
	return m_LLVMContext->Module;
}

//===========================================================================
/// Retrieve the compilation success flag: true on success, false
/// otherwise.
bool LLVMFrontend::GetCompilationSucceeded() const
{
	return m_Status;
}

//===========================================================================
/// Throw a compiler exception.
void LLVMFrontend::ThrowCompileErrorException(
    const char* file, int line, const char* func,
    bool is_ice, PT_Term term,
    const char* format, ... )
{
	va_list args;

	// Work out how much space is required for the message.
	va_start( args, format );
	int message_length = vsnprintf( NULL, 0, format, args );
	va_end( args );

	// Allocate room for message.
	char* message = new char[message_length + 1];

	// Form the error message.
	va_start( args, format );
	vsnprintf( message, message_length + 1,
	           format, args );
	va_end( args );

	std::string message_str( message );

	delete message;

	// Throw exception.
	throw CompileErrorException( message_str, file,
	                             line, func, term, is_ice );
}

//===========================================================================
/// Handle a compiler error exception by recording it in the error
/// log.
void LLVMFrontend::HandleCompilerError(
    const CompileErrorException& error_exception )
{
	if ( error_exception.IsIce() ) {
		RecordMessage( error_exception.GetTerm(), true,
		               "error: ICE: %s @ %s:%s:%i",
		               error_exception.GetMessage().c_str(),
		               error_exception.GetFile().c_str(),
		               error_exception.GetFunction().c_str(),
		               error_exception.GetLine() );
	} else {
		RecordMessage( error_exception.GetTerm(), true,
		               "error: %s",
		               error_exception.GetMessage().c_str() );
	}
}

//===========================================================================
/// Record an error or warning in the log.
void LLVMFrontend::RecordWarning( PT_Term term, const char* format, ... )
{
	va_list args;

	// Work out how much space is required for the rest of the message.
	va_start( args, format );
	int message_length = vsnprintf( NULL, 0, format, args );
	va_end( args );

	// Allocate room for message.
	char* message = new char[message_length + 1];

	// Form the error message.
	va_start( args, format );
	vsnprintf( message, message_length + 1, format, args );
	va_end( args );

	RecordMessage( term, false, "warning: %s", message );

	delete message;
}

//===========================================================================
/// Record an error or warning in the log.
void LLVMFrontend::RecordMessage( PT_Term term, bool is_error,
                                 const char* format, ... )
{
	va_list args;

	// Work out how much space is required for the term location in the
	// message.
	int term_loc_length = 0;

	if (( term != NULL ) && ( PT_hasPos( term ) ) ) {
		term_loc_length = snprintf( NULL, 0, "%li:%li: ", PT_row( term ),
		                            PT_col( term ) );
	}

	// Work out how much space is required for the rest of the message.
	va_start( args, format );

	int message_length = vsnprintf( NULL, 0, format, args );

	va_end( args );

	// Allocate room for message.
	char* message = new char[term_loc_length + message_length + 1];

	if ( term_loc_length != 0 ) {
		snprintf( message, term_loc_length+1, "%li:%li: ", PT_row( term ),
		          PT_col( term ) );
	}

	// Form the error message.
	va_start( args, format );

	vsnprintf( message + term_loc_length, message_length + 1,
	           format, args );

	va_end( args );

	// record message.
	m_Log.push_back( message );

	// set error flag is necessary
	if ( is_error ) {
		m_Status = false;
	}

	delete message;
}

}

// vim:sw=4:ts=4:cindent:noet
