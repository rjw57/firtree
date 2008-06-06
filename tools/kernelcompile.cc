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

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

void Usage()
{
   fprintf(stderr, "Usage: kernelcompile <kernel file>\n");
}

int main(int argc, char *argv[])
{
   if(argc != 2)
   {
      Usage();
      return 1;
   }

   const char* inFile = argv[1];
   FILE* pInFile = fopen(inFile, "rb");
   if(!pInFile)
   {
      fprintf(stderr, "Error openign file: %s.\n", inFile);
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
         Firtree::NullBackend be;
         Firtree::Compiler c(be);
         bool ret = c.Compile((const char**)(&buffer), 1);

         printf("Compiler info log:\n%s\n", c.GetInfoLog());
         if(!ret)
         {
            fprintf(stderr, "Error compiling shader.\n");
            return 3;
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
