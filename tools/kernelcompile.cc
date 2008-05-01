/*
 * Mesa 3-D graphics library
 * Version:  6.5.3
 *
 * Copyright (C) 1999-2007  Brian Paul, Tungsten Graphics, Inc.
 * All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * BRIAN PAUL BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
 * AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

/**
 * \mainpage
 *
 * Stand-alone Shading Language compiler.  
 * Basically, a command-line program which accepts GLSL shaders and emits
 * vertex/fragment programs (GPU instructions).
 *
 * This file is basically just a Mesa device driver but instead of building
 * a shared library we build an executable.
 *
 * We can emit programs in three different formats:
 *  1. ARB-style (GL_ARB_vertex/fragment_program)
 *  2. NV-style (GL_NV_vertex/fragment_program)
 *  3. debug-style (a slightly more sophisticated, internal format)
 *
 * Note that the ARB and NV program languages can't express all the
 * features that might be used by a fragment program (examples being
 * uniform and varying vars).  So, the ARB/NV programs that are
 * emitted aren't always legal programs in those languages.
 */


extern "C" {
#include "imports.h"
#include "context.h"
#include "extensions.h"
#include "framebuffer.h"
#include "shaders.h"
#include "shader/shader_api.h"
#include "shader/prog_print.h"
#include "shader/prog_parameter.h"
#include "drivers/common/driverfuncs.h"
#include "tnl/tnl.h"
#include "tnl/t_context.h"
#include "tnl/t_pipeline.h"
#include "swrast/swrast.h"
#include "swrast/s_context.h"
#include "swrast/s_triangle.h"
#include "swrast_setup/swrast_setup.h"
#include "vbo/vbo.h"
}

static const char *Prog = "glslcompiler";


struct options {
   GLboolean LineNumbers;
   gl_prog_print_mode Mode;
   const char *VertFile;
   const char *FragFile;
   const char *KernelFile;
   const char *OutputFile;
};

static struct options Options;


/**
 * GLSL compiler driver context. (kind of an artificial thing for now)
 */
struct compiler_context
{
   GLcontext MesaContext;
   int foo;
};

typedef struct compiler_context CompilerContext;



static void
UpdateState(GLcontext *ctx, GLuint new_state)
{
   /* easy - just propogate */
   _swrast_InvalidateState( ctx, new_state );
   _swsetup_InvalidateState( ctx, new_state );
   _tnl_InvalidateState( ctx, new_state );
   _vbo_InvalidateState( ctx, new_state );
}



static GLboolean
CreateContext(void)
{
   struct dd_function_table ddFuncs;
   GLvisual *vis;
   GLframebuffer *buf;
   GLcontext *ctx;
   CompilerContext *cc;

   vis = _mesa_create_visual(GL_TRUE, GL_FALSE, GL_FALSE, /* RGB */
                             8, 8, 8, 8,  /* color */
                             0, 0, 0,  /* z, stencil */
                             0, 0, 0, 0, 1);  /* accum */
   buf = _mesa_create_framebuffer(vis);

   cc = (CompilerContext*)(_mesa_calloc(sizeof(*cc)));
   if (!vis || !buf || !cc) {
      if (vis)
         _mesa_destroy_visual(vis);
      if (buf)
         _mesa_destroy_framebuffer(buf);
      return GL_FALSE;
   }

   _mesa_init_driver_functions(&ddFuncs);
   ddFuncs.GetString = NULL;/*get_string;*/
   ddFuncs.UpdateState = UpdateState;
   ddFuncs.GetBufferSize = NULL;

   ctx = &cc->MesaContext;
   _mesa_initialize_context(ctx, vis, NULL, &ddFuncs, cc);
   _mesa_enable_sw_extensions(ctx);

   if (!_swrast_CreateContext( ctx ) ||
       !_vbo_CreateContext( ctx ) ||
       !_tnl_CreateContext( ctx ) ||
       !_swsetup_CreateContext( ctx )) {
      _mesa_destroy_visual(vis);
      _mesa_free_context_data(ctx);
      _mesa_free(cc);
      return GL_FALSE;
   }
   TNL_CONTEXT(ctx)->Driver.RunPipeline = _tnl_run_pipeline;
   _swsetup_Wakeup( ctx );

   _mesa_make_current(ctx, buf, buf);

   return GL_TRUE;
}


