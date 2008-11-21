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
#include <set>

// ============================================================================
/// The Firtree namespace.
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
/// - References returned from accessor methods are assumed to be valid for
///   the lifetime of the object.
///
/// - Pointers returned from Create.*() methods are assumed to be 'owned' by
///   the caller and should have Release() called on them when finished with.
///
/// - Pointers returned from Get.*() methods are assumed to only be valid 
///   for the lifetime of the object. If you wish to keep them valid, call
///   Release()/Retain() on them.
///
/// - Pointers passed to object methods are assumed to only be valid within
///   that method. Should the object wish to retain the passed object, it
///   should either be copied or retained.
///
class ReferenceCounted {
    private:
        static size_t ObjectCount;
        static std::set<ReferenceCounted*> ActiveObjects;

    protected:
        /// @{
        /// Protected constructors/destructors. Allocation should be
        /// done via explicit static Create.*() methods which use
        /// the 'new' operator.
        ReferenceCounted();
        /// @}

    public:
        virtual ~ReferenceCounted();
        /// Increment the object's reference count.
        void Retain();

        /// Decremement the object's reference count. Should the reference
        /// count fall below 1, the object will be deleted.
        void Release();

        /// Return a global count of all reference counted objects.
        /// This should be zero immediately before your app exits
        /// to guard against memory leaks.
        static size_t GetGlobalObjectCount();

        /// Return a set containing all the active (i.e. not yet deallocated)
        /// objects.
        static const std::set<ReferenceCounted*>& GetActiveObjects();

    private:
        unsigned int m_RefCount;
};

/// Convenience macro to retain a variable if non-NULL or warn if
/// it is NULL.
#define FIRTREE_SAFE_RETAIN(a) do { \
    Firtree::ReferenceCounted* _tmp = (a); \
    if(_tmp != NULL) { _tmp->Retain(); } else { \
        FIRTREE_WARNING("Attempt to retain a NULL pointer."); } \
} while(0)

/// Convenience macro to release a variable if non-NULL and assign 
/// NULL to it. The argument has to be an lvalue.
#define FIRTREE_SAFE_RELEASE(a) do { \
    if((a) != NULL) { (a)->Release(); a = NULL; } } while(0)

// ============================================================================
// EXCEPTIONS

/// Base class for all exceptions which can be thrown by FIRTREE.
class Exception {
    protected:
        std::string       m_Message;    ///< Exception message.
        std::string       m_File;       ///< Exception file.
        int               m_Line;       ///< Exception line.
        std::string       m_Function;   ///< Exception function.
    public:
        /// Constructor for an exception.
        ///
        /// \param message The message associated with this exception.
        /// \param file The file in which this exception was thrown.
        /// \param line The line within the file in which this exception was 
        ///             thrown.
        /// \param func The name of the function in which this exception was 
        ///             thrown.
        Exception(const char* message, const char* file, int line,
                const char* func);
        ~Exception();

        /// Return the message associated with this exception.
        const std::string& GetMessage() const;

        /// Return the file in which this exception was thrown.
        const std::string& GetFile() const;

        /// Return the line within the file in which this exception was thrown.
        int GetLine() const;

        /// Return the name of the function in which this exception was thrown.
        const std::string& GetFunction() const;
};

/// Exception describing an internal FIRTREE error.
class ErrorException : public Exception {
    public:
        /// Constructor for an exception. Usually this will be called via
        /// the FIRTREE_ERROR() macro.
        /// \see Exception::Exception()
        ErrorException(const char* message, const char* file, int line, 
                const char* func);
        ~ErrorException();
};

// ============================================================================
// Error reporting

///@{
///     Report a fatal error to the user. The default implementation is to
///     throw an ErrorException.
#define FIRTREE_ERROR(...) do {Firtree::Error(__FILE__, __LINE__, __PRETTY_FUNCTION__, __VA_ARGS__);} while(0)
void    Error(const char* file, int line, const char* func, const char* format, ...);
///@}

///@{
///     Report a non-fatal warning to the user. The default implementation is
///     to print a message to stderr. It takes printf-style arguments.
#define FIRTREE_WARNING(...) do {Firtree::Warning(__FILE__, __LINE__, __PRETTY_FUNCTION__, __VA_ARGS__);} while(0)
void    Warning(const char* file, int line, const char* func, const char* format, ...);
///@}

///@{
///     Report a debug message. The default implementation is
///     to print a message to stdout. It takes printf-style arguments.
#define FIRTREE_DEBUG(...) do {Firtree::Debug(__FILE__, __LINE__, __PRETTY_FUNCTION__, __VA_ARGS__);} while(0)
void    Debug(const char* file, int line, const char* func, const char* format, ...);
///@}

///@{
///     Report a trace message. The default implementation is
///     to print a message to stdout. It takes printf-style arguments.
#define FIRTREE_TRACE(...) do {Firtree::Trace(__FILE__, __LINE__, __PRETTY_FUNCTION__, __VA_ARGS__);} while(0)
void    Trace(const char* file, int line, const char* func, const char* format, ...);
///@}
    
//=============================================================================
/// \brief The possible types in the Firtree kernel language.
///
/// Note that during code generation, the '__color' type is aliased to
/// vec4 and the sampler type is aliased to const int.
enum KernelTypeSpecifier {
    TySpecFloat,          ///< A 32-bit floating point.
    TySpecInt,            ///< A 32-bit signed integet.
    TySpecBool,           ///< A 1-bit boolean.
    TySpecVec2,           ///< A 2 component floating point vector.
    TySpecVec3,           ///< A 3 component floating point vector.
    TySpecVec4,           ///< A 4 component floating point vector.
    TySpecSampler,        ///< An image sampler.
    TySpecColor,          ///< A colour.
    TySpecVoid,           ///< A 'void' type.
    TySpecInvalid = -1,   ///< An 'invalid' type.
};

// ============================================================================
/// This class defines private copy and assignment constructors to make
/// sure that certain classes are uncopiable.
class Uncopiable {
    protected:
        Uncopiable() { }

    private:
        /// @{
        /// Intentionally unimplemented so that attempts to copy
        /// uncopiable classes cause at least linker errors.
        Uncopiable(Uncopiable&);
        const Uncopiable& operator = (Uncopiable&);
        /// @}
};

// ============================================================================
} // namespace Firtree 
// ============================================================================

// ============================================================================
#endif // FIRTREE_MAIN_H
// ============================================================================

// So that VIM does the "right thing"
// vim:cindent:sw=4:ts=4:et
