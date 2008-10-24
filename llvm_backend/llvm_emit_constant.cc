//===========================================================================
/// \file llvm_expression.cc

#include <firtree/main.h>

#include "llvm_backend.h"
#include "llvm_private.h"
#include "llvm_expression.h"
#include "llvm_emit_constant.h"

#include <llvm/Instructions.h>

using namespace llvm;

namespace Firtree
{

//===========================================================================
ConstantExpressionValue::ConstantExpressionValue( LLVMContext* ctx,
        Value* val )
		: VoidExpressionValue( ctx )
		, m_WrappedValue( val )
{
}

//===========================================================================
ConstantExpressionValue::~ConstantExpressionValue()
{
}

//===========================================================================
ExpressionValue* ConstantExpressionValue::Create( LLVMContext* ctx,
        llvm::Value* val )
{
	return new ConstantExpressionValue( ctx, val );
}

//===========================================================================
llvm::Value* ConstantExpressionValue::GetLLVMValue() const
{
	return m_WrappedValue;
}

//===========================================================================
FullType ConstantExpressionValue::GetType() const
{
	FullType return_value;
	return_value.Qualifier = FullType::TyQualConstant;

	const llvm::Type* type = GetLLVMValue()->getType();

	if ( type->getTypeID() == llvm::Type::FloatTyID ) {
		return_value.Specifier = FullType::TySpecFloat;
	} else if ( llvm::isa<llvm::IntegerType>( type ) ) {
		const llvm::IntegerType* int_type =
		    llvm::cast<llvm::IntegerType>( type );
		if ( int_type->getBitWidth() == 32 ) {
			return_value.Specifier = FullType::TySpecInt;
		} else if ( int_type->getBitWidth() == 1 ) {
			return_value.Specifier = FullType::TySpecBool;
		} else {
			FIRTREE_LLVM_ICE( GetContext(), NULL,
			                  "unknown integer type." );
		}
	} else {
		FIRTREE_LLVM_ICE( GetContext(), NULL, "unknown type." );
	}

	return return_value;
}

//===========================================================================
//===========================================================================

//===========================================================================
/// \brief Class to emit an immediate constant.
class ConstantEmitter : ExpressionEmitter
{
	public:
		ConstantEmitter()
				: ExpressionEmitter() {
		}

		virtual ~ConstantEmitter() {
		}

		/// Create an instance of this emitter.
		static ExpressionEmitter* Create() {
			return new ConstantEmitter();
		}

		ExpressionValue* EmitInt( LLVMContext* context,
		                          firtreeExpression expression,
		                          GLS_Tok token ) {
			FIRTREE_LLVM_WARNING( context, expression,
			                      "FIXME: Hex and octal integer constants "
			                      "not supported." );
			int ival = atoi( GLS_Tok_string( token ) );
			llvm::Value* val = ConstantInt::get( Type::Int32Ty, ival );
			return ConstantExpressionValue::Create( context, val );
		}

		ExpressionValue* EmitFloat( LLVMContext* context,
		                            firtreeExpression expression,
		                            GLS_Tok token ) {
			float fval = static_cast<float>( atof( GLS_Tok_string(
			                                           token ) ) );
			llvm::Value* val = ConstantFP::get( Type::FloatTy,
			                                    APFloat( fval ) );
			return ConstantExpressionValue::Create( context, val );
		}

		ExpressionValue* EmitBool( LLVMContext* context,
		                           firtreeExpression expression,
		                           GLS_Tok token ) {
			int bval = ( strcmp( GLS_Tok_string( token ), "true" ) == 0 ) ?
			           1 : 0;
			llvm::Value* val = ConstantInt::get( Type::Int1Ty, bval );
			return ConstantExpressionValue::Create( context, val );
		}

		/// Emit the passed expression term returning a pointer to
		/// a ExpressionValue containing it's value. It is the
		/// responsibility of the caller to Release() this value.
		virtual ExpressionValue* Emit( LLVMContext* context,
		                               firtreeExpression expression ) {
			GLS_Tok token = NULL;
			if ( firtreeExpression_int( expression, &token ) ) {
				return EmitInt( context, expression, token );
			} else if ( firtreeExpression_float( expression, &token ) ) {
				return EmitFloat( context, expression, token );
			} else if ( firtreeExpression_bool( expression, &token ) ) {
				return EmitBool( context, expression, token );
			}

			FIRTREE_LLVM_ICE( context, expression, "Invalid constant." );

			return NULL;
		}
};

//===========================================================================
// Register the emitter.
RegisterEmitter<ConstantEmitter> g_ConstantEmitterIntReg( "int" );
RegisterEmitter<ConstantEmitter> g_ConstantEmitterFloatReg( "float" );
RegisterEmitter<ConstantEmitter> g_ConstantEmitterBoolReg( "bool" );

}

// vim:sw=4:ts=4:cindent:noet
