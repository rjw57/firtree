//===========================================================================
/// \file glsl-target.cpp Interface to LLVM-based glsl-target.

#include "glsl-target.h"

#include <llvm_frontend.h>

#include "llvm/ADT/hash_map"
#include "llvm/Support/InstVisitor.h"
#include "llvm/Support/CallSite.h"

#include <iostream>
#include <sstream>

using namespace llvm;

namespace Firtree
{
	
//===========================================================================
// Visitor class for GLSL.
// 
// When we visit each instruction, we immediately skip if it has zero
// uses. This is because all the firtree functions are 'pure', i.e. they
// have no side effect.
//
// We similarly skip immediately writing instructions with one use since their
// value will be 'inlined' into their use.
// 
// This is except for instructions which clearly have side effects such
// as return statements.
class GLSLVisitor : public llvm::InstVisitor<GLSLVisitor>
{
	public:
		GLSLVisitor() 
			:	m_VisitingBasicBlock(false)
			,	m_VisitingFunctionDefinition(false)
			,	m_WaitingForEntryBasicBlock(false)
			,	m_DoInlining(true)
		{
		}

		~GLSLVisitor() { 
		}

		void runOnModule(Module& M)
		{
			visit(M);
			transitionToEnd();
		}

		bool GetDoInlining() const 
		{
			return m_DoInlining;
		}

		void SetDoInlining(bool flag) 
		{
			m_DoInlining = flag;
		}

		const std::ostringstream& getOutputStream() const
		{
			return m_OutputStream;
		}

		void visitFunction(Function& F)
		{
			bool is_declaration = (F.size() == 0);
			transitionToFunction(!is_declaration);

			if(is_declaration)
			{
				// Function declaration
				m_OutputStream << "// Declaration of " << F.getName() << ".\n";
			} else {
				// Function definition
				m_OutputStream << "// Definition of " << F.getName() << ".\n";
			}

			// Write the function prototype
			writePrototype(F, m_OutputStream);

			if(is_declaration)
			{
				m_OutputStream << ";\n\n";
			} else {
				m_OutputStream << "\n{\n";
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
				m_OutputStream << "{\n";
			}
		}

		void visitInstruction(Instruction& I)
		{
			SetValueToGLSL(&I, "/* ??? */");
			m_OutputStream << "// ??? => " << I;
			FIRTREE_ERROR("Unknown instruction.");
		}

		void visitCallInst(CallInst &I)
		{
			if(I.use_empty()) { return; }

			Function* F = I.getCalledFunction();
			if((F == NULL) || !(F->hasName()))
			{
				FIRTREE_ERROR("NULL function call");
			}

			std::ostringstream glsl_rep;
			glsl_rep << F->getName() << "(";

			CallSite::arg_iterator it = I.op_begin()+1;
			for( ; it != I.op_end(); ++it)
			{
				if(it != I.op_begin()+1) {
					glsl_rep << ", ";
				} else {
					glsl_rep << " ";
				}

				writeOperand(*it, glsl_rep);
			}

			glsl_rep << " )";

			if(m_DoInlining && I.hasOneUse()) {
				SetValueToGLSL(&I, glsl_rep.str());
			} else {
				writeIndent();
				writeValueDecl(I);
				m_OutputStream << glsl_rep.str() << ";\n";
				SetValueToVariable(&I);
			}
		}

		void visitBinaryOperator(BinaryOperator &I)
		{
			if(I.use_empty()) { return; }

			std::ostringstream glsl_rep;

			glsl_rep << "(";
			writeOperand(I.getOperand(0), glsl_rep);
			glsl_rep << ") ";

			switch(I.getOpcode())
			{
				case Instruction::Add:
					glsl_rep << "+";
					break;
				case Instruction::Sub:
					glsl_rep << "-";
					break;
				case Instruction::Mul:
					glsl_rep << "*";
					break;
				case Instruction::FDiv:
					glsl_rep << "/";
					break;
				case Instruction::And:
					glsl_rep << "&&";
					break;
				case Instruction::Or:
					glsl_rep << "||";
					break;
				case Instruction::Xor:
					glsl_rep << "^^";
					break;
				default:
					FIRTREE_ERROR("Unknown binary op (%i)", I.getOpcode());
					break;
			}

			glsl_rep << " (";
			writeOperand(I.getOperand(1), glsl_rep);
			glsl_rep << ")";

			if(m_DoInlining && I.hasOneUse()) {
				SetValueToGLSL(&I, glsl_rep.str());
			} else {
				writeIndent();
				writeValueDecl(I);
				m_OutputStream << glsl_rep.str() << ";\n";
				SetValueToVariable(&I);
			}
		}

