#ifndef _KERNEL_SKIPLIST_H
#define _KERNEL_SKIPLIST_H

#include <kernel/sync.h>

#define SKIPLIST_DEFAULT_LEVELS (7)

typedef struct skl_node_t
{
	// real node
	void *rnode;

	struct skl_node_t **forward;
} skl_node_t;

// 1 for rnode is greater than list_rnode
// 0 for rnode is equal to list_rnode
// -1 for rnode is less than list_rnode
//
// IMPORTANT: there's a very good chance that list_rnode
// will be null at some point - the comparator must handle
// this accordingly
typedef int (*skiplist_compare)(void *rnode, void *list_rnode);

// Probabilistic skip list header
typedef struct skiplist_t
{
	// max levels for skiplist
	int levels;

	// number of nodes in the skiplist
	int size;

	// flags
	int flags;

	// node value comparator
	skiplist_compare compare;

	// level heads
	skl_node_t head;
} skiplist_t;

// Init a new skip list
void skl_init(skiplist_t *skl, int max_levels, skiplist_compare insert_comparator, int flags);

// Insert real node pointer into the skip list
//-1 will be returned if the pointer is already in the
// skip list, otherwise 0 is returned
int skl_insert(skiplist_t *skl, void *rnode);

// Delete the node pointer from the skip list
//-1 will be returned if the pointer not in the
// skip list, otherwise 0 is returned
int skl_delete(skiplist_t *skl, void *rnode);

// Search for the real node value in the skip list
// using the search comparator
void *skl_search(skiplist_t *skl, void *rnode, skiplist_compare search_comparator);

// Get first node in the skip list
void *skl_first(skiplist_t *skl);

// Get first node in the skip list and remove it
void *skl_pull_first(skiplist_t *skl);

#endif