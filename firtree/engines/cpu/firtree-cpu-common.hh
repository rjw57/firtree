/* firtree-cpu-common.hh */

/* Firtree - A generic image processing library
 * Copyright (C) 2009 Rich Wareham <richwareham@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as published
 * by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.    See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program; if not, write to the Free Software Foundation, Inc., 51
 * Franklin Street, Fifth Floor, Boston, MA    02110-1301, USA
 */

#ifndef _FIRTREE_CPU_COMMON
#define _FIRTREE_CPU_COMMON

#include <glib-object.h>

#include <firtree/firtree.h>
#include <string>

G_BEGIN_DECLS

void*
firtree_cpu_common_lazy_function_creator(const std::string& name);

G_END_DECLS

#endif /* _FIRTREE_CPU_COMMON */

/* vim:sw=4:ts=4:et:cindent
 */
