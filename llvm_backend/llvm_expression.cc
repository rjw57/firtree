//===========================================================================
/// \file llvm_expression.cc

#include <firtree/main.h>

#include "llvm_backend.h"
#include "llvm_private.h"
#include "llvm_expression.h"

using namespace llvm;

namespace Firtree
{

//===========================================================================
VoidExpressionValue::VoidExpressionValue( LLVMContext* ctx )
		: ExpressionValue()
		, m_Context( ctx )
{
}

//===========================================================================
VoidExpressionValue::~VoidExpressionValue()
{
}

//===========================================================================
VoidExpressionValue* VoidExpressionValue::Create( LLVMContext* ctx )
{
	return new VoidExpressionValue( ctx );
}

//===========================================================================
llvm::Value* VoidExpressionValue::GetLLVMValue() const
{
	FIRTREE_LLVM_ICE( GetContext(), NULL, "Attempt to use value of void "
	                  "value." );
	return NULL;
}

//===========================================================================
FullType VoidExpressionValue::GetType() const
{
	FullType type;
	type.Qualifier = FullType::TyQualNone;
	type.Specifier = FullType::TySpecVoid;
	return type;
}

//===========================================================================
bool VoidExpressionValue::IsMutable() const
{
	return false;
}

//===========================================================================
void VoidExpressionValue::AssignFrom( const ExpressionValue& val ) const
{
	FIRTREE_LLVM_ICE( GetContext(), NULL, "Attempt to assign an immutable "
	                  "value." );
}


//===========================================================================
//===========================================================================

//===========================================================================
ExpressionEmitterRegistry::ExpressionEmitterRegistry()
{
}

//===========================================================================
ExpressionEmitterRegistry::~ExpressionEmitterRegistry()
{
}

//===========================================================================
// Global variable holding ExpressionEmitterRegistry singleton.
ExpressionEmitterRegistry* g_ExpressionEmitterRegistrySingleton = NULL;

//===========================================================================
/// Return a pointer to the singleton representing the
/// global ExpressionEmitterRegistry.
ExpressionEmitterRegistry* ExpressionEmitterRegistry::GetRegistry()
{
	if ( g_ExpressionEmitterRegistrySingleton == NULL ) {
		g_ExpressionEmitterRegistrySingleton =
		    new ExpressionEmitterRegistry();
	}

	return g_ExpressionEmitterRegistrySingleton;
}

//===========================================================================
/// Return a ExpressionEmitter instance which can handle
/// the term passed. Make sure to call Release() on the
/// returned pointer. Returns NULL if a suitable
/// emitter cannot be found but, in that case, also calls
/// FIRTREE_LLVM_ICE and so will not return given the
/// default implementation.
ExpressionEmitter* ExpressionEmitterRegistry::CreateEmitterForTerm(
    LLVMContext* context, firtreeExpression expr )
{
	const char* expr_product = symbolToString(
	                               PT_product(( PT_Term )expr ) );
	std::map<std::string, EmitterFactory*>::iterator it =
	    m_RegisteredEmitters.find( expr_product );
	if ( it == m_RegisteredEmitters.end() ) {
		FIRTREE_LLVM_ICE( context, expr, "Unknown expression type '%s'.",
		                  expr_product );
	}
	return it->second->Create();
}

//===========================================================================
ExpressionValue* ExpressionEmitterRegistry::Emit( LLVMContext* context,
        firtreeExpression expr )
{
	ExpressionEmitter* emitter = CreateEmitterForTerm( context, expr );
	if ( emitter != NULL ) {
		try {
			ExpressionValue* val = emitter->Emit( context, expr );
			FIRTREE_SAFE_RELEASE( emitter );
			return val;
		} catch ( CompileErrorException e ) {
			// On error, release the emitter and pass the error up the
			// call chain.
			FIRTREE_SAFE_RELEASE( emitter );
			throw e;
		}
	}
	return NULL;
}

//===========================================================================
//===========================================================================

//===========================================================================
/// \brief Class to emit a nop
class NopEmitter : ExpressionEmitter
{
	public:
		NopEmitter()
				: ExpressionEmitter() {
		}

		virtual ~NopEmitter() {
		}

		/// Create an instance of this emitter.
		static ExpressionEmitter* Create() {
			return new NopEmitter();
		}

		/// Emit the passed expression term returning a pointer to
		/// a ExpressionValue containing it's value. It is the
		/// responsibility of the caller to Release() this value.
		virtual ExpressionValue* Emit( LLVMContext* context,
		                               firtreeExpression expression ) {
			if ( !firtreeExpression_nop( expression ) ) {
				FIRTREE_LLVM_ICE( context, expression, "Invalid nop." );
			}

			return VoidExpressionValue::Create( context );
		}
};

//===========================================================================
RegisterEmitter<NopEmitter> g_NopEmitterReg( "nop" );

}

// vim:sw=4:ts=4:cindent:noet