static void
LoadAndCompileShader(GLuint shader, const char *text)
{
   GLint stat;
   _mesa_ShaderSourceARB(shader, 1, (const GLchar **) &text, NULL);
   _mesa_CompileShaderARB(shader);
   _mesa_GetShaderiv(shader, GL_COMPILE_STATUS, &stat);
   if (!stat) {
      GLchar log[1000];
      GLsizei len;
      _mesa_GetShaderInfoLog(shader, 1000, &len, log);
      fprintf(stderr, "%s: problem compiling shader: %s\n", Prog, log);
      exit(1);
   }
   else {
      printf("Shader compiled OK\n");
   }
}


/**
 * Read a shader from a file.
 */
static void
ReadShader(GLuint shader, const char *filename)
{
   const int max = 100*1000;
   int n;
   char *buffer = (char*) malloc(max);
   FILE *f = fopen(filename, "r");
   if (!f) {
      fprintf(stderr, "%s: Unable to open shader file %s\n", Prog, filename);
      exit(1);
   }

   n = fread(buffer, 1, max, f);
   /*
   printf("%s: read %d bytes from shader file %s\n", Prog, n, filename);
   */
   if (n > 0) {
      buffer[n] = 0;
      LoadAndCompileShader(shader, buffer);
   }

   fclose(f);
   free(buffer);
}


#if 0
static void
CheckLink(GLuint prog)
{
   GLint stat;
   _mesa_GetProgramiv(prog, GL_LINK_STATUS, &stat);
   if (!stat) {
      GLchar log[1000];
      GLsizei len;
      _mesa_GetProgramInfoLog(prog, 1000, &len, log);
      fprintf(stderr, "%s: Linker error:\n%s\n", Prog, log);
   }
   else {
      fprintf(stderr, "%s: Link success!\n", Prog);
   }
}
#endif

static const char*
DataTypeToString(GLenum Type)
{
   switch(Type)
   {
      case GL_FLOAT:
         return "float";
         break;
      case GL_FLOAT_VEC2:
         return "vec2";
         break;
      case GL_FLOAT_VEC3:
         return "vec3";
         break;
      case GL_FLOAT_VEC4:
         return "vec4";
         break;
      case GL_INT:
         return "int";
         break;
      case GL_INT_VEC2:
         return "ivec2";
         break;
      case GL_INT_VEC3:
         return "ivec3";
         break;
      case GL_INT_VEC4:
         return "ivec4";
         break;
      case GL_BOOL:
         return "bool";
         break;
      case GL_BOOL_VEC2:
         return "bvec2";
         break;
      case GL_BOOL_VEC3:
         return "bvec3";
         break;
      case GL_BOOL_VEC4:
         return "bvec4";
         break;
      case GL_FLOAT_MAT2:
         return "mat2";
         break;
      case GL_FLOAT_MAT3:
         return "mat3";
         break;
      case GL_FLOAT_MAT4:
         return "mat4";
         break;
      case GL_SAMPLER_1D:
         return "sampler1D";
         break;
      case GL_SAMPLER_2D:
         return "sampler2D";
         break;
      case GL_SAMPLER_3D:
         return "sampler3D";
         break;
      case GL_SAMPLER_CUBE:
         return "samplerCube";
         break;
      case GL_SAMPLER_1D_SHADOW:
         return "sampler1DShadow";
         break;
      case GL_SAMPLER_2D_SHADOW:
         return "sampler2DShadow";
         break;
      case GL_FLOAT_MAT2x3:
         return "mat2x3";
         break;
      case GL_FLOAT_MAT2x4:
         return "mat2x4";
         break;
      case GL_FLOAT_MAT3x2:
         return "mat3x2";
         break;
      case GL_FLOAT_MAT3x4:
         return "mat3x4";
         break;
      case GL_FLOAT_MAT4x2:
         return "mat4x2";
         break;
      case GL_FLOAT_MAT4x3:
         return "mat4x3";
         break;
   }

   return "????";
}

