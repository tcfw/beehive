#include <kernel/skiplist.h>
#include <kernel/sync.h>
#include <kernel/mm.h>
#include <kernel/rand.h>
#include <kernel/strings.h>

static int rand_level(int max)
{
	int level = 1;

	while (rand() < RAND_MAX / 2 && level < max)
		level++;

	return level;
}

void skl_init(skiplist_t *skl, int max_levels, skiplist_compare insert_comparator, int flags)
{
	skl->head.forward = (skl_node_t **)kmalloc(sizeof(skl_node_t *) * max_levels);

	skl->size = 0;
	skl->levels = max_levels;
	skl->compare = insert_comparator;
	skl->flags = flags;
}

int skl_insert(skiplist_t *skl, void *rnode)
{
	skl_node_t *update[skl->levels];
	skl_node_t *x = &skl->head;

	int i, level;

	for (i = skl->levels; i >= 0; i--)
	{
		skl_node_t *fnode = x->forward[i];
		if (fnode)
			while (skl->compare(rnode, fnode->rnode) == 1)
			{
				x = fnode;
				fnode = x->forward[i];
				if (!fnode)
					break;
			}
		update[i] = x;
	}

	x = x->forward[0];

	if (x != 0 && x->rnode == rnode)
		return -1;

	level = rand_level(skl->levels);

	x = (skl_node_t *)kmalloc(sizeof(skl_node_t));
	x->rnode = rnode;
	x->forward = (skl_node_t **)kmalloc(sizeof(skl_node_t *) * (skl->levels));
	memset(x->forward, sizeof(skl_node_t *) * (skl->levels), 0);
	for (i = 0; i < level; i++)
	{
		x->forward[i] = update[i]->forward[i];
		update[i]->forward[i] = x;
	}

	skl->size++;

	return 0;
}

int skl_delete(skiplist_t *skl, void *rnode)
{
	skl_node_t *update[skl->levels];
	skl_node_t *x = &skl->head;

	int i;
	for (i = skl->levels; i >= 0; i--)
	{
		skl_node_t *fnode = x->forward[i];
		if (fnode)
			while (skl->compare(rnode, fnode->rnode) == 1)
			{
				x = fnode;
				fnode = x->forward[i];
				if (!fnode)
					break;
			}

		update[i] = x;
	}

	x = x->forward[0];
	if (rnode == x->rnode)
	{
		for (i = 0; i <= skl->levels; i++)
		{
			if (update[i]->forward[i] != x)
				break;
			update[i]->forward[i] = x->forward[i];
		}

		kfree(x);
		skl->size--;
		return 0;
	}

	return -1;
}

void *skl_search(skiplist_t *skl, void *rnode, skiplist_compare search_comparator)
{
	skl_node_t *x = &skl->head;

	for (int i = skl->levels; i >= 0; i--)
	{
		if (x->forward[i] == 0)
			continue;

		while (search_comparator(rnode, x->forward[i]->rnode) == 1)
		{
			x = x->forward[i];
			if (!x)
				break;
		}
	}

	if (x != NULL)
		return x->forward[0];

	return x;
}

void *skl_first(skiplist_t *skl)
{
	skl_node_t *x = skl->head.forward[0];
	if (x)
		return x->rnode;

	return 0;
}

void *skl_pull_first(skiplist_t *skl)
{
	skl_node_t *x = skl->head.forward[0];
	if (x)
	{
		void *v = x->rnode;
		skl_node_t *next = x->forward[0];

		for (int i = 0; i < skl->levels; i++)
		{
			if (skl->head.forward[i] == x)
				skl->head.forward[i] = next;
		}

		kfree(x);
		skl->size--;
		return v;
	}

	return 0;
}