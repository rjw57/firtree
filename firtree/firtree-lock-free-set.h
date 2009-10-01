/* firtree-lock-free-set.h Lock-free data structures. */

/* FIRTREE - A generic image processing library
 * Copyright (C) 2009 Rich Wareham <richwareham@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as published
 * by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program; if not, write to the Free Software Foundation, Inc., 51
 * Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#ifndef __FIRTREE_LOCK_FREE_H__
#define __FIRTREE_LOCK_FREE_H__

#include <glib.h>

G_BEGIN_DECLS

typedef struct 		 _FirtreeLockFreeSet 		FirtreeLockFreeSet;

FirtreeLockFreeSet 	*firtree_lock_free_set_new	(gsize element_size);

void			 firtree_lock_free_set_free	(FirtreeLockFreeSet 	*set);

void			 firtree_lock_free_set_add_element
							(FirtreeLockFreeSet 	*set,
							 gpointer		 element);

gpointer		 firtree_lock_free_set_get_first_element
							(FirtreeLockFreeSet 	*set);

gpointer		 firtree_lock_free_set_get_next_element
							(FirtreeLockFreeSet 	*set,
							 gpointer		 element);

gint			 firtree_lock_free_set_get_element_count
							(FirtreeLockFreeSet 	*set);

G_END_DECLS

#endif				/* __FIRTREE_LOCK_FREE_H__ */

/* vim:sw=8:ts=8:noet:cindent
 */
