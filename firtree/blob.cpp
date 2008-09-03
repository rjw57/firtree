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
// This file implements the FIRTREE blob encapsulation.
//=============================================================================

#include <firtree/blob.h>

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

// ===============================================================================
namespace Firtree {
// ===============================================================================

// ===============================================================================
// PROTECTED CONSTRUCTORS
// ===============================================================================

// ===============================================================================
Blob::Blob()
    :   ReferenceCounted()
    ,   m_Length(0)
    ,   m_Buffer(NULL)
{
}

// ===============================================================================
Blob::Blob(size_t length)
    :   ReferenceCounted()
    ,   m_Length(0)
    ,   m_Buffer(NULL)
{
    m_Buffer = (uint8_t*)(malloc(length));
    if(m_Buffer != NULL)
    {
        m_Length = length;
    }
}

// ===============================================================================
Blob::Blob(const void* buffer, size_t length)
    :   ReferenceCounted()
    ,   m_Length(0)
    ,   m_Buffer(NULL)
{
    AssignFromBuffer(buffer, length);
}

// ===============================================================================
Blob::Blob(const Blob& blob)
    :   ReferenceCounted()
    ,   m_Length(0)
    ,   m_Buffer(NULL)
{
    AssignFromBlob(blob);
}

// ===============================================================================
Blob::~Blob()
{
    if(m_Buffer != NULL)
    {
        free(m_Buffer);
    }
}

// ===============================================================================
// STATIC CONSTRUCTION METHODS
// ===============================================================================

// ===============================================================================
Blob* Blob::Create() 
{
    return new Blob(); 
}

// ===============================================================================
Blob* Blob::CreateWithLength(size_t length) 
{
    return new Blob(length); 
}

// ===============================================================================
Blob* Blob::CreateFromBuffer(const void* buffer, size_t length) 
{
    return new Blob(buffer, length); 
}

// ===============================================================================
Blob* Blob::CreateFromBlob(const Blob& blob)
{
    return new Blob(blob);
}

// ===============================================================================
// CONST METHODS
// ===============================================================================

// ===============================================================================
const uint8_t* Blob::GetBytes() const
{
    return m_Buffer;
}

// ===============================================================================
size_t Blob::GetLength() const
{
    return m_Length;
}

// ===============================================================================
Blob* Blob::Copy() const
{
    return Blob::CreateFromBlob(*this);
}

// ===============================================================================
Blob* Blob::CopySubRange(size_t offset, size_t length) const
{
    // If a zero-length buffer was requested, create one explicitly.
    if(length == 0) { return Blob::Create(); }

    // Truncate the requested length if it goes off the end of the buffer.
    if(offset + length > GetLength()) { length = GetLength() - offset; }

    return Blob::CreateFromBuffer(m_Buffer + offset, length);
}

// ===============================================================================
// MUTATING METHODS
// ===============================================================================

// ===============================================================================
void Blob::Clear()
{
    // Set length to zero
    m_Length = 0;

    // Free any memory assoicated with our buffer.
    if(m_Buffer != NULL)
    {
        free(m_Buffer);
        m_Buffer = NULL;
    }
}

// ===============================================================================
void Blob::AssignFromBuffer(const void* buffer, size_t length)
{
    // Firstly, clear any existing data
    Clear();

    // If a zero-length buffer was required, we have no more work to do.
    if((length == 0) || (buffer == NULL))
        return;

    // Allocate room to copy data into.
    m_Buffer = reinterpret_cast<uint8_t*>(malloc(length));

    // If allocation failed, panic.
    if(m_Buffer == NULL)
    {
        FIRTREE_ERROR("Failed to allocate buffer memory.");
        return;
    }

    // If succeeded, copy.
    memcpy(m_Buffer, buffer, length);
}

// ===============================================================================
void Blob::AssignFromBlob(const Blob& blob)
{
    // Use the accessor methods on the blob to assign from the underlying
    // buffer.
    AssignFromBuffer(blob.GetBytes(), blob.GetLength());
}

// ===============================================================================
} // namespace Firtree 
// ===============================================================================

// So that VIM does the "right thing"
// vim:cindent:sw=4:ts=4:et
