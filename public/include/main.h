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
/// \file main.h The main FIRTREE interfaces.
//=============================================================================

// ============================================================================
#ifndef FIRTREE_MAIN_H
#define FIRTREE_MAIN_H
// ============================================================================

#include <string>

// ============================================================================
namespace Firtree {
// ============================================================================

// Uncomment if you want detailed statistics of object allocation sent
// to stdout.
//
// #define FIRTREE_DEBUG_MEM

//=============================================================================
/// The base-class for a class capable of maintaining a reference count and
/// auto-deleting. THIS CLASS IS NOT THREAD SAFE.
///
/// FIRTREE uses reference counting to allow multiple classes to 'claim 
/// interest' in a particular object. The convention used in FIRTREE is 
/// the following (any deviation has either a very good cause or is a bug).
///
/// * References returned from accessor methods are assumed to be valid for
///   the lifetime of the object.
///
/// * Pointers returned from Create.*() methods are assumed to be 'owned' by
///   the caller and should have Release() called on them when finished with.
///
/// * Pointers returned from Get.*() methods are assumed to only be valid 
///   for the lifetime of the object. If you wish to keep them valid, call
///   Release()/Retain() on them.
///
/// * Pointers passed to object methods are assumed to only be valid within
///   that mathod. Should the object wish to retain the passed object, it
///   should either be copied or retained.
///
class ReferenceCounted {
    private:
        static size_t ObjectCount;

    public:
        ReferenceCounted();
        virtual ~ReferenceCounted();

        void Retain();
        void Release();

        static size_t GetGlobalObjectCount();
    private:
        unsigned int m_RefCount;
};

#define FIRTREE_SAFE_RELEASE(a) do { \
    if((a) != NULL) { (a)->Release(); a = NULL; } } while(0)

// ============================================================================
// EXCEPTIONS

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