		void visitAllocaInst(AllocaInst &I)
		{
			if(I.use_empty()) { return; }

			const Type* deref_type = I.getType()->getElementType();

			writeIndent();
			writeType(deref_type, m_OutputStream);
			m_OutputStream << " " << valueNameToVarName(I.getName()) << ";\n";
			SetValueToVariable(&I);
		}

		void visitStoreInst(StoreInst &I)
		{
			// FIXME: Function parameters...
			
			if(!IsInVariable(I.getOperand(1)))
			{
				FIRTREE_ERROR("Attempt to store to non variable.");
			}

			writeIndent();
			writeOperand(I.getOperand(1), m_OutputStream);
			m_OutputStream << " = ";
			writeOperand(I.getOperand(0), m_OutputStream);
			m_OutputStream << ";\n";
		}

		void visitLoadInst(LoadInst &I)
		{
			if(I.use_empty()) { return; }

			if(!IsInVariable(I.getOperand(0)))
			{
				FIRTREE_ERROR("Attempt to store to non variable.");
			}

			std::ostringstream glsl_rep;
			writeOperand(I.getOperand(0), glsl_rep);

			if(m_DoInlining && I.hasOneUse()) {
				SetValueToGLSL(&I, glsl_rep.str());
			} else {
				writeIndent();
				writeValueDecl(I);
				m_OutputStream << glsl_rep.str() << ";\n";
				SetValueToVariable(&I);
			}
		}

		void visitSIToFP(SIToFPInst &I)
		{
			if(I.use_empty()) { return; }

			std::ostringstream glsl_rep;
			writeType(I.getType(), glsl_rep);
			glsl_rep << "( ";
			writeOperand(I.getOperand(0), glsl_rep);
			glsl_rep << " )";

			if(m_DoInlining && I.hasOneUse()) {
				SetValueToGLSL(&I, glsl_rep.str());
			} else {
				writeIndent();
				writeValueDecl(I);
				m_OutputStream << glsl_rep.str() << ";\n";
				SetValueToVariable(&I);
			}
		}

		void visitFPToSI(FPToSIInst &I)
		{
			if(I.use_empty()) { return; }

			std::ostringstream glsl_rep;
			writeType(I.getType(), glsl_rep);
			glsl_rep << "( ";
			writeOperand(I.getOperand(0), glsl_rep);
			glsl_rep << " )";

			if(m_DoInlining && I.hasOneUse()) {
				SetValueToGLSL(&I, glsl_rep.str());
			} else {
				writeIndent();
				writeValueDecl(I);
				m_OutputStream << glsl_rep.str() << ";\n";
				SetValueToVariable(&I);
			}
		}

		void visitReturnInst(ReturnInst &I)
		{
			std::ostringstream glsl_rep;
			glsl_rep << "return";

			if(I.getNumOperands() > 0) {
				glsl_rep << " ";
				writeOperand(I.getOperand(0), glsl_rep);
			}

			writeIndent();
			m_OutputStream << glsl_rep.str() << ";\n";
		}