static const char*
RegisterTypeToString(enum register_file Type)
{
   switch(Type)
   {
      case PROGRAM_TEMPORARY:
         return "TEMPORARY"; 
         break;
      case PROGRAM_LOCAL_PARAM:
         return "LOCAL_PARAM"; 
         break;
      case PROGRAM_ENV_PARAM:
         return "ENV_PARAM"; 
         break;
      case PROGRAM_STATE_VAR:
         return "STATE_VAR"; 
         break;
      case PROGRAM_INPUT:
         return "INPUT"; 
         break;
      case PROGRAM_OUTPUT:
         return "OUTPUT"; 
         break;
      case PROGRAM_NAMED_PARAM:
         return "NAMED_PARAM"; 
         break;
      case PROGRAM_CONSTANT:
         return "CONSTANT"; 
         break;
      case PROGRAM_UNIFORM:
         return "UNIFORM"; 
         break;
      case PROGRAM_VARYING:
         return "VARYING"; 
         break;
      case PROGRAM_WRITE_ONLY:
         return "WRITE_ONLY"; 
         break;
      case PROGRAM_ADDRESS:
         return "ADDRESS"; 
         break;
      case PROGRAM_SAMPLER:
         return "SAMPLER"; 
         break;
      case PROGRAM_UNDEFINED:
         return "UNDEFINED"; 
         break;
   }

   return "????";
}

static void 
PrintParameter(struct gl_program_parameter* param, GLfloat value[4], FILE *f)
{
   fprintf(f, "#  - %s\t %s\t %s = (%f,%f,%f,%f)\t (%04x,%04x)\n",
         RegisterTypeToString(param->Type),
         DataTypeToString(param->DataType),
         (param->Name != NULL) ? param->Name : "(unnamed)",
         value[0], value[1], value[2], value[3],
         param->Type, param->DataType
         );
}

static void
PrintParameterList(gl_program_parameter_list* list, FILE* f)
{
   GLuint j;
   for(j = 0; j<list->NumParameters; j++)
   {
      struct gl_program_parameter* param = &(list->Parameters[j]);
      PrintParameter(param, list->ParameterValues[j], f);
   }
}

static void
PrintShaderParameters(GLuint shader, FILE *f)
{
   GET_CURRENT_CONTEXT(ctx);
   struct gl_shader *sh = _mesa_lookup_shader(ctx, shader);
   GLuint i;

   for (i = 0; i < sh->NumPrograms; i++) {
      struct gl_program *prog = sh->Programs[i];

      fprintf(f, "# Parameters for program %i (num = %i):\n", i,
            prog->Parameters->NumParameters);
      PrintParameterList(prog->Parameters, f);
      fprintf(f, "# Varying params for program %i (num = %i):\n", i,
            prog->Varying->NumParameters);
      PrintParameterList(prog->Varying, f);
      fprintf(f, "# Attributes for program %i (num = %i):\n", i,
            prog->Attributes->NumParameters);
      PrintParameterList(prog->Attributes, f);
   }
}


static void
PrintShaderInstructions(GLuint shader, FILE *f)
{
   GET_CURRENT_CONTEXT(ctx);
   struct gl_shader *sh = _mesa_lookup_shader(ctx, shader);
   GLuint i;

   for (i = 0; i < sh->NumPrograms; i++) {
      struct gl_program *prog = sh->Programs[i];
      _mesa_print_program_opt(prog, Options.Mode, Options.LineNumbers);
   }
}


