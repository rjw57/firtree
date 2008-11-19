//===========================================================================
/// \file llvm_swizzle.cc

#include <firtree/main.h>

#include "llvm_frontend.h"
#include "llvm_private.h"
#include "llvm_emit_decl.h"
#include "llvm_expression.h"
#include "llvm_type_cast.h"
#include "llvm_emit_constant.h"

#include <llvm/Instructions.h>

using namespace llvm;

namespace Firtree
{

//===========================================================================
/// \brief ExpressionValue sub-class which encapsulates a reference to a
/// swizzle and support assignment if possible.
class SwizzleExpressionValue : public ExpressionValue
{
	protected:
		SwizzleExpressionValue( LLVMContext* ctx,
		                        ExpressionValue* swizzlee,
		                        const std::vector<int>& swizzle_indices )
				: ExpressionValue()
				, m_Context( ctx )
				, m_Swizzlee( swizzlee )
				, m_SwizzleIndices( swizzle_indices ) {
			FIRTREE_SAFE_RETAIN( swizzlee );
		}
		virtual ~SwizzleExpressionValue() {
			FIRTREE_SAFE_RELEASE( m_Swizzlee );
		}

	public:
		static ExpressionValue* Create( LLVMContext* ctx,
		                                ExpressionValue* swizzlee,
		                                const std::vector<int>& swizzle_indices ) {
			return new SwizzleExpressionValue( ctx, swizzlee,
			                                   swizzle_indices );
		}

		/// Return the LLVM value associated with this value.
		virtual llvm::Value*	GetLLVMValue() const {
			if ( m_SwizzleIndices.size() == 1 ) {
				// just need to return a single value.
				llvm::Value* element_value =
				    LLVM_NEW_2_3( ExtractElementInst,
				                 m_Swizzlee->GetLLVMValue(),
				                 m_SwizzleIndices.front(),
				                 "tmp",
				                 m_Context->BB );
				return element_value;
			}

			// If we get this far, we have to return a vector.

			std::vector<Constant*> zero_vector;
			for ( unsigned int i=0; i<m_SwizzleIndices.size(); ++i ) {
#if	LLVM_AT_LEAST_2_3
				zero_vector.push_back( ConstantFP::get( Type::FloatTy, 0.0 ) );
#else
				zero_vector.push_back( ConstantFP::get( Type::FloatTy,
				                                        APFloat( 0.f ) ) );
#endif
			}

			llvm::Value* return_value = ConstantVector::get( zero_vector );

			llvm::Value* swizlee_val = m_Swizzlee->GetLLVMValue();
			for ( unsigned int i=0; i<m_SwizzleIndices.size(); ++i ) {
				llvm::Value* swizzlee_element =
				    LLVM_NEW_2_3( ExtractElementInst,
				                 swizlee_val,
				                 m_SwizzleIndices[i],
				                 "tmp",m_Context->BB );
				return_value =
				    LLVM_CREATE( InsertElementInst,
				                 return_value,
				                 swizzlee_element, i,
				                 "tmp",
				                 m_Context->BB );
			}

			return return_value;
		}

		/// Return the Firtree type associated with this value.
		virtual FullType		GetType() const {
			Firtree::KernelTypeQualifier type_qual =
			    IsMutable() ? Firtree::TyQualNone : Firtree::TyQualConstant;
			switch ( m_SwizzleIndices.size() ) {
				case 1:
					return FullType( type_qual, Firtree::TySpecFloat );
					break;
				case 2:
					return FullType( type_qual, Firtree::TySpecVec2 );
					break;
				case 3:
					return FullType( type_qual, Firtree::TySpecVec3 );
					break;
				case 4:
					return FullType( type_qual, Firtree::TySpecVec4 );
					break;
				default:
					// Handled by fall-through to returning an
					// invalid type below.
					break;
			}

			return FullType();
		}

		/// Return a flag which is true iff this value is a lvalue.
		virtual bool			IsMutable() const {
			return m_Swizzlee->IsMutable();
		}

