//===========================================================================
/// \file llvm-emit-constant.cc

#define __STDC_CONSTANT_MACROS



#include "llvm-frontend.h"
#include "llvm-private.h"
#include "llvm-expression.h"
#include "llvm-emit-constant.h"

#include <llvm/Instructions.h>
#include <llvm/DerivedTypes.h>

using namespace llvm;

namespace Firtree
{

//===========================================================================
ConstantExpressionValue::ConstantExpressionValue( LLVMContext* ctx,
        llvm::Value* val, Firtree::KernelTypeSpecifier ty_spec, bool is_static )
		: VoidExpressionValue( ctx )
		, m_WrappedValue( val )
		, m_TypeSpecifier( ty_spec )
		, m_IsStatic( is_static )
{
}

//===========================================================================
ConstantExpressionValue::~ConstantExpressionValue()
{
}

//===========================================================================
ExpressionValue* ConstantExpressionValue::Create( LLVMContext* ctx,
        llvm::Value* val, Firtree::KernelTypeSpecifier ty_spec, bool is_static )
{
	return new ConstantExpressionValue( ctx, val, ty_spec, is_static );
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
	return_value.Qualifier = 
		m_IsStatic ? Firtree::TyQualStatic : Firtree::TyQualConstant;

	return_value.Specifier = m_TypeSpecifier;
	return return_value;

#if 0
	const llvm::Type* type = GetLLVMValue()->getType();

	if ( type->getTypeID() == llvm::Type::VoidTyID ) {
		return_value.Specifier = Firtree::TySpecVoid;
	} else if ( type->getTypeID() == llvm::Type::FloatTyID ) {
		return_value.Specifier = Firtree::TySpecFloat;
	} else if ( llvm::isa<llvm::IntegerType>( type ) ) {
		const llvm::IntegerType* int_type =
		    llvm::cast<llvm::IntegerType>( type );
		if ( int_type->getBitWidth() == 32 ) {
			return_value.Specifier = Firtree::TySpecInt;
		} else if ( int_type->getBitWidth() == 1 ) {
			return_value.Specifier = Firtree::TySpecBool;
		} else {
			FIRTREE_LLVM_ICE( GetContext(), NULL,
			                  "unknown integer type." );
		}
	} else if ( llvm::isa<llvm::VectorType>( type ) ) {
		const llvm::VectorType* vec_type =
		    llvm::cast<llvm::VectorType>( type );
		switch ( vec_type->getNumElements() ) {
			case 2:
				return_value.Specifier = Firtree::TySpecVec2;
				break;
			case 3:
				return_value.Specifier = Firtree::TySpecVec3;
				break;
			case 4:
				return_value.Specifier = Firtree::TySpecVec4;
				break;
			default:
				FIRTREE_LLVM_ICE( GetContext(), NULL,
				                  "unknown vector type." );
		}
	} else {
		FIRTREE_LLVM_ICE( GetContext(), NULL, "unknown type." );
	}

	return return_value;
#endif
}

//===========================================================================
//===========================================================================

//===========================================================================
/// Utilitiy function to create a floating point immediate value.
ExpressionValue* CreateFloat( LLVMContext* context, float float_val )
{
#if LLVM_AT_LEAST_2_3
	llvm::Value* val = ConstantFP::get( Type::FLOAT_TY(context), (double)(float_val) );
#else
	llvm::Value* val = ConstantFP::get( Type::FLOAT_TY(context),
	                                    APFloat( float_val ) );
#endif
	return ConstantExpressionValue::Create( context, val, Firtree::TySpecFloat, true );
}

//===========================================================================
/// Utilitiy function to create an integer immediate value.
ExpressionValue* CreateInt( LLVMContext* context, int int_val )
{
	llvm::Value* val = ConstantInt::get( Type::INT32_TY(context), int_val );
	return ConstantExpressionValue::Create( context, val, Firtree::TySpecInt, true );
}

//===========================================================================
/// Utilitiy function to create a boolean immediate value.
ExpressionValue* CreateBool( LLVMContext* context, bool bool_val )
{
	llvm::Value* val = ConstantInt::get( Type::INT1_TY(context), ( int )bool_val );
	return ConstantExpressionValue::Create( context, val, Firtree::TySpecBool, true );
}

//===========================================================================
/// Utilitiy function to create a vector immediate value.
ExpressionValue* CreateVector( LLVMContext* context, const float* params,
                               int param_count )
{
	if (( param_count < 2 ) || ( param_count > 4 ) ) {
		FIRTREE_ERROR( "Invalid dimension for constant vector." );
	}

	std::vector<Constant*> elements;

	Firtree::KernelTypeSpecifier spec;
	switch(param_count) 
	{
		case 2:
			spec = Firtree::TySpecVec2;
			break;
		case 3:
			spec = Firtree::TySpecVec3;
			break;
		case 4:
			spec = Firtree::TySpecVec4;
			break;
		default:
			g_error("Unreachable");
			break;
	}

	for ( int i=0; i<param_count; i++ ) {
#if LLVM_AT_LEAST_2_3
		elements.push_back( ConstantFP::get( Type::FLOAT_TY(context), 
					(double)(params[i])));
#else
		elements.push_back( ConstantFP::get( Type::FLOAT_TY(context),
		                                     APFloat( params[i] ) ) );
#endif
	}

	for ( int i=param_count; i<4; i++ ) {
#if LLVM_AT_LEAST_2_3
		elements.push_back( ConstantFP::get( Type::FLOAT_TY(context), 0.0 ) );
#else
		elements.push_back( ConstantFP::get( Type::FLOAT_TY(context), APFloat( 0.0f ) ) );
#endif
	}


	llvm::Value* val = ConstantVector::get( elements );
	return ConstantExpressionValue::Create( context, val, spec, true );
}

//===========================================================================
ExpressionValue* CreateVector( LLVMContext* context,  float x,
                               float y )
{
	float p[] = {x, y};
	return CreateVector( context, p, 2 );
}

//===========================================================================
ExpressionValue* CreateVector( LLVMContext* context, float x,
                               float y, float z )
{
	float p[] = {x, y, z};
	return CreateVector( context, p, 3 );
}

//===========================================================================
ExpressionValue* CreateVector( LLVMContext* context, float x,
                               float y, float z, float w )
{
	float p[] = {x, y, z, w};
	return CreateVector( context, p, 4 );
}

//===========================================================================
ExpressionValue* CreateVector( LLVMContext* context,
                               std::vector<ExpressionValue*> values,
							   bool is_static)
{
	if (( values.size() < 2 ) || ( values.size() > 4 ) ) {
		FIRTREE_LLVM_ICE( context, NULL, "Invalid vector size." );
	}

	Firtree::KernelTypeSpecifier spec;
	switch(values.size()) 
	{
		case 2:
			spec = Firtree::TySpecVec2;
			break;
		case 3:
			spec = Firtree::TySpecVec3;
			break;
		case 4:
			spec = Firtree::TySpecVec4;
			break;
		default:
			g_error("Unreachable");
			break;
	}

	static const float zeros[] = {0.f, 0.f, 0.f, 0.f};

	ExpressionValue* zero_vec = CreateVector( context,
	                            zeros, values.size() );

	llvm::Value* vec_val = zero_vec->GetLLVMValue();

	for ( unsigned int i=0; i<values.size(); ++i ) {
#if LLVM_AT_LEAST_2_6
		vec_val = LLVM_CREATE_NO_CONTEXT(InsertElementInst, vec_val,
		                       values[i]->GetLLVMValue(), 
							   llvm::ConstantInt::get(
								   llvm::Type::INT32_TY(context),
								   i),
		                       "tmp", context->BB );
#else
		vec_val = LLVM_CREATE_NO_CONTEXT(InsertElementInst, vec_val,
		                       values[i]->GetLLVMValue(), i,
		                       "tmp", context->BB );
#endif
	}

	FIRTREE_SAFE_RELEASE( zero_vec );

	return ConstantExpressionValue::Create( context, vec_val, spec, is_static );
}

//===========================================================================
/// Utility function to create a set of ExpressionValues by cracking a
/// vector.
void CrackVector( LLVMContext* context, ExpressionValue* vector,
                  std::vector<ExpressionValue*>& output_values )
{
	FullType vec_type = vector->GetType();
	if ( !vec_type.IsVector() ) {
		FIRTREE_LLVM_ICE( context, NULL, "Cannot crack non-vector value." );
	}

	// Get the llvm vector value associated with the vector
	llvm::Value* vec_val = vector->GetLLVMValue();

	for ( unsigned int i=0; i<vec_type.GetArity(); i++ ) {
#if LLVM_AT_LEAST_2_6
		llvm::Value* ext_val = LLVM_CREATE_NO_CONTEXT(ExtractElementInst,
				vec_val, 
				llvm::ConstantInt::get(
					llvm::Type::INT32_TY(context),
					i),
				"tmp",
				context->BB );
#else
		llvm::Value* ext_val = new ExtractElementInst(
		                                    vec_val, i, "tmp",
		                                    context->BB );
#endif
		// Firtree vector components are always floats.
		output_values.push_back(
		    ConstantExpressionValue::Create( context, ext_val, TySpecFloat ) );
	}
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
			int ival = 0;
			const char* tok_str = GLS_Tok_string( token );

			// Parse integer
			ival = strtol( tok_str, ( char ** )NULL, 0 );

			return CreateInt(context, ival);
		}

		ExpressionValue* EmitFloat( LLVMContext* context,
		                            firtreeExpression expression,
		                            GLS_Tok token ) {
			float fval = static_cast<float>( atof( GLS_Tok_string(
			                                           token ) ) );

			return CreateFloat(context, fval);
		}

		ExpressionValue* EmitBool( LLVMContext* context,
		                           firtreeExpression expression,
		                           GLS_Tok token ) {
			int bval = ( strcmp( GLS_Tok_string( token ), "true" ) == 0 ) ?
			           1 : 0;

			return CreateBool(context, bval);
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
FIRTREE_LLVM_DECLARE_EMITTER(ConstantEmitter, int)
FIRTREE_LLVM_DECLARE_EMITTER(ConstantEmitter, float)
FIRTREE_LLVM_DECLARE_EMITTER(ConstantEmitter, bool)

}

// vim:sw=4:ts=4:cindent:noet
