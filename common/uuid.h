// FIRTREE - A generic image processing library
// Copyright (C) 2009 Rich Wareham <richwareham@gmail.com>
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
/// \file uuid.h Functions to generate uuids.
//=============================================================================

// ============================================================================
#ifndef FIRTREE_UUID_H
#define FIRTREE_UUID_H
// ============================================================================

#include <glib.h>

G_BEGIN_DECLS

// ============================================================================
/// Write a textual representation of a random UUID to the buffer passed. The
/// buffer must be at least 37 bytes long to hold the text + terminating 
/// NULL. The field separator character is also specified. To comply with
/// RFC 4122, this separator should be '-'.
void
generate_random_uuid(gchar* buffer, gchar separator);

G_END_DECLS

// ============================================================================
#endif // FIRTREE_UUID_H
// ============================================================================

// So that VIM does the "right thing"
// vim:cindent:sw=4:ts=4:et
