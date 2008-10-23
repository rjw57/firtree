//===========================================================================
/// \file llvm_expression.h Classes for outputting firtree expressions.
///
/// This file defines the interfaces for various objects which know how to
/// output LLVM for Firtree expressions.

#ifndef __LLVM_EXPRESSION_H
#define __LLVM_EXPRESSION_H

#include "gls.h"     // General Language Services
#include "symbols.h" // Datatype: Symbols

#include "llvm/DerivedTypes.h"

#include "../gen/firtree_int.h"

namespace Firtree
{

//===========================================================================
/// \brief A class encapsulating the 'value' of an expression.
///
/// Each expression has a certain value which has an associated type and
/// LLVM 'value'. In addition certain values are
/// 'lvalues' which means they can be assigned to. The object hides
/// the precise mechanism by which a value is assigned so that each
/// value object knows best how to assign itself.
///
/// This class provides an abstract base class from which value
/// implementations should be derrived.
class ExpressionValue : public ReferenceCounted
{
	protected:
		ExpressionValue() : ReferenceCounted() { }
		virtual ~ExpressionValue() { }

	public:
		/// Return the LLVM value associated with this value.
		virtual llvm::Value*	GetLLVMValue() const = 0;

		/// Return the Firtree type associated with this value.
		virtual FullType		GetType() const = 0;

		/// Return a flag which is true iff this value is a lvalue.
		virtual bool			IsMutable() const = 0;

		/// Assign the value from the passed ExpressionValue.
		virtual void	AssignFrom( const ExpressionValue& val ) const = 0;
};

//===========================================================================
/// \brief A void value.
///
/// A void value has no associated llvm::Value (calling GetLLVMValue()
/// results in an ICE), a type of void, is not mutable and raises an
/// ICE when AssignFrom() is called on it.
class VoidExpressionValue : public ExpressionValue
{
	protected:
		VoidExpressionValue( LLVMContext* ctx );
		virtual ~VoidExpressionValue();

		LLVMContext*	GetContext() const {
			return m_Context;
		}

	public:
		static VoidExpressionValue* Create( LLVMContext* ctx );

		virtual llvm::Value*	GetLLVMValue() const;
		virtual FullType		GetType() const;
		virtual bool			IsMutable() const;
		virtual void	AssignFrom( const ExpressionValue& val ) const;

	private:
		LLVMContext*	m_Context;
};

//===========================================================================
/// \brief Abstract base class for expression emitters.
///
/// Firtree has an internal 'plugin' style mechanism for outputting
/// expressions. Each statement in the body of a Firtree function is, in
/// effect, an expression. An implementer of this interface registers
/// their implementation via RegisterEmitter.
class ExpressionEmitter : public ReferenceCounted
{
	public:
		/// Create an instance of this emitter.
		inline static ExpressionEmitter* Create() {
			return NULL;
		}

		/// Emit the passed expression term returning a pointer to
		/// a ExpressionValue containing it's value. It is the
		/// responsibility of the caller to Release() this value.
		virtual ExpressionValue* Emit( LLVMContext* context,
		                               firtreeExpression expression ) = 0;
};

//===========================================================================
// Forward declaration.
class EmitterFactory;

//===========================================================================
/// \brief Registry of ExpressionEmitter-s.
///
/// Allows one to construct an ExpressionEmitter instance appropriate
/// for a Firtree firtreeExpression node. The registry is a singleton.
class ExpressionEmitterRegistry
{
	public:
		ExpressionEmitterRegistry();
		virtual ~ExpressionEmitterRegistry();

		/// Return a pointer to the singleton representing the
		/// global ExpressionEmitterRegistry.
		static ExpressionEmitterRegistry* GetRegistry();

		/// Return a ExpressionEmitter instance which can handle
		/// the term passed. Make sure to call Release() on the
		/// returned pointer. Returns NULL if a suitable
		/// emitter cannot be found but, in that case, also calls
		/// FIRTREE_LLVM_ICE and so will not return given the
		/// default implementation.
		ExpressionEmitter* CreateEmitterForTerm( LLVMContext* context,
		        firtreeExpression expr );

		/// Convenience wrapper around CreateEmitterForTerm() which
		/// calls Emit() on the result.
		ExpressionValue* Emit( LLVMContext* context,
				firtreeExpression expr );

	protected:
		/// Internal method to register an emitter factory with a
		/// particular production name.
		inline void RegisterFactory( std::string product,
		                             EmitterFactory* factory ) {
			m_RegisteredEmitters.insert(
			    std::pair<std::string, EmitterFactory*>(
			        product, factory ) );
		}

	private:
		std::map<std::string, EmitterFactory*>	m_RegisteredEmitters;

		template<typename Emitter>
		friend class RegisterEmitter;
};

class EmitterFactory
{
	public:
		virtual ExpressionEmitter* Create() = 0;
};

template<typename EmitterT>
class RegisterEmitter
{
	protected:
		class Factory : public EmitterFactory
		{
			public:
				virtual ExpressionEmitter* Create() {
					return EmitterT::Create();
				}
		};

		Factory			m_Factory;

	public:
		RegisterEmitter(const char *name) {
			ExpressionEmitterRegistry::GetRegistry()->RegisterFactory(
			    name, &m_Factory );
		}
};

} // namespace Firtree

#endif // __LLVM_EXPRESSION_H 

// vim:sw=4:ts=4:cindent:noet
