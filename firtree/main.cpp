// FIRTREE - A generic image processing library
// Copyright (C) 2007, 2008 Rich Wareham <richwareham@gmail.com>
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License verstion as published
// by the Free Software Foundation.
//
// This program is distributed in the hope that it will be useful, but WITHOUT
// ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
// FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
// more details.
//
// You should have received a copy of the GNU General Public License along with
// this program; if not, write to the Free Software Foundation, Inc., 51
// Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA

//=============================================================================
// This file implements the FIRTREE ancillary functions.
//=============================================================================

#include <firtree/main.h>

#include <string.h>
#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

// ===============================================================================
namespace Firtree {
// ===============================================================================

// ===============================================================================
// The object monitor is a long-lived singleton whose job is to monitor the
// allocated object count at exit and moan if it is non-zero.
class ObjectMonitor {
    public:
        ObjectMonitor() 
        {
            // FIRTREE_DEBUG("Object monitor started.");
        }

        ~ObjectMonitor()
        {
            // FIRTREE_DEBUG("Object monitor shutting down...");
            if(ReferenceCounted::GetGlobalObjectCount() > 0)
            {
                FIRTREE_WARNING("MEMORY LEAK! Allocated object count at exit "
                        "is non-zero: %i.",
                        ReferenceCounted::GetGlobalObjectCount());
            }
        }
};

static ObjectMonitor g_ObjectMonitorSingleton;

// ===============================================================================
size_t ReferenceCounted::ObjectCount(0);

// ===============================================================================
size_t ReferenceCounted::GetGlobalObjectCount()
{ 
    return ReferenceCounted::ObjectCount; 
}

// ===============================================================================
ReferenceCounted::ReferenceCounted()
    :   m_RefCount(1)
{
    ObjectCount++; 
#ifdef FIRTREE_DEBUG_MEM
    printf("Object: %p %i objects created.\n", this, ObjectCount);
#endif
}

// ===============================================================================
ReferenceCounted::~ReferenceCounted()
{ 
    ObjectCount--; 
#ifdef FIRTREE_DEBUG_MEM
    printf("Destructor of %p, %i left.\n", this, ObjectCount);
#endif
}

// ===============================================================================
void ReferenceCounted::Retain()
{ 
    m_RefCount++;
#ifdef FIRTREE_DEBUG_MEM
    printf("Object %p retained, refcount now: %i\n", this, m_RefCount);
#endif
}

// ===============================================================================
void ReferenceCounted::Release()
{ 
    assert(m_RefCount > 0); 
    if(m_RefCount == 1) { 
#ifdef FIRTREE_DEBUG_MEM
        printf("Deleting: %p\n", this);
#endif
        delete this; 
    } else { 
        m_RefCount--;
#ifdef FIRTREE_DEBUG_MEM
        printf("Object %p released, refcount now: %i\n", this, m_RefCount);
#endif
    }
}

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