		void visitInsertElementInst(InsertElementInst &I)
		{
			if(I.use_empty()) { return; }

			const VectorType* vec_type = cast<VectorType>(I.getType());
			if(vec_type == NULL)
			{
				FIRTREE_ERROR("Insert element must have vector type");
			}
			unsigned int output_arity = vec_type->getNumElements();

			// The third operand *must* be a constant integer
			// in range 0..3
			const llvm::Value* idx_val = I.getOperand(2);
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
			const llvm::Value* left_side = I.getOperand(0);

			if(!m_DoInlining || !I.hasOneUse())
			{
				// If we have one use, fall back to default implementation
				// FIXME: Remove duplication here
				writeIndent();
				writeValueDecl(I);
				writeOperand(left_side, m_OutputStream);
				m_OutputStream << ";\n";

				writeIndent();
				m_OutputStream << valueNameToVarName(I.getName());
				m_OutputStream << "." << idx_char[idx] << " = ";
				writeOperand(I.getOperand(1), m_OutputStream);
				m_OutputStream << ";\n";

				SetValueToVariable(&I);
			} else if(isa<ConstantVector>(left_side)) {
				// If the left operand is a constant vector, inline the
				// insertion.
				const ConstantVector* cv = cast<ConstantVector>(left_side);
				std::vector<std::string> vector_glsl_reps;
				for(unsigned int i=0; i<output_arity; ++i)
				{
					std::ostringstream element_rep;
					if(i == idx) {
						writeOperand(I.getOperand(1), element_rep);
					} else {
						writeOperand(cv->getOperand(i), element_rep);
					}
					vector_glsl_reps.push_back(element_rep.str());
				}

				SetValueToGLSLVector(&I, vector_glsl_reps);
			} else if(isa<ConstantAggregateZero>(left_side)) {
				// If the left operand is a constant zero, inline the
				// insertion.
				std::vector<std::string> vector_glsl_reps;
				for(unsigned int i=0; i<output_arity; ++i)
				{
					std::ostringstream element_rep;
					if(i == idx) {
						writeOperand(I.getOperand(1), element_rep);
					} else {
						element_rep << "0.0";
					}
					vector_glsl_reps.push_back(element_rep.str());
				}

				SetValueToGLSLVector(&I, vector_glsl_reps);
			} else if(m_VectorValueMap.count(left_side) != 0) {
				// If the left operand is a GLSL vector, inline the
				// insertion.
				std::vector<std::string> vector_glsl_reps;
				for(unsigned int i=0; i<output_arity; ++i)
				{
					std::ostringstream element_rep;
					if(i == idx) {
						writeOperand(I.getOperand(1), element_rep);
					} else {
						element_rep << m_VectorValueMap[left_side][i];
					}
					vector_glsl_reps.push_back(element_rep.str());
				}

				SetValueToGLSLVector(&I, vector_glsl_reps);
			} else {
				// FIXME: Remove duplication here
				writeIndent();
				writeValueDecl(I);
				writeOperand(left_side, m_OutputStream);
				m_OutputStream << ";\n";

				writeIndent();
				m_OutputStream << valueNameToVarName(I.getName());
				m_OutputStream << "." << idx_char[idx] << " = ";
				writeOperand(I.getOperand(1), m_OutputStream);
				m_OutputStream << ";\n";

				SetValueToVariable(&I);
			}
		}

