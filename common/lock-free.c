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

#include "lock-free.h"

struct _LockFreeSetNode {
    struct _LockFreeSetNode*    next;
    /* Data is stored in memory after the header. */
};
typedef struct _LockFreeSetNode LockFreeSetNode;

struct _LockFreeSet {
    LockFreeSetNode*    head;
    gpointer            tail;
    gsize               element_size;
    gint                element_count;
};

#define LF_NODE_TO_DATA(node) ((gpointer)((guint8*)(node)+sizeof(LockFreeSetNode)))
#define LF_DATA_TO_NODE(data) ((LockFreeSetNode*)((guint8*)(data)-sizeof(LockFreeSetNode)))

static inline
LockFreeSetNode*
_lock_free_set_new_node(LockFreeSet* set)
{
    LockFreeSetNode* node = (LockFreeSetNode*)
        g_slice_alloc(set->element_size + sizeof(LockFreeSetNode));
    node->next = NULL;
    return node;
}

LockFreeSet*
lock_free_set_new(gsize element_size)
{
    LockFreeSet* set = g_new(LockFreeSet, 1);
    
    set->element_size = element_size;
    g_atomic_int_set(&(set->element_count), 0);

    /* We always keep an element 'in hand' to copy to. This makes the
     * CAS locking simpler. */
    LockFreeSetNode* node = _lock_free_set_new_node(set);
    g_atomic_pointer_set(&(set->head), node);
    g_atomic_pointer_set(&(set->tail), node);

    return set;
}

void
lock_free_set_free(LockFreeSet* set)
{
    g_assert(set != NULL);

    /* Walk the set, freeing each node. */
    if(set->head != NULL) {
        g_slice_free_chain_with_offset(
                set->element_size + sizeof(LockFreeSetNode),
                set->head,
                0);
    }

    set->head = set->tail = NULL;
    g_free(set);
}

void
lock_free_set_add_element(LockFreeSet* set, gpointer element)
{
    /* Create a new node 'in hand' which will be used in the next call. */
    LockFreeSetNode* new_node = _lock_free_set_new_node(set);

    /* Atomically swap this for the tail. */
    LockFreeSetNode* old_tail = g_atomic_pointer_get(&(set->tail));
    while(!g_atomic_pointer_compare_and_exchange(&(set->tail), old_tail, new_node)) {
        old_tail = g_atomic_pointer_get(&(set->tail));
    }

    /* The old tail is where we write our data */
    old_tail->next = new_node;

    /* Copy the element data. */
    memcpy(LF_NODE_TO_DATA(old_tail), element, set->element_size);

    /* Increment the element count. */
    g_atomic_int_inc(&(set->element_count));
}

gpointer
lock_free_set_get_first_element(LockFreeSet* set)
{
    return LF_NODE_TO_DATA(g_atomic_pointer_get(&(set->head)));
}

gpointer
lock_free_set_get_next_element(LockFreeSet* set, gpointer element)
{
    LockFreeSetNode* node = LF_DATA_TO_NODE(element)->next;
    
    if(node && node->next) {
        /* If this node is not the tail 'in hand' */
        return LF_NODE_TO_DATA(node);
    }

    return NULL;
}

/* vim:cindent:sw=4:ts=4:et 
 */
