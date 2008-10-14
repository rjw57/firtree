#include "llvmout_priv.h"
#include "llvmutil.h"

#include "stdosx.h"  // General Definitions (for gcc)
#include "ptm_gen.h" // General Parsing Routines
#include "ptm_pp.h"  // Pretty Printer
#include "gls.h"     // General Language Services
#include "hmap.h"    // Datatype: Finite Maps
#include "symbols.h" // Datatype: Symbols

#include "llvm/Module.h"
#include "llvm/DerivedTypes.h"
#include "llvm/Constants.h"
#include "llvm/Instructions.h"
#include "llvm/Bitcode/ReaderWriter.h"

#include "gen/firtree_int.h" // grammar interface
#include "gen/firtree_lim.h" // scanner table
#include "gen/firtree_pim.h" // parser  table

#include <iostream>
#include <assert.h>

using namespace llvm;

//==========================================================================
// Return true if a function matching this prototype has been declared
// previously.
bool haveFunctionDeclared(llvm_context* ctx, firtreeFunctionPrototype proto,
    firtreeFunctionPrototype* existing_proto)
{
  firtreeFunctionQualifier qual;
  firtreeFullySpecifiedType type;
  GLS_Tok name;
  GLS_Lst(firtreeParameterDeclaration) params;

  if(!firtreeFunctionPrototype_functionprototype(proto, &qual, &type, &name, &params))
  {
    PANIC("unknown function prototype");
  }

  // Extract the name of the function from this prototype.
  std::string func_name(GLS_Tok_string(name));

  // Do we have any existing functions by that name?
  if(ctx->func_table.count(func_name) != 0)
  {
    // Must check each function for collision.
    std::multimap<std::string, firtreeFunctionPrototype>::const_iterator possible_collisions_it =
        ctx->func_table.find(func_name);
    for( ; possible_collisions_it != ctx->func_table.end(); possible_collisions_it++)
    {
      if(prototypesCollide(proto, possible_collisions_it->second))
      {
        if(existing_proto != NULL)
        {
          *existing_proto = possible_collisions_it->second;
        }
        return true;
      }
    }
  }

  return false;
}

//==========================================================================
// Record a function prototype in the global function table.
void declareFunction(llvm_context* ctx, firtreeFunctionPrototype proto)
{
  firtreeFunctionQualifier qual;
  firtreeFullySpecifiedType type;
  GLS_Tok name;
  GLS_Lst(firtreeParameterDeclaration) params;

  if(!firtreeFunctionPrototype_functionprototype(proto, &qual, &type, &name, &params))
  {
    PANIC("unknown function prototype");
  }

  // Extract the name of the function from this prototype.
  std::string func_name(GLS_Tok_string(name));

  std::cerr << "I: Declare function '" << func_name << "'.\n";

  // Do we have any existing functions by that name?
  if(haveFunctionDeclared(ctx, proto))
  {
    std::cerr << "E: Function '" << func_name << "' already exists with clashing prototype.\n";
    PANIC("existing prototype");
  }

  // Ensure we have no void parameter types
  GLS_Lst(firtreeParameterDeclaration) params_tail;
  GLS_FORALL(params_tail, params) {
    firtreeParameterDeclaration param_decl = 
      GLS_FIRST(firtreeParameterDeclaration, params_tail);
  
    firtreeTypeSpecifier spec;
    if(!firtreeParameterDeclaration_parameterdeclaration(param_decl, NULL, NULL, &spec, NULL))
    {
      PANIC("unknown param decl.");
    }

    if(firtreeTypeSpecifier_void(spec))
    {
      cerr << "E: Parameters to function cannot have void type.\n";
      PANIC("cannot have void type params.");
    }
  }

  // Record prototype
  ctx->func_table.insert(std::pair<std::string const, firtreeFunctionPrototype>(
        func_name, proto));
}

//==========================================================================
// Get a LLVM type corresponding to a particular firtree type.
const Type* getLLVMTypeForTerm(firtreeFullySpecifiedType type)
{
  firtreeTypeSpecifier spec;
  firtreeTypeQualifier qual = NULL;

  if(firtreeFullySpecifiedType_unqualifiedtype(type, &spec))
  { /* nop */ 
  } else if(firtreeFullySpecifiedType_qualifiedtype(type, &qual, &spec))
  { /* nop */
  } else {
    PANIC("unknown fully specified type.");
  }

  return getLLVMTypeForTerm(qual, spec);
}

