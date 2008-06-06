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

//=============================================================================
// This file implements the FIRTREE ancillary functions.
//=============================================================================

#include "include/main.h"

#include <string.h>
#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

// ===============================================================================
namespace Firtree {
// ===============================================================================

// ===============================================================================
Exception::Exception(const char* message, const char* file, int line,
        const char* func)
:   m_Message(message)
,   m_File(file)
,   m_Line(line)
,   m_Function(func)
{
}

// ===============================================================================
Exception::~Exception() { }

// ===============================================================================
const std::string& Exception::GetMessage() const
{
    return m_Message;
}

// ===============================================================================
const std::string& Exception::GetFile() const
{
    return m_File;
}

// ===============================================================================
int Exception::GetLine() const
{
    return m_Line;
}

// ===============================================================================
const std::string& Exception::GetFunction() const
{
    return m_Function;
}

// ===============================================================================
ErrorException::ErrorException(const char* message, const char* file,
        int line, const char* func)
:   Exception(message, file, line, func)
{
}

// ===============================================================================
ErrorException::~ErrorException() { }

// ============================================================================
void Error(const char* file, int line, const char* func, const char* format, ...)
{
    va_list args;
   
    // Allocate space for the message
    va_start(args, format);
    int size = vsnprintf(NULL, 0, format, args) + 1;
    va_end(args);

    char* message = (char*)(malloc(size));
    if(message == NULL)
        return;

    va_start(args, format);
    vsnprintf(message, size, format, args);
    va_end(args);

    throw ErrorException(message, file, line, func);
}

// ============================================================================
void Warning(const char* file, int line, const char* func, const char* format, ...)
{
    va_list args;
    fprintf(stderr, "%s:%i: [W] ",file,line);
    va_start(args, format);
    vfprintf(stderr, format, args);
    va_end(args);
    fprintf(stderr, "\n");
}

// ============================================================================
void Debug(const char* file, int line, const char* func, const char* format, ...)
{
    va_list args;
    fprintf(stderr, "%s:%i: [I] ",file,line);
    va_start(args, format);
    vfprintf(stdout, format, args);
    va_end(args);
    fprintf(stdout, "\n");
}

// ===============================================================================
} // namespace Firtree 
// ===============================================================================

// So that VIM does the "right thing"
// vim:cindent:sw=4:ts=4:et
