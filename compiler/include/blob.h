/* 
 * FIRTREE - A generic image processing system.
 * Copyright (C) 2007 Rich Wareham <richwareham@gmail.com>
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
// This file defines the interface for accessing blobs of data.
//=============================================================================

// ============================================================================
#ifndef FIRTREE_BLOB_H
#define FIRTREE_BLOB_H
// ============================================================================

#include <compiler/include/main.h>

// ============================================================================
namespace Firtree {
// ============================================================================

// ============================================================================
/// An encapsulation of a binary blob of data.
class Blob : public ReferenceCounted {
    protected:
        Blob(); 
        Blob(const void* buffer, size_t length);
        Blob(const Blob& blob);
        virtual ~Blob();

    public:
        // ====================================================================
        // CONSTRUCTION METHODS

        /// Create an empty blob.
        static Blob*    Create();

        /// Create a blob by copying the contents of a buffer.
        static Blob*    CreateFromBuffer(const void* buffer, size_t length);

        /// Create a blob by deep-copying an existing blob.
        static Blob*    CreateFromBlob(const Blob& blob);

        // ====================================================================
        // CONST METHODS

        /// Return a pointer to the blob's internal buffer. Use this method
        /// with care.
        const uint8_t*  GetBytes() const;

        /// Return the length of the blob in bytes
        size_t          GetLength() const;

        /// Return a new blob initialised with a copy of this blob's data.
        Blob*           Copy() const;

        /// Copy a subset of the data within this blob into a new blob and
        /// return it. Should the subset requested extend off the end of
        /// this blob, it is truncated.
        Blob*           CopySubRange(size_t offset, size_t length) const;

        // ====================================================================
        // MUTATING METHODS
        
        /// Truncates any existing data in the buffer to zero length
        /// and frees any associated resources.
        void            Clear();

        /// Copy data from a buffer into this blob.
        void            AssignFromBuffer(const void* buffer, size_t length);

        /// Copy the data from an existing blob into this blob.
        void            AssignFromBlob(const Blob& blob);

    private:
        size_t          m_Length;
        uint8_t*        m_Buffer;
};

// ============================================================================
} // namespace Firtree 
// ============================================================================

// ============================================================================
#endif // FIRTREE_BLOB_H
// ============================================================================

// So that VIM does the "right thing"
// vim:cindent:sw=4:ts=4:et