const Type* getLLVMTypeForTerm(firtreeTypeQualifier qual, firtreeTypeSpecifier spec)
{
  if(firtreeTypeSpecifier_int(spec)) {
    return Type::Int32Ty;
  } else if(firtreeTypeSpecifier_void(spec)) {
    return Type::VoidTy;
  } else if(firtreeTypeSpecifier_bool(spec)) {
    return Type::Int1Ty;
  } else if(firtreeTypeSpecifier_float(spec)) {
    return Type::FloatTy;
  } else if(firtreeTypeSpecifier_sampler(spec)) {
    return Type::Int32Ty;
  } else if(firtreeTypeSpecifier_vec2(spec)) {
    return VectorType::get(Type::FloatTy, 2);
  } else if(firtreeTypeSpecifier_vec3(spec)) {
    return VectorType::get(Type::FloatTy, 3);
  } else if(firtreeTypeSpecifier_vec4(spec)) {
    return VectorType::get(Type::FloatTy, 4);
  } else if(firtreeTypeSpecifier_color(spec)) {
    return VectorType::get(Type::FloatTy, 4);
  } else {
    PANIC("unknown type");
  }
}

//==========================================================================
// Check if two types are equivalent.
bool typesAreEqual(firtreeFullySpecifiedType a, firtreeFullySpecifiedType b)
{
  firtreeTypeSpecifier a_spec, b_spec;
  firtreeTypeQualifier a_qual = NULL, b_qual = NULL;

  crack_fully_specified_type(a, &a_spec, &a_qual);
  crack_fully_specified_type(b, &b_spec, &b_qual);

  if(PT_product(a_spec) != PT_product(b_spec))
  {
    return false;
  }

  if((a_qual != NULL) && (b_qual != NULL))
  {
    if(PT_product(a_qual) != PT_product(b_qual))
    {
      return false;
    }
  }

  if((a_qual == NULL) && (b_qual != NULL) && !(firtreeTypeQualifier_none(b_qual)))
  {
    return false;
  }

  if((a_qual != NULL) && (b_qual == NULL) && !(firtreeTypeQualifier_none(a_qual)))
  {
    return false;
  }

  return true;
}

//==========================================================================
// Check whether two function prototypes 'clash', i.e. have the same name
// and same parameter types in same order.
bool prototypesCollide(firtreeFunctionPrototype a, firtreeFunctionPrototype b)
{
  // Extract info on both prototypes.
  firtreeFunctionQualifier a_qual;
  firtreeFullySpecifiedType a_type;
  GLS_Tok a_name;
  GLS_Lst(firtreeParameterDeclaration) a_params;

  if(!firtreeFunctionPrototype_functionprototype(a, &a_qual, &a_type, &a_name, &a_params))
  {
    PANIC("unknown function prototype");
  }

  firtreeFunctionQualifier b_qual;
  firtreeFullySpecifiedType b_type;
  GLS_Tok b_name;
  GLS_Lst(firtreeParameterDeclaration) b_params;

  if(!firtreeFunctionPrototype_functionprototype(b, &b_qual, &b_type, &b_name, &b_params))
  {
    PANIC("unknown function prototype");
  }

  // Simplest check, do they differ in name?
  if(strcmp(GLS_Tok_string(a_name), GLS_Tok_string(b_name)) != 0)
  {
    // They do.. no problem
    return false;
  }

  // Next check, do they differ in parameter count?
  if(GLS_LENGTH(a_params) != GLS_LENGTH(b_params))
  {
    return false;
  }

  // Now iterate through both parameter lists, looking for parameters which
  // differ.
  GLS_Lst(firtreeParameterDeclaration) a_tail = a_params, b_tail = b_params;

  while(!GLS_EMPTY(a_tail) && !GLS_EMPTY(b_tail))
  {
    firtreeParameterDeclaration a_param = GLS_FIRST(firtreeParameterDeclaration, a_tail);
    firtreeTypeQualifier a_type_qual;
    firtreeParameterQualifier a_param_qual;
    firtreeTypeSpecifier a_type_spec;
    firtreeParameterIdentifierOpt a_parm_ident;
    if(!firtreeParameterDeclaration_parameterdeclaration(a_param, &a_type_qual, &a_param_qual,
          &a_type_spec, &a_parm_ident))
    {
      PANIC("unknown parameter declaration");
    }

    firtreeParameterDeclaration b_param = GLS_FIRST(firtreeParameterDeclaration, b_tail);
    firtreeTypeQualifier b_type_qual;
    firtreeParameterQualifier b_param_qual;
    firtreeTypeSpecifier b_type_spec;
    firtreeParameterIdentifierOpt b_parm_ident;
    if(!firtreeParameterDeclaration_parameterdeclaration(b_param, &b_type_qual, &b_param_qual,
          &b_type_spec, &b_parm_ident))
    {
      PANIC("unknown parameter declaration");
    }

    // Check the product (vec4, vec3, etc) of the type specifiers to ensure they differ
    if(PT_product(a_type_spec) != PT_product(b_type_spec))
    {
      return false;
    }

    // FIXME: Do we allow differing type qualifiers? Prob not.

    a_tail = GLS_REST(firtreeParameterDeclaration, a_tail);
    b_tail = GLS_REST(firtreeParameterDeclaration, b_tail);
  }

  return true;
}

/* vim:sw=2:ts=2:cindent:et 
 */
