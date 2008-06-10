/* 
 * FIRTREE - A generic image processing system.
 * Copyright (C) 2008 Rich Wareham <srichwareham@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 as published by
 * the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program; if not, write to the Free Software Foundation, Inc., 51
 * Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include <compiler/include/compiler.h>
#include <compiler/include/main.h>

#include <compiler/backends/irdump/irdump.h>
#include <compiler/backends/glsl/glsl.h>

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

void Usage()
{
   fprintf(stderr, "Usage: kernelcompile [-b <irdump|glsl>] <kernel file>\n");
}

void printParameter(Firtree::GLSLBackend::Parameter& param)
{
   printf(" %s -> %s", param.humanName.c_str(), param.uniformName.c_str());
   printf(" (");
   switch(param.basicType)
   {
      case Firtree::GLSLBackend::Parameter::Int:
         printf("int");
         break;
      case Firtree::GLSLBackend::Parameter::Float:
         printf("float");
         break;
      case Firtree::GLSLBackend::Parameter::Bool:
         printf("bool");
         break;
      case Firtree::GLSLBackend::Parameter::Sampler:
         printf("sampler");
         break;
      default:
         printf("????");
         break;
   }
   printf(" x %i", param.vectorSize);
   if(param.isColor) {
      printf(" [color]");
   }
   printf(")\n");
}

enum Backend {
   IRDump,
   GLSL,
};

int main(int argc, char *argv[])
{
   if(argc == 1)
   {
      Usage();
      return 1;
   }

   int index = 1;
   const char* inFile = NULL;
   Backend backend = GLSL;

   while((index < argc) && (argv[index][0] == '-'))
   {
      switch(argv[index][1])
      {
         case 'b':
            index++;
            if(index<argc)
            {
               if(0 == strcmp(argv[index], "irdump")) {
                  backend = IRDump;
               } else if(0 == strcmp(argv[index], "glsl")) {
                  backend = GLSL;
               } else {
                  fprintf(stderr, "Unknown backend: %s\n", argv[index]);
                  Usage();
                  return 1;
               }
            } else {
               fprintf(stderr, "Option -b requires an argument\n");
               Usage();
               return 1;
            }
            break;
         default:
            fprintf(stderr, "unknown option: %c\n", argv[index][1]);
            Usage();
            return 1;
      }

      index++;
   }

   if(index<argc)
   {
      inFile = argv[index];
   }

   if(inFile == NULL)
   {
      Usage();
      return 1;
   }

   FILE* pInFile = fopen(inFile, "rb");
   if(!pInFile)
   {
      fprintf(stderr, "Error opening file: %s.\n", inFile);
      return 2;
   }

   const int maxLen = 100*1000;
   int n;
   char* buffer = (char*) malloc(maxLen);
   n = fread((void*)buffer, 1, maxLen, pInFile);
   if(n > 0)
   {
      buffer[n] = 0;

      try {
         Firtree::Backend* be = NULL;

         switch(backend)
         {
            case GLSL:
               be = new Firtree::GLSLBackend("BLOCK");
               break;
            case IRDump:
               be = new Firtree::IRDumpBackend(stdout);
               break;
         }

         Firtree::Compiler c(*be);
         bool ret = c.Compile((const char**)(&buffer), 1);

         printf("Compiler info log:\n%s\n", c.GetInfoLog());
         if(!ret)
         {
            fprintf(stderr, "Error compiling shader.\n");
            return 3;
         } else {
            if(backend == GLSL)
            {
               printf("Compiled GLSL:\n%s\n", 
                     reinterpret_cast<Firtree::GLSLBackend*>(be)->GetOutput());

               printf("Input parameters:\n");
               Firtree::GLSLBackend::Parameters& params = 
                  reinterpret_cast<Firtree::GLSLBackend*>(be)->GetInputParameters();
               for(Firtree::GLSLBackend::Parameters::iterator i = params.begin();
                     i != params.end(); i++)
               {
                  printParameter(*i);
               }
            }
         }
      } catch(Firtree::Exception e) {
         fprintf(stderr, "Exception caught: %s\n", e.GetMessage().c_str());
      }
   }
   free(buffer);

   fclose(pInFile);

   return 0;
}

// vim:cindent:sw=3:ts=3:et
