//===========================================================================
/// \file glsl-target.cpp Interface to LLVM-based glsl-target.

#include "glsl-target.h"

#include "../llvm_frontend.h"

#include "llvm/Support/InstVisitor.h"
#include "llvm/Support/CallSite.h"

#include <iostream>

using namespace llvm;

namespace Firtree
{
	
//===========================================================================
// Visitor class for GLSL.
class GLSLVisitor : public llvm::InstVisitor<GLSLVisitor>
{
	public:
		GLSLVisitor() 
			:	m_VisitingBasicBlock(false)
			,	m_VisitingFunctionDefinition(false)
			,	m_WaitingForEntryBasicBlock(false)
		{
		}

		~GLSLVisitor() { 
		}

		void runOnModule(Module& M)
		{
			visit(M);
			transitionToEnd();
		}

		void visitFunction(Function& F)
		{
			bool is_declaration = (F.size() == 0);
			transitionToFunction(!is_declaration);

			if(is_declaration)
			{
				// Function declaration
				std::cout << "// Declaration of " << F.getName() << ".\n";
			} else {
				// Function definition
				std::cout << "// Definition of " << F.getName() << ".\n";
			}

			// Write the function prototype
			writePrototype(F);

			if(is_declaration)
			{
				std::cout << ";\n\n";
			} else {
				std::cout << "\n{\n";
				m_WaitingForEntryBasicBlock = true;
			}
		}

		void visitBasicBlock(BasicBlock& BB)
		{
			transitionToBasicBlock();

			if(m_WaitingForEntryBasicBlock)
			{
				m_WaitingForEntryBasicBlock = false;
				m_VisitingBasicBlock = false;
			} else {
				writeIndent();
				std::cout << "{\n";
			}
		}

		void visitInstruction(Instruction& I)
		{
			std::cout << "// ??? => " << I;
			FIRTREE_ERROR("Unknown instruction.");
		}

		void visitCallInst(CallInst &I)
		{
			writeIndent();
			writeValueDecl(I);

			Function* F = I.getCalledFunction();
			if((F == NULL) || !(F->hasName()))
			{
				FIRTREE_ERROR("NULL function call");
			}
			std::cout << F->getName() << "(";

			CallSite::arg_iterator it = I.op_begin()+1;
			for( ; it != I.op_end(); ++it)
			{
				if(it != I.op_begin()+1) {
					std::cout << ", ";
				} else {
					std::cout << " ";
				}

				writeOperand(*it);
			}

			std::cout << " );\n";
		}

		void visitBinaryOperator(BinaryOperator &I)
		{
			writeIndent();
			writeValueDecl(I);

			std::string opstring;

			switch(I.getOpcode())
			{
				case Instruction::Add:
					opstring = "+";
					break;
				case Instruction::Sub:
					opstring = "-";
					break;
				case Instruction::Mul:
					opstring = "*";
					break;
				case Instruction::FDiv:
					opstring = "/";
					break;
				case Instruction::And:
					opstring = "&&";
					break;
				case Instruction::Or:
					opstring = "||";
					break;
				case Instruction::Xor:
					opstring = "^^";
					break;
				default:
					FIRTREE_ERROR("Unknown binary op (%i)", I.getOpcode());
					break;
			}

			writeOperand(I.getOperand(0));
			std::cout << " " << opstring << " ";
			writeOperand(I.getOperand(1));

			std::cout << ";\n";
		}

		void visitReturnInst(ReturnInst &I)
		{
			writeIndent();
			std::cout << "return ";
			writeOperand(I.getOperand(0));
			std::cout << ";\n";
		}

		void visitInsertElementInst(InsertElementInst &I)
		{
			if(!I.hasName())
			{
				// Skip, it has no effect
				return;
			}

			writeIndent();
			writeValueDecl(I);
			writeOperand(I.getOperand(0));
			std::cout << ";\n";

			// The third operand *must* be a constant integer
			// in range 0..3
			const Value* idx_val = I.getOperand(2);
			if(!isa<ConstantInt>(idx_val)) {
				FIRTREE_ERROR("Insert element instruction's third "
						"operand must be constant integer.");
			}

			const ConstantInt *ci_val = cast<ConstantInt>(idx_val);
			int64_t idx = ci_val->getValue().getSExtValue();
			if((idx < 0) || (idx > 3)) {
				FIRTREE_ERROR("Insert element index mut be in range "
						"0 .. 3.\n");
			}

			static char idx_char[] = "xyzw";

			writeIndent();
			std::cout << varNameSanitize(I.getName());
			std::cout << "." << idx_char[idx] << " = ";
			writeOperand(I.getOperand(1));
			std::cout << ";\n";
		}

