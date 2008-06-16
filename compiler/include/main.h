/* 
 * FIRTREE - A generic image processing system.
 * Copyright (C) 2007. 2008 Rich Wareham <srichwareham@gmail.com>
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
// This file defines the FIRTREE compiler's ancillary infrastructure.
//=============================================================================

// ============================================================================
#ifndef FIRTREE_MAIN_H
#define FIRTREE_MAIN_H
// ============================================================================

#include <string>

// ============================================================================
namespace Firtree {
// ============================================================================

//=============================================================================
// THIS IS NOT THREAD SAFE!
class ReferenceCounted {
    public:
        ReferenceCounted() : m_RefCount(1) { }
        virtual ~ReferenceCounted() { }

        void Retain() { m_RefCount++; }
        void Release() { if(m_RefCount <= 1) { delete this; } }

    private:
        unsigned int m_RefCount;
};

// ============================================================================
// Exceptions

/// Base class for all exceptions.
class Exception {
    protected:
        std::string       m_Message;
        std::string       m_File;
        int               m_Line;
        std::string       m_Function;
    public:
        Exception(const char* message, const char* file, int line, const char* func);
        ~Exception();

        const std::string& GetMessage() const;
        const std::string& GetFile() const;
        int GetLine() const;
        const std::string& GetFunction() const;
};

/// Exception describing an internal error
class ErrorException : public Exception {
    public:
        ErrorException(const char* message, const char* file, int line, 
                const char* func);
        ~ErrorException();
};

// ============================================================================
// Error reporting

///     Report a fatal error to the user. The default implementation is to
///     throw an ErrorException.
#define FIRTREE_ERROR(...) do {Firtree::Error(__FILE__, __LINE__, __PRETTY_FUNCTION__, __VA_ARGS__);} while(0)
void    Error(const char* file, int line, const char* func, const char* format, ...);

///     Report a non-fatal warning to the user. The default implementation is
///     to print a message to stderr. It takes printf-style arguments.
#define FIRTREE_WARNING(...) do {Firtree::Warning(__FILE__, __LINE__, __PRETTY_FUNCTION__, __VA_ARGS__);} while(0)
void    Warning(const char* file, int line, const char* func, const char* format, ...);

///     Report a debug message. The default implementation is
///     to print a message to stdout. It takes printf-style arguments.
#define FIRTREE_DEBUG(...) do {Firtree::Debug(__FILE__, __LINE__, __PRETTY_FUNCTION__, __VA_ARGS__);} while(0)
void    Debug(const char* file, int line, const char* func, const char* format, ...);

// ============================================================================
} // namespace Firtree 
// ============================================================================

// ============================================================================
#endif // FIRTREE_MAIN_H
// ============================================================================

// So that VIM does the "right thing"
// vim:cindent:sw=4:ts=4:et