		void visitShuffleVectorInst(ShuffleVectorInst &I)
		{
			// A shuffle instruction has only left-hand
			// indices, only right-hand indices or a mixture of
			// both. Work out which we are.
			
			const VectorType* val_vec_type = cast<VectorType>(I.getType());
			if(val_vec_type == NULL) {
				FIRTREE_ERROR("Shuffle instruction must return vector.");
			}

			llvm::Value* shuffle_indices = I.getOperand(2);
			const ConstantVector* vec_val =
				cast<ConstantVector>(shuffle_indices);
			if(vec_val == NULL)
			{
				FIRTREE_ERROR("Shuffle indices vector must be constant vector.");
			}

			unsigned int output_arity = val_vec_type->getNumElements();

			bool only_left = true;
			bool only_right = true;
			std::vector<uint64_t> out_indices;
			for(unsigned int i=0; i<output_arity; i++)
			{
				const Constant* element = vec_val->getOperand(i);
				if(!isa<ConstantInt>(element)) {
					FIRTREE_ERROR("Shuffle indices must be constant integer.");
				}
				const ConstantInt* ci_element = cast<ConstantInt>(element);
				uint64_t element_val = ci_element->getValue().getZExtValue();

				if(element_val < output_arity) {
					only_right = false;
				}

				if(element_val >= output_arity) {
					only_left = false;
				}

				out_indices.push_back(element_val);
			}

			static char idx_string[] = "xyzw";

			std::ostringstream glsl_rep;

			if(m_DoInlining && only_left) { 
				glsl_rep << "(";
				writeOperand(I.getOperand(0), glsl_rep);
				glsl_rep << ").";
				for(unsigned int i=0; i<output_arity; ++i) {
					glsl_rep << idx_string[out_indices[i]];
				}
			} else if(m_DoInlining && only_right) {
				glsl_rep << "(";
				writeOperand(I.getOperand(1), glsl_rep);
				glsl_rep << ").";
				for(unsigned int i=0; i<output_arity; ++i) {
					glsl_rep << idx_string[out_indices[i]-output_arity];
				}
			} else {
				// Write left and right into variables if necessary
				std::string left_var_name, right_var_name;
				const llvm::Value* left_value = I.getOperand(0);
				const llvm::Value* right_value = I.getOperand(1);

				if(!m_DoInlining || !IsVectorValue(left_value)) {
					std::ostringstream left_var_name_stream;
					if(IsInVariable(left_value))
					{
						writeOperand(left_value, left_var_name_stream);
						left_var_name = left_var_name_stream.str();
					} else {
						left_var_name_stream << " _left_" <<
							valueNameToVarName(I.getName());
						left_var_name = left_var_name_stream.str();

						writeIndent();
						writeType(left_value->getType(), m_OutputStream);
						m_OutputStream << left_var_name << " = ";
						writeOperand(left_value, m_OutputStream);
						m_OutputStream << ";\n";
					}
				}

				if(!m_DoInlining || !IsVectorValue(right_value)) {
					std::ostringstream right_var_name_stream;
					if(IsInVariable(right_value))
					{
						writeOperand(right_value, right_var_name_stream);
						right_var_name = right_var_name_stream.str();
					} else {
						right_var_name_stream << " _right_" << 
							valueNameToVarName(I.getName());
						right_var_name = right_var_name_stream.str();

						writeIndent();
						writeType(right_value->getType(), m_OutputStream);
						m_OutputStream << right_var_name << " = ";
						writeOperand(right_value, m_OutputStream);
						m_OutputStream << ";\n";
					}
				}

				writeType(I.getType(), glsl_rep);
				glsl_rep << "(";
				for(unsigned int i=0; i<output_arity; ++i) {
					if(i == 0) {
						glsl_rep << " ";
					} else {
						glsl_rep << ", ";
					}

					if(out_indices[i] < output_arity) {
						if(IsVectorValue(left_value))
						{
							glsl_rep << m_VectorValueMap[left_value][out_indices[i]];
						} else {
							glsl_rep << left_var_name << "."; 
							glsl_rep << idx_string[out_indices[i]];
						}
					} else {
						if(IsVectorValue(right_value))
						{
							glsl_rep << m_VectorValueMap[left_value]
								[out_indices[i] - output_arity];
						} else {
							glsl_rep << right_var_name << "."; 
							glsl_rep << idx_string[out_indices[i] - output_arity];
						}
					}
				}
				glsl_rep << " )";
			}
			
			if(m_DoInlining && I.hasOneUse()) {
				SetValueToGLSL(&I, glsl_rep.str());
			} else {
				writeIndent();
				writeValueDecl(I);
				m_OutputStream << glsl_rep.str() << ";\n";
				SetValueToVariable(&I);
			}
		}

		void visitExtractElementInst(ExtractElementInst &I)
		{
			if(I.use_empty()) { return; }

			std::ostringstream glsl_rep;

			// The second operand *must* be a constant integer
			// in range 0..3
			const llvm::Value* idx_val = I.getOperand(1);
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

			writeOperand(I.getOperand(0), glsl_rep);
			glsl_rep << "." << idx_char[idx];

			if(m_DoInlining && I.hasOneUse()) { 
				SetValueToGLSL(&I, glsl_rep.str());
			} else {
				writeIndent();
				writeValueDecl(I);
				m_OutputStream << glsl_rep.str() << ";\n";
				SetValueToVariable(&I);
			}
		}

	protected:
		// These flags store the current state
		bool	m_VisitingBasicBlock;
		bool	m_VisitingFunctionDefinition;
		bool	m_WaitingForEntryBasicBlock;
		std::ostringstream m_OutputStream;

		// This stores the mapping between a value and it's GLSL representation
		hash_map<const llvm::Value*, std::string> m_ValueMap;

