// Firtree - A real-time image processing system.
// Copyright (C) 2009 Rich Wareham <richwareham@gmail.com>
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License along
// with this program; if not, write to the Free Software Foundation, Inc.,
// 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.

#define __STDC_LIMIT_MACROS
#define __STDC_CONSTANT_MACROS

#include <glib.h>

#include <fcntl.h>
#include <stdlib.h>
#include <string.h>

#include <iostream>
#include <fstream>

#include <firtree/firtree-kernel-priv.hpp>

//=============================================================================
static GOptionEntry opt_entries[] =
{
    { NULL }
};

//=============================================================================
void
free_lines(GPtrArray* line_array)
{
    for(guint i=0; i<line_array->len; ++i)
    {
        g_free(line_array->pdata[i]);
    }
    g_ptr_array_free(line_array, TRUE);
}

//=============================================================================
GPtrArray*
read_lines(int fd)
{
    GPtrArray* line_array = NULL;
    GIOChannel* io_channel = g_io_channel_unix_new(fd);

    if(io_channel == NULL) {
        return NULL;
    }

    /* create an array to hold the new lines */
    line_array = g_ptr_array_new();

    gchar* line_str;
    GError* error = NULL;
    GIOStatus status;
    while(G_IO_STATUS_NORMAL == (status = g_io_channel_read_line(
                    io_channel, &line_str, NULL, NULL, &error))) 
    {
        g_ptr_array_add(line_array, line_str);
    }

    if(status != G_IO_STATUS_EOF)
    {
        g_print("Error reading input: %s\n", error->message);
    } 

    g_io_channel_unref(io_channel);

    return line_array;
}

//=============================================================================
int
main(int argc, char** argv)
{
    GError *error = NULL;
    GOptionContext *context;

    g_type_init();

    context = g_option_context_new("infile outfile - compile LLVM modules from kernels.");

    if(!g_option_context_parse(context, &argc, &argv, &error))
    {
        g_print ("option parsing failed: %s\n", error->message);
        exit(1);
    }

    if(argc < 3) {
        g_print("Must have input and output file.\n");
        exit(2);
    }

    int fd = 0;
    if(0 != strcmp(argv[1], "-")) {
        fd = open(argv[1], O_RDONLY);
        if(fd == -1)
        {
            g_print("Error opening input: %s\n", argv[1]);
            exit(2);
        }
    }

    GPtrArray* lines = read_lines(fd);
    if(lines == NULL) 
    {
        exit(3);
    }

    FirtreeKernel* kernel = firtree_kernel_new();

    if(!firtree_kernel_compile_from_source(kernel, (gchar**)lines->pdata, lines->len, NULL))
    {
        g_log(G_LOG_DOMAIN, G_LOG_LEVEL_INFO, "Compilation failed.\n");
    }

    free_lines(lines);

    if(fd != 0) 
    {
        close(fd);
    }

    const char* const* log_lines = firtree_kernel_get_compile_log(kernel, NULL);
    while(*log_lines != NULL) {
        g_log(G_LOG_DOMAIN, G_LOG_LEVEL_INFO, "%s\n", *log_lines);
        ++log_lines;
    }

    if(firtree_kernel_get_compile_status(kernel)) {
        std::ofstream output(argv[2]);

        firtree_kernel_get_llvm_module(kernel)->print(output, NULL);

        output << "\n";

        guint n_args = 0;
        GQuark* args = firtree_kernel_list_arguments(kernel, &n_args);

        output << "; Kernel argument count: " << n_args << "\n";

        GQuark* current_arg = args;
        while(*current_arg) {
            FirtreeKernelArgumentSpec* spec =
                firtree_kernel_get_argument_spec(kernel, *current_arg);
            output << ";   - " << g_quark_to_string(*current_arg);
            output << " (is static: " << spec->is_static << ") \n";
            ++current_arg;
        }
    }

    g_object_unref(kernel);

    return 0;
}

// vim:sw=4:ts=4:cindent:et