		void visitExtractElementInst(ExtractElementInst &I)
		{
			writeIndent();
			writeValueDecl(I);
			writeOperand(I.getOperand(0));

			// The second operand *must* be a constant integer
			// in range 0..3
			const Value* idx_val = I.getOperand(1);
			if(!isa<ConstantInt>(idx_val)) {
				FIRTREE_ERROR("Extract element instruction's second "
						"operand must be constant integer.");
			}

			const ConstantInt *ci_val = cast<ConstantInt>(idx_val);
			int64_t idx = ci_val->getValue().getSExtValue();
			if((idx < 0) || (idx > 3)) {
				FIRTREE_ERROR("Extract element index mut be in range "
						"0 .. 3.\n");
			}

			static char idx_char[] = "xyzw";
			std::cout << "." << idx_char[idx] << ";\n";
		}

	protected:
		// These flags store the current state
		bool	m_VisitingBasicBlock;
		bool	m_VisitingFunctionDefinition;
		bool	m_WaitingForEntryBasicBlock;

		// Using the current state, handle transitioning between
		// different elements
		void transitionToBasicBlock()
		{
			// If we're already visiting a basic block,
			// close it.
			if(m_VisitingBasicBlock) {
				writeIndent();
				std::cout << "}\n";
				m_VisitingBasicBlock = false;
			}

			m_VisitingBasicBlock = true;
		}

		void transitionToFunction(bool is_definition)
		{
			// If we're already visiting a basic block,
			// close it.
			if(m_VisitingBasicBlock) {
				std::cout << "  }\n";
				m_VisitingBasicBlock = false;
			}

			// If we're already visiting a function, close it
			if(m_VisitingFunctionDefinition)
			{
				std::cout << "}\n\n";
				m_VisitingFunctionDefinition = false;
			}

			m_VisitingFunctionDefinition = is_definition;
		}

		void transitionToEnd()
		{
			// If we're already visiting a basic block,
			// close it.
			if(m_VisitingBasicBlock) {
				std::cout << "  }\n";
				m_VisitingBasicBlock = false;
			}

			// If we're already visiting a function, close it
			if(m_VisitingFunctionDefinition)
			{
				std::cout << "}\n\n";
				m_VisitingFunctionDefinition = false;
			}	
		}

		void writeIndent()
		{
			std::cout << "    ";
		}

		void writeConstant(const Constant* c)
		{
			if(isa<ConstantVector>(c))
			{
				const VectorType* vt = cast<VectorType>(c->getType());
				if(vt->getElementType()->getTypeID() != Type::FloatTyID)
				{
					FIRTREE_ERROR("Unknown const vector type.");
				}

				if((vt->getNumElements() < 2) || (vt->getNumElements() > 4))
				{
					FIRTREE_ERROR("Vector type has invalid arity (%u).",
							vt->getNumElements());
				}

				writeType(vt);
				std::cout << "(";

				User::const_op_iterator it = c->op_begin();
				bool is_start = true;
				for( ; it != c->op_end(); ++it)
				{
					if(!is_start)
					{
						std::cout << ", ";
					} else {
						std::cout << " ";
						is_start = false;
					}

					writeOperand(*it);
				}

				std::cout << " )";
			} else if(isa<ConstantFP>(c)) {
				const ConstantFP* fpval = cast<ConstantFP>(c);
				std::cout << fpval->getValueAPF().convertToFloat();
			} else if(isa<ConstantInt>(c)) {
				const ConstantInt* intval = cast<ConstantInt>(c);
				switch(intval->getBitWidth())
				{
					case 32:
						std::cout << intval->getValue().getSExtValue();
						break;
					case 1:
						std::cout << 
							((intval->getValue().getZExtValue()) ? 
							"true" : "false");
						break;
					default:
						FIRTREE_ERROR("Invalid constant in bitwidth (%i).",
								intval->getBitWidth());
				}
			} else if(isa<ConstantAggregateZero>(c)) {
				switch(c->getType()->getTypeID())
				{
					case Type::FloatTyID:
						std::cout << "0.0";
						break;
					case Type::IntegerTyID:
						{
							const IntegerType* int_type = 
								cast<IntegerType>(c->getType());
							switch(int_type->getBitWidth())
							{
								case 32:
									std::cout << "0";
									break;
								case 1:
									std::cout << "false";
									break;
								default:
									FIRTREE_ERROR("Invalid zero integer.");
									break;
							}
						}
						break;
					case Type::VectorTyID:
						{
							writeType(c->getType());
							std::cout << "(";
							const VectorType* vt = 
								cast<VectorType>(c->getType());
							for(unsigned int i=0; i<vt->getNumElements(); i++)
							{
								if(i != 0) { std::cout << ","; }
								std::cout << "0.0";
							}
							std::cout << ")";
						}
						break;
					default:
						FIRTREE_ERROR("Invalid constant zero type.");
				}
			} else {
				FIRTREE_ERROR("Unknown constant type.");
			}
		}