		// This stores the mapping between a value and it's vector of GLSL 
		// representations,
		hash_map<const llvm::Value*, std::vector<std::string> > m_VectorValueMap;

		// This stores a flag indicating if the value is stored in a variable.
		// i.e. whether the m_ValueMap entry contains a variable name.
		hash_map<const llvm::Value*, bool> m_InVariable;

		// This stores a map between a value name and the variable name.
		hash_map<std::string, std::string> m_VarNameMap;

		// A flag indicating whether inlining optimisations should be performed
		bool m_DoInlining;

		void SetValueToGLSL(const llvm::Value* v, const std::string& glsl)
		{
			m_ValueMap[v] = glsl;
			m_InVariable[v] = false;
		}

		void SetValueToGLSLVector(const llvm::Value* v, 
				const std::vector<std::string>& glsl_vec)
		{
			// Form a GLSL reprentation from this vector
			if((glsl_vec.size() < 2) || (glsl_vec.size() > 4)) {
				FIRTREE_ERROR("Invalid GLSL vector size: %u.", glsl_vec.size());
			}

			std::ostringstream glsl_rep;
			writeType(v->getType(), glsl_rep);
			glsl_rep << "(";
			for(unsigned int i=0; i<glsl_vec.size(); ++i)
			{
				if(i != 0)
				{
					glsl_rep << ",";
				}
				glsl_rep << glsl_vec[i];
			}
			glsl_rep << ")";

			SetValueToGLSL(v, glsl_rep.str());
			m_VectorValueMap[v] = glsl_vec;
		}

		void SetValueToVariable(const llvm::Value* v)
		{
			m_ValueMap[v] = valueNameToVarName(v->getName());
			m_InVariable[v] = true;
		}

		bool KnowAboutValue(const llvm::Value* v)
		{
			return m_ValueMap.count(v) != 0;
		}

		bool IsInVariable(const llvm::Value* v)
		{
			return KnowAboutValue(v) && m_InVariable[v];
		}

		bool IsVectorValue(const llvm::Value* v)
		{
			return KnowAboutValue(v) && !m_InVariable[v] && 
				(m_VectorValueMap.count(v) != 0);
		}

		// Using the current state, handle transitioning between
		// different elements
		void transitionToBasicBlock()
		{
			// If we're already visiting a basic block,
			// close it.
			if(m_VisitingBasicBlock) {
				writeIndent();
				m_OutputStream << "}\n";
				m_VisitingBasicBlock = false;
			}

			m_VisitingBasicBlock = true;
		}

		void transitionToFunction(bool is_definition)
		{
			// If we're already visiting a basic block,
			// close it.
			if(m_VisitingBasicBlock) {
				m_OutputStream << "  }\n";
				m_VisitingBasicBlock = false;
			}

			// If we're already visiting a function, close it
			if(m_VisitingFunctionDefinition)
			{
				m_OutputStream << "}\n\n";
				m_VisitingFunctionDefinition = false;
			}

			m_VisitingFunctionDefinition = is_definition;
		}

		void transitionToEnd()
		{
			// If we're already visiting a basic block,
			// close it.
			if(m_VisitingBasicBlock) {
				m_OutputStream << "  }\n";
				m_VisitingBasicBlock = false;
			}

			// If we're already visiting a function, close it
			if(m_VisitingFunctionDefinition)
			{
				m_OutputStream << "}\n\n";
				m_VisitingFunctionDefinition = false;
			}	
		}

		void writeIndent()
		{
			m_OutputStream << "    ";
		}

		void writeConstant(const Constant* c, std::ostream& dest)
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

				writeType(vt, dest);
				dest << "(";

				User::const_op_iterator it = c->op_begin();
				bool is_start = true;
				for( ; it != c->op_end(); ++it)
				{
					if(!is_start)
					{
						dest << ", ";
					} else {
						dest << " ";
						is_start = false;
					}

					writeOperand(*it, dest);
				}

