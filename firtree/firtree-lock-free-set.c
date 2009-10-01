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

#include <string.h>

#include "firtree-lock-free-set.h"

/**
 * SECTION:firtree-lock-free-set
 * @short_description: Lock-free data structure for storing a equal sized
 * elements.
 * @include: firtree/firtree-lock-free-set.h
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

/**
 * FirtreeLockFreeSet:
 *
 * An opaque structure which represents a lock-free set.
 */

struct _FirtreeLockFreeSetNode {
	struct _FirtreeLockFreeSetNode *next;
	/* Data is stored in memory after the header. */
};
typedef struct _FirtreeLockFreeSetNode FirtreeLockFreeSetNode;

struct _FirtreeLockFreeSet {
	FirtreeLockFreeSetNode *head;
	gpointer tail;
	gsize element_size;
	gint element_count;
};

#define LF_NODE_TO_DATA(node) ((gpointer)((guint8*)(node)+sizeof(FirtreeLockFreeSetNode)))
#define LF_DATA_TO_NODE(data) ((FirtreeLockFreeSetNode*)((guint8*)(data)-sizeof(FirtreeLockFreeSetNode)))

static inline
    FirtreeLockFreeSetNode *
_firtree_lock_free_set_new_node(FirtreeLockFreeSet * set)
{
	FirtreeLockFreeSetNode *node = (FirtreeLockFreeSetNode *)
	    g_slice_alloc(set->element_size + sizeof(FirtreeLockFreeSetNode));
	node->next = NULL;
	return node;
}

/**
 * firtree_lock_free_set_new:
 * @element_size: The size, in bytes, of a set element.
 *
 * Allocate a new FirtreeLockFreeSet structure which will hold elements of @element_size.
 *
 * Returns: A newly allocated FirtreeLockFreeSet structure.
 */
FirtreeLockFreeSet *firtree_lock_free_set_new(gsize element_size)
{
	FirtreeLockFreeSet *set = g_new(FirtreeLockFreeSet, 1);

	set->element_size = element_size;
	g_atomic_int_set(&(set->element_count), 0);

	/* We always keep an element 'in hand' to copy to. This makes the
	 * CAS locking simpler. */
	FirtreeLockFreeSetNode *node = _firtree_lock_free_set_new_node(set);
	g_atomic_pointer_set(&(set->head), node);
	g_atomic_pointer_set(&(set->tail), node);

	return set;
}

/**
 * firtree_lock_free_set_free:
 * @set: A FirtreeLockFreeSet structure.
 *
 * Frees the set @set releasing any memory resources associated with it.
 */
void firtree_lock_free_set_free(FirtreeLockFreeSet * set)
{
	g_assert(set != NULL);

	/* Walk the set, freeing each node. */
	if (set->head != NULL) {
		g_slice_free_chain_with_offset(set->element_size +
					       sizeof(FirtreeLockFreeSetNode),
					       set->head, 0);
	}

	set->head = set->tail = NULL;
	g_free(set);
}

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
firtree_lock_free_set_add_element(FirtreeLockFreeSet * set, gpointer element)
{
	/* Create a new node 'in hand' which will be used in the next call. */
	FirtreeLockFreeSetNode *new_node = _firtree_lock_free_set_new_node(set);

	/* Atomically swap this for the tail. */
	FirtreeLockFreeSetNode *old_tail = g_atomic_pointer_get(&(set->tail));
	while (!g_atomic_pointer_compare_and_exchange
	       (&(set->tail), old_tail, new_node)) {
		old_tail = g_atomic_pointer_get(&(set->tail));
	}

	/* The old tail is where we write our data */
	old_tail->next = new_node;

	/* Copy the element data. */
	memcpy(LF_NODE_TO_DATA(old_tail), element, set->element_size);

	/* Increment the element count. */
	g_atomic_int_inc(&(set->element_count));
}

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
gpointer firtree_lock_free_set_get_first_element(FirtreeLockFreeSet * set)
{
	if (firtree_lock_free_set_get_element_count(set) == 0) {
		return NULL;
	}

	return LF_NODE_TO_DATA(g_atomic_pointer_get(&(set->head)));
}

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
firtree_lock_free_set_get_next_element(FirtreeLockFreeSet * set,
				       gpointer element)
{
	FirtreeLockFreeSetNode *node = LF_DATA_TO_NODE(element)->next;

	if (node && node->next) {
		/* If this node is not the tail 'in hand' */
		return LF_NODE_TO_DATA(node);
	}

	return NULL;
}

/**
 * firtree_lock_free_set_get_element_count:
 * @set: A FirtreeLockFreeSet structure.
 *
 * Retrieve the number of elements currently in the set.
 * 
 * Returns: A gint giving the number of elements in the set.
 */
gint firtree_lock_free_set_get_element_count(FirtreeLockFreeSet * set)
{
	return g_atomic_int_get(&(set->element_count));
}

/* vim:sw=8:ts=8:tw=78:noet:cindent
 */