		void writeOperand(const Value* v)
		{
			if(isa<Constant>(v))
			{
				writeConstant(cast<Constant>(v));
			} else if((isa<Instruction>(v)) || (isa<Argument>(v))) {
				if(v->hasName())
				{
					std::cout << varNameSanitize(v->getName());
				} else {
					FIRTREE_ERROR("Anonymous value.");
				}
			} else {
				FIRTREE_ERROR("Unknown operand type.");
			}
		}

		void writeValueDecl(Value& v)
		{
			if(!v.hasName())
			{
				return;
			}

			writeType(v.getType());
			std::cout << " " << varNameSanitize(v.getName()) << " = ";
		}

		void writeType(const Type* type)
		{
			switch(type->getTypeID())
			{
				case Type::VoidTyID:
					std::cout << "void";
					break;
				case Type::FloatTyID:
					std::cout << "float";
					break;
				case Type::IntegerTyID:
					{
						const IntegerType* int_type = cast<IntegerType>(type);
						switch(int_type->getBitWidth())
						{
							case 32:
								std::cout << "int";
								break;
							case 1:
								std::cout << "bool";
								break;
							default:
								FIRTREE_ERROR("Unknown integer type.");
								break;
						}
					}
					break;
				case Type::VectorTyID:
					{
						const VectorType* vec_type = cast<VectorType>(type);
						if(vec_type->getElementType()->getTypeID()
								!= Type::FloatTyID)
						{
							FIRTREE_ERROR("Unknown vector element type.");
						}
						switch(vec_type->getNumElements())
						{
							case 2:
								std::cout << "vec2";
								break;
							case 3:
								std::cout << "vec3";
								break;
							case 4:
								std::cout << "vec4";
								break;
							default:
								FIRTREE_ERROR("Unknown vector arity.");
								break;
						}
					}
					break;
				default:
					FIRTREE_ERROR("Unknown type.");
					break;
			}
		}

		void writePrototype(Function& F)
		{
			// Write the return type.
			writeType(F.getReturnType());
			std::cout << " ";

			// Write the function name
			std::cout << F.getName() << " ( ";

			Function::arg_iterator it = F.arg_begin();
			for( ; it != F.arg_end(); ++it)
			{
				if(it != F.arg_begin()) {
					std::cout << ", ";
				}

				writeType(it->getType());

				if(it->hasName())
				{
					std::cout << " " << it->getName();
				}
			}

			std::cout << " )";
		}

		std::string varNameSanitize(std::string s)
		{
			return varNameSanitize(s.c_str());
		}

		std::string varNameSanitize(const char* str)
		{
			std::string output;

			for( ; *str != '\0'; ++str) {
				const char c = *str;
				if((c >= '0') && (c <= '9')) {
					output += c;
				} else if((c >= 'A') && (c <= 'Z')) {
					output += c;
				} else if((c >= 'a') && (c <= 'z')) {
					output += c;
				} else {
					static char fstr[32];
					snprintf(fstr, 32, "_%X_", c);
					output += fstr;
				}
			}

			return output;
		}
};

//===========================================================================
GLSLTarget::GLSLTarget()
	:	ReferenceCounted()
{
}

//===========================================================================
GLSLTarget::~GLSLTarget()
{
}

//===========================================================================
GLSLTarget* GLSLTarget::Create()
{
	return new GLSLTarget();
}

//===========================================================================
void GLSLTarget::ProcessModule(llvm::Module* module)
{
	if(module == NULL)
	{
		return;
	}

	GLSLVisitor visitor;
	visitor.runOnModule(*module);
}

} // namespace Firtree

// vim:sw=4:ts=4:cindent:noet