		/// Assign the value from the passed ExpressionValue.
		virtual void	AssignFrom( const ExpressionValue& val ) const {
			ExpressionValue* assignment_value = NULL;

			if ( !IsMutable() ) {
				FIRTREE_LLVM_ICE( m_Context, NULL,
				                  "Attempt to assign to non mutable value." );
			}

			try {
				// Attempt to cast the assignment value to the right
				// type.
				assignment_value = TypeCaster::CastValue( m_Context,
				                   NULL, &val,
				                   GetType().Specifier );

				// Depending on assignment_value type, output
				// actual assignment code...
				llvm::Value* swizzlee = m_Swizzlee->GetLLVMValue();
				llvm::Value* assignment = assignment_value->GetLLVMValue();
				llvm::Value* new_val = NULL;
				if ( m_SwizzleIndices.size() == 1 ) {
					new_val = LLVM_CREATE( InsertElementInst,
					                       swizzlee,
					                       assignment,
					                       m_SwizzleIndices.front(),
					                       "tmp",
					                       m_Context->BB );
				} else {
					// Need to build value to assign from swizzlee.
					new_val = swizzlee;
					for ( unsigned int i=0; i<m_SwizzleIndices.size(); ++i ) {
						llvm::Value* assign_val =
						    LLVM_NEW_2_3( ExtractElementInst,
						                 assignment,
						                 i, "tmp",
						                 m_Context->BB );
						new_val =
						    LLVM_CREATE( InsertElementInst,
						                 new_val, assign_val,
						                 m_SwizzleIndices[i],
						                 "tmp",
						                 m_Context->BB );
					}
				}

				ExpressionValue* new_exp_val = ConstantExpressionValue::
				                               Create( m_Context, new_val );
				m_Swizzlee->AssignFrom( *new_exp_val );
				FIRTREE_SAFE_RELEASE( new_exp_val );

				FIRTREE_SAFE_RELEASE( assignment_value );
			} catch ( CompileErrorException e ) {
				FIRTREE_SAFE_RELEASE( assignment_value );
				throw e;
			}
		}

	private:
		LLVMContext*				m_Context;
		ExpressionValue*			m_Swizzlee;
		std::vector<int>			m_SwizzleIndices;
};

//===========================================================================
/// \brief Class to emit a return instruction.
class SwizzleEmitter : ExpressionEmitter
{
	public:
		SwizzleEmitter()
				: ExpressionEmitter() {
		}

		virtual ~SwizzleEmitter() {
		}

		/// Create an instance of this emitter.
		static ExpressionEmitter* Create() {
			return new SwizzleEmitter();
		}

		/// Emit the passed expression term returning a pointer to
		/// a ExpressionValue containing it's value. It is the
		/// responsibility of the caller to Release() this value.
		virtual ExpressionValue* Emit( LLVMContext* context,
		                               firtreeExpression expression ) {
			firtreeExpression swizzlee;
			GLS_Tok swizzle_spec_tok;

			if ( !firtreeExpression_fieldselect( expression,
			                                     &swizzlee,
			                                     &swizzle_spec_tok ) ) {
				FIRTREE_LLVM_ICE( context, expression, "Invalid swizzle." );
			}

			// Emit the code to calculate the swizzlee value.
			ExpressionValue* swizzlee_value =
			    ExpressionEmitterRegistry::GetRegistry()->Emit(
			        context, swizzlee );
			ExpressionValue* return_value = NULL;

			try {
				// The swizlee must be a vector.
				if ( !swizzlee_value->GetType().IsVector() ) {
					FIRTREE_LLVM_ERROR( context, swizzlee,
					                    "Invalid operand to swizzle." );
				}

				// Extract thw swizzle specification.
				const char* swizzle_spec_str =
				    GLS_Tok_string( swizzle_spec_tok );

				std::vector<int> swizzle_indices;
				for ( int c=0; swizzle_spec_str[c] != '\0'; ++c ) {
					switch ( swizzle_spec_str[c] ) {
						case 'x':
						case 'r':
						case 's':
							swizzle_indices.push_back( 0 );
							break;
						case 'y':
						case 'g':
						case 't':
							swizzle_indices.push_back( 1 );
							break;
						case 'z':
						case 'b':
						case 'p':
							swizzle_indices.push_back( 2 );
							break;
						case 'w':
						case 'a':
						case 'q':
							swizzle_indices.push_back( 3 );
							break;
						default:
							FIRTREE_LLVM_ERROR( context, expression,
							                    "Invalid character '%c' "
							                    "in swizzle specifier.",
							                    swizzle_spec_str[c] );
							break;
					}
				}

				if (( swizzle_indices.size() < 1 ) ||
				        ( swizzle_indices.size() > 4 ) ) {
					FIRTREE_LLVM_ERROR( context, expression,
					                    "Swizzle specifier has invalid "
					                    "size (%u).",
					                    swizzle_indices.size() );
				}

				return_value = SwizzleExpressionValue::
				               Create( context, swizzlee_value,
				                       swizzle_indices );

				FIRTREE_SAFE_RELEASE( swizzlee_value );

				return return_value;
			} catch ( CompileErrorException e ) {
				FIRTREE_SAFE_RELEASE( swizzlee_value );
				FIRTREE_SAFE_RELEASE( return_value );
				throw e;
			}

			return NULL;
		}
};

//===========================================================================
// Register the emitter.
FIRTREE_LLVM_DECLARE_EMITTER(SwizzleEmitter, fieldselect)

}

// vim:sw=4:ts=4:cindent:noet