static GLuint
CompileShader(const char *filename, GLenum type)
{
   GLuint shader;

   assert(type == GL_FRAGMENT_SHADER ||
          type == GL_VERTEX_SHADER ||
	  type == GL_KERNEL_SHADER_FIRTREE);

   shader = _mesa_CreateShader(type);
   ReadShader(shader, filename);

   return shader;
}


static void
Usage(void)
{
   printf("Mesa GLSL stand-alone compiler\n");
   printf("Usage:\n");
   printf("  --vs FILE          vertex shader input filename\n");
   printf("  --fs FILE          fragment shader input filename\n");
   printf("  --kernel FILE      kernel input filename\n");
   printf("  --arb              emit ARB-style instructions (the default)\n");
   printf("  --nv               emit NV-style instructions\n");
   printf("  --debug            emit debug-style instructions\n");
   printf("  --firtree          emit firtree-style instructions (default for kernel)\n");
   printf("  --number, -n       emit line numbers\n");
   printf("  --output, -o FILE  output filename\n");
   printf("  --help             display this information\n");
}


static void
ParseOptions(int argc, char *argv[])
{
   int i;

   Options.LineNumbers = GL_FALSE;
   Options.Mode = PROG_PRINT_ARB;
   Options.VertFile = NULL;
   Options.FragFile = NULL;
   Options.KernelFile = NULL;
   Options.OutputFile = NULL;

   if (argc == 1) {
      Usage();
      exit(0);
   }

   for (i = 1; i < argc; i++) {
      if (strcmp(argv[i], "--vs") == 0) {
         Options.VertFile = argv[i + 1];
         i++;
      }
      else if (strcmp(argv[i], "--fs") == 0) {
         Options.FragFile = argv[i + 1];
         i++;
      }
      else if (strcmp(argv[i], "--kernel") == 0) {
         Options.KernelFile = argv[i + 1];
         Options.Mode = PROG_PRINT_FIRTREE;
         i++;
      }
      else if (strcmp(argv[i], "--arb") == 0) {
         Options.Mode = PROG_PRINT_ARB;
      }
      else if (strcmp(argv[i], "--nv") == 0) {
         Options.Mode = PROG_PRINT_NV;
      }
      else if (strcmp(argv[i], "--debug") == 0) {
         Options.Mode = PROG_PRINT_DEBUG;
      }
      else if (strcmp(argv[i], "--firtree") == 0) {
         Options.Mode = PROG_PRINT_FIRTREE;
      }
      else if (strcmp(argv[i], "--number") == 0 ||
               strcmp(argv[i], "-n") == 0) {
         Options.LineNumbers = GL_TRUE;
      }
      else if (strcmp(argv[i], "--output") == 0 ||
               strcmp(argv[i], "-o") == 0) {
         Options.OutputFile = argv[i + 1];
         i++;
      }
      else if (strcmp(argv[i], "--help") == 0) {
         Usage();
         exit(0);
      }
      else {
         printf("Unknown option: %s\n", argv[i]);
         Usage();
         exit(1);
      }
   }
}


int
main(int argc, char *argv[])
{
   GLuint shader = 0;

   if (!CreateContext()) {
      fprintf(stderr, "%s: Failed to create compiler context\n", Prog);
      exit(1);
   }

   ParseOptions(argc, argv);

   if (Options.VertFile) {
      shader = CompileShader(Options.VertFile, GL_VERTEX_SHADER);
   }
   else if (Options.FragFile) {
      shader = CompileShader(Options.FragFile, GL_FRAGMENT_SHADER);
   }
   else if (Options.KernelFile) {
      shader = CompileShader(Options.KernelFile, GL_KERNEL_SHADER_FIRTREE);
   }

   if (shader) {
      if (Options.OutputFile) {
         fclose(stdout);
         /*stdout =*/ freopen(Options.OutputFile, "w", stdout);
      }
      if (stdout) {
         PrintShaderParameters(shader, stdout);
         PrintShaderInstructions(shader, stdout);
      }
      if (Options.OutputFile) {
         fclose(stdout);
      }
   }

   return 0;
}

// vim:cindent:sw=3:ts=3:et
