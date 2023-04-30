#include <kernel/buddy.h>
#include "stdint.h"
#include "stdbool.h"

bool buddy_is_full(struct buddy_t *buddy)
{
	for (
		uint8_t i = 0; i < BUDDY_MAX_ORDER; i++)
	{
		if (buddy->freelist[i] != 0)
		{
			return false;
		}
	}

	return true;
}

bool buddy_is_parent_split(struct buddy_t *buddy, unsigned int p)
{
	if (p < 1)
		return true;

	if (BUDDY_GET_POSITION(buddy->buddy_tree, BUDDY_PARENT_POSITION(p)) == 1)
		if (BUDDY_GET_POSITION(buddy->buddy_tree, p) == 1 || BUDDY_GET_POSITION(buddy->buddy_tree, BUDDY_COMPANION_POSITION(p)) == 1)
			return true;

	return false;
}

void buddy_init(struct buddy_t *buddy)
{
	for (uint8_t i = 0; i < BUDDY_MAX_ORDER; i++)
	{
		buddy->freelist[i] = 1;
	}
}