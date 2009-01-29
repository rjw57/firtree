#include <firtree/main.h>
#include <firtree/compiler/llvm_compiled_kernel.h>
#include <firtree/linker/sampler_provider.h>
#include <firtree/value.h>
#include "../targets/glsl/glsl-target.h"

#include "llvm/Bitcode/ReaderWriter.h"
#include "llvm/PassManager.h"
#include "llvm/Support/Streams.h"
#include "llvm/Assembly/PrintModulePass.h"

#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>

const char* kernel1_src = 
  "kernel vec4 testKernel(sampler src, float roff, float goff) {\n"
  "  vec4 incol = vec4(0,0,0,0);\n"
  "  for(int dx=-3;dx<=3;++dx) {\n"
  "    for(int dy=-3;dy<=3;++dy) {\n"
  "      incol += sample(src, samplerTransform(src, destCoord() + vec2(dx,dy)));\n"
  "    }\n"
  "  }\n"
  "  incol.rg += vec2(roff, goff);\n"
  "  return incol;\n"
  "}\n";

const char* kernel2_src = 
  "kernel vec4 testKernel(vec4 col) {\n"
  "  return vec4(1.0 + 0.5*sin(destCoord()*0.1),0,1);\n"
  "}\n";

void print_err_log(FILE* dest, Firtree::LLVM::CompiledKernel* kernel)
{
    const char* const* log = kernel->GetCompileLog();
    size_t i=0;
    while(log[i] != NULL) {
      fprintf(stderr, " > %s\n", log[i]);
      ++i;
    }
}

int main( int argc, const char* argv[] )
{
  Firtree::LLVM::CompiledKernel* kernel1(NULL);
  Firtree::LLVM::CompiledKernel* kernel2(NULL);
  Firtree::LLVM::SamplerProvider* kernel1_samp(NULL);
  Firtree::LLVM::SamplerProvider* kernel2_samp(NULL);
  Firtree::Value* val;
  Firtree::LLVM::SamplerLinker linker;
  Firtree::GLSLTarget* glsl_target = Firtree::GLSLTarget::Create();
  
  kernel1 = Firtree::LLVM::CompiledKernel::Create();
  //kernel1->SetDoOptimization(false);
  kernel1->Compile(&kernel1_src, 1);

  if(!kernel1->GetCompileStatus()) {
    fprintf(stderr, "1 ERROR:\n");
    print_err_log(stderr, kernel1);
    goto bail;
  }
 
  kernel2 = Firtree::LLVM::CompiledKernel::Create();
  //kernel2->SetDoOptimization(false);
  kernel2->Compile(&kernel2_src, 1);

  if(!kernel2->GetCompileStatus()) {
    fprintf(stderr, "2 ERROR:\n");
    print_err_log(stderr, kernel2);
    goto bail;
  }

  kernel1_samp = Firtree::LLVM::SamplerProvider::CreateFromCompiledKernel(kernel1);

  val = Firtree::Value::CreateFloatValue(0.5);
  kernel1_samp->SetParameterValue("goff", val);
  FIRTREE_SAFE_RELEASE(val);

  kernel2_samp = Firtree::LLVM::SamplerProvider::CreateFromCompiledKernel(kernel2);

  kernel1_samp->SetParameterSampler("src", kernel2_samp);

  if(linker.CanLinkSampler(kernel1_samp))
  {
    linker.LinkSampler(kernel1_samp);
    linker.GetModule()->dump();
    glsl_target->ProcessModule( linker.GetModule(), true );
    std::cout << glsl_target->GetCompiledGLSL();
  }

  FIRTREE_SAFE_RELEASE(kernel1);
  FIRTREE_SAFE_RELEASE(kernel2);
  FIRTREE_SAFE_RELEASE(kernel1_samp);
  FIRTREE_SAFE_RELEASE(kernel2_samp);
  FIRTREE_SAFE_RELEASE(glsl_target);

  return 0;

bail:
  FIRTREE_SAFE_RELEASE(kernel1);
  FIRTREE_SAFE_RELEASE(kernel2);
  FIRTREE_SAFE_RELEASE(kernel1_samp);
  FIRTREE_SAFE_RELEASE(kernel2_samp);
  FIRTREE_SAFE_RELEASE(glsl_target);
  return 1;
}

/* vim:sw=2:ts=2:cindent:et
 */
