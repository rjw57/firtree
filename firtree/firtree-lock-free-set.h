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

#ifndef FIRTREE_LOCK_FREE_H
#define FIRTREE_LOCK_FREE_H

#include <glib.h>

G_BEGIN_DECLS

/**
 * FirtreeLockFreeSet:
 *
 * A lock-free set is one which is safe for multiple threads to add elements to.
 * The ordering of elements is not guranteed. Adding an element is lock-free but
 * assumes that the contents of memory the element value is copied from does
 * not change over the lifetime of the firtree_lock_free_set_add_element() call.
 *
 * Once added, an element cannot be removed. The FirtreeLockFreeSet structure is designed
 * to only ever be added to.
 *
 * Concurrent access is only supported by the firtree_lock_free_set_add_element() call.
 */
typedef struct _FirtreeLockFreeSet FirtreeLockFreeSet;

/**
 * firtree_lock_free_set_new:
 * @element_size: The size, in bytes, of a set element.
 *
 * Allocate a new FirtreeLockFreeSet structure which will hold elements of @element_size.
 *
 * Returns: A newly allocated FirtreeLockFreeSet structure.
 */
FirtreeLockFreeSet*
firtree_lock_free_set_new(gsize element_size);

/**
 * firtree_lock_free_set_free:
 * @set: A FirtreeLockFreeSet structure.
 *
 * Frees the set @set releasing any memory resources associated with it.
 */
void
firtree_lock_free_set_free(FirtreeLockFreeSet* set);

/**
 * firtree_lock_free_set_add_element:
 * @set: A FirtreeLockFreeSet structure.
 * @element: A pointer to the element to add.
 *
 * A new element is added to the set and it's data is copied from the @element
 * pointer. The number of bytes copied is specified in the firtree_lock_free_set_new() call.
 *
 * Note: This is the only method on FirtreeLockFreeSet which is thread-safe. Other calls must
 * be surrounded by appropriate locking if concurrent access is expected.
 */
void
firtree_lock_free_set_add_element(FirtreeLockFreeSet* set, gpointer element);

/**
 * firtree_lock_free_set_get_first_element:
 * @set: A FirtreeLockFreeSet structure.
 *
 * Retrieve the first element in the set or NULL if the set is empty.
 *
 * Together with firtree_lock_free_set_get_next_element(), this call may be used to iterate
 * over all items in the set.
 *
 * Returns: A pointer to a set element or NULL.
 */
gpointer
firtree_lock_free_set_get_first_element(FirtreeLockFreeSet* set);

/**
 * firtree_lock_free_set_get_next_element:
 * @set: A FirtreeLockFreeSet structure.
 * @element: A set element.
 *
 * Retrieve the next element based upon @element which should have been returned from
 * firtree_lock_free_set_get_first_element() or
 * firtree_lock_free_set_get_next_element(). Returns NULL if there are no more
 * elements.
 *
 * Together with firtree_lock_free_set_get_first_element(), this call may be used to iterate
 * over all items in the set.
 *
 * Returns: A pointer to a set element or NULL.
 */
gpointer
firtree_lock_free_set_get_next_element(FirtreeLockFreeSet* set, gpointer element);

/**
 * firtree_lock_free_set_get_element_count:
 * @set: A FirtreeLockFreeSet structure.
 *
 * Retrieve the number of elements currently in the set.
 * 
 * Returns: A gint giving the number of elements in the set.
 */
gint
firtree_lock_free_set_get_element_count(FirtreeLockFreeSet* set);

G_END_DECLS

#endif /* FIRTREE_LOCK_FREE_H */

/* vim:cindent:sw=4:ts=4:et 
 */