				dest << " )";
			} else if(isa<ConstantFP>(c)) {
				const ConstantFP* fpval = cast<ConstantFP>(c);
				dest << fpval->getValueAPF().convertToFloat();
			} else if(isa<ConstantInt>(c)) {
				const ConstantInt* intval = cast<ConstantInt>(c);
				switch(intval->getBitWidth())
				{
					case 32:
						dest << intval->getValue().getSExtValue();
						break;
					case 1:
						dest << 
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
						dest << "0.0";
						break;
					case Type::IntegerTyID:
						{
							const IntegerType* int_type = 
								cast<IntegerType>(c->getType());
							switch(int_type->getBitWidth())
							{
								case 32:
									dest << "0";
									break;
								case 1:
									dest << "false";
									break;
								default:
									FIRTREE_ERROR("Invalid zero integer.");
									break;
							}
						}
						break;
					case Type::VectorTyID:
						{
							writeType(c->getType(), dest);
							dest << "(";
							const VectorType* vt = 
								cast<VectorType>(c->getType());
							for(unsigned int i=0; i<vt->getNumElements(); i++)
							{
								if(i != 0) { dest << ","; }
								dest << "0.0";
							}
							dest << ")";
						}
						break;
					default:
						FIRTREE_ERROR("Invalid constant zero type.");
				}
			} else {
				FIRTREE_ERROR("Unknown constant type.");
			}
		}

		void writeOperand(const llvm::Value* v, std::ostream& dest)
		{
			if(isa<Constant>(v))
			{
				writeConstant(cast<Constant>(v), dest);
			} else if((isa<Instruction>(v)) || (isa<Argument>(v))) {
				// Should hopefully be in hash map
				if(KnowAboutValue(v))
				{
					dest << m_ValueMap[v];
				} else if(v->hasName()) {
					dest << valueNameToVarName(v->getName());
				} else {
					FIRTREE_ERROR("Anonymous value.");
				}
			} else {
				FIRTREE_ERROR("Unknown operand type.");
			}
		}

		void writeValueDecl(const llvm::Value& v)
		{
			// m_OutputStream << "/* uses = " << v.getNumUses() << " */ ";
			if(!v.hasName())
			{
				return;
			}

			writeType(v.getType(), m_OutputStream);
			m_OutputStream << " " << valueNameToVarName(v.getName()) << " = ";
		}

		void writeType(const Type* type, std::ostream& dest)
		{
			switch(type->getTypeID())
			{
				case Type::VoidTyID:
					dest << "void";
					break;
				case Type::FloatTyID:
					dest << "float";
					break;
				case Type::IntegerTyID:
					{
						const IntegerType* int_type = cast<IntegerType>(type);
						switch(int_type->getBitWidth())
						{
							case 32:
								dest << "int";
								break;
							case 1:
								dest << "bool";
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
								dest << "vec2";
								break;
							case 3:
								dest << "vec3";
								break;
							case 4:
								dest << "vec4";
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

		void writePrototype(Function& F, std::ostream& dest)
		{
			// Write the return type.
			writeType(F.getReturnType(), dest);
			dest << " ";

			// Write the function name
			dest << F.getName() << " ( ";

			Function::arg_iterator it = F.arg_begin();
			for( ; it != F.arg_end(); ++it)
			{
				if(it != F.arg_begin()) {
					dest << ", ";
				}

				writeType(it->getType(), dest);

				if(it->hasName())
				{
					dest << " " << it->getName();
				}
			}

			dest << " )";
		}

		std::string valueNameToVarName(std::string s)
		{
			return valueNameToVarName(s.c_str());
		}

		std::string valueNameToVarName(const char* str)
		{
			if(m_VarNameMap.count(str) == 0) {
				std::ostringstream new_name(std::ostringstream::out);
				new_name << "var" << m_VarNameMap.size();
				m_VarNameMap[str] = new_name.str();
			}

			return m_VarNameMap[str];
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
const std::string& GLSLTarget::ProcessModule(const llvm::Module* module, 
		bool optimize)
{
	GLSLVisitor visitor;

	llvm::Module* non_const_module = const_cast<llvm::Module*>(module);

	visitor.SetDoInlining(optimize);
	visitor.runOnModule(*non_const_module);

	m_CompiledGLSL = visitor.getOutputStream().str();

	return GetCompiledGLSL();
}

//===========================================================================
const std::string& GLSLTarget::GetCompiledGLSL() const
{
	return m_CompiledGLSL;
}

} // namespace Firtree

// vim:sw=4:ts=4:cindent:noet
