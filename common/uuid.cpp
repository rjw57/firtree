// FIRTREE - A generic image processing library
// Copyright (C) 2007, 2008 Rich Wareham <richwareham@gmail.com>
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as published
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
// This file implements the UUID gneration function
//=============================================================================

#include "uuid.h"

#include <stdio.h>

void
generate_random_uuid(gchar* buffer, gchar separator)
{
    // The field names refer to RFC 4122 section 4.1.2
    snprintf(buffer, 37, "%04x%04x%c%04x%c%03x4%c%04x%c%04x%04x%04x",
            g_random_int_range(0, 65535), // -._
            g_random_int_range(0, 65535), // -' '- 32 bits - time_low
            separator,
            g_random_int_range(0, 65535), // ---- 16 bits - time_mid
            separator,
            g_random_int_range(0, 4095),  // ---- leading 12 bits of time_hi_and_version
            separator,
            ((g_random_int_range(0, 65535) & ~0x0300) | 0x0100), // clk_seq_low
            separator,
            g_random_int_range(0, 65535),  // -. 
            g_random_int_range(0, 65535),  //  |- 48 bits - node
            g_random_int_range(0, 65535)); // -'
}

// So that VIM does the "right thing"
// vim:cindent:sw=4:ts=4:et
