#include <kernel/stdbool.h>
#include <kernel/buddy.h>
#include <kernel/tty.h>

// buddy_is_full checks if all orders have been allocated or split via the freelist
static bool buddy_is_full(struct buddy_t *buddy, uint8_t order);

// buddy_is_parent_split checks if the parent is split (parent pos = 1 and either children is 1)
static bool buddy_is_parent_split(struct buddy_t *buddy, uint16_t p);

// buddy_is_parent_allocd checks if a parent is allocated (parent pos = 1 and all chicdren are 0)
static bool buddy_is_parent_allocd(struct buddy_t *buddy, uint16_t p);

// buddy_order_next_free_pos finds the next best free position for a given order
static uint16_t buddy_order_next_free_pos(struct buddy_t *buddy, uint8_t order);

// buddy_pos_to_addr converts a position to an address, excluding the office of the arena
static uintptr_t buddy_pos_to_addr(int pos);

// buddy_set_position sets an individual bit at position p in the buddy tree
static void buddy_set_position(struct buddy_t *buddy, uint16_t p, uint8_t v);

// buddy_split_parent recursively split parents until we find a parent that is already split
// if the split can be done it'll mark the parent as split
static uint16_t buddy_split_mark_parent(struct buddy_t *buddy, uint16_t p);

// buddy_coalesce attempts to free and collapse/coalesce buddies to mark parents as free
static void buddy_coalesce(struct buddy_t *buddy, uint8_t o, uint16_t p);

bool buddy_is_full(struct buddy_t *buddy, uint8_t order)
{
	for (int i = BUDDY_MAX_ORDER; i >= order; i--)
		if (buddy->freelist[i] != 0)
			return false;

	return true;
}

static bool buddy_is_parent_split(struct buddy_t *buddy, uint16_t p)
{
	if (p == 0)
		return true;

	uint8_t pv = BUDDY_GET_POSITION(buddy, BUDDY_PARENT_POSITION(p));
	uint8_t mv = BUDDY_GET_POSITION(buddy, p);
	uint8_t cv = BUDDY_GET_POSITION(buddy, BUDDY_COMPANION_POSITION(p));

	return pv == 1 && (mv == 1 || cv == 1);
}

static bool buddy_is_parent_allocd(struct buddy_t *buddy, uint16_t p)
{
	if (p == 0)
		return false;

	uint8_t pv = BUDDY_GET_POSITION(buddy, BUDDY_PARENT_POSITION(p));
	uint8_t mv = BUDDY_GET_POSITION(buddy, p);
	uint8_t cv = BUDDY_GET_POSITION(buddy, BUDDY_COMPANION_POSITION(p));

	return pv == 1 && mv == 0 && cv == 0;
}

static uintptr_t buddy_pos_to_addr(int pos)
{
	while (pos < BUDDY_MIN_BUDDY_FOR_ORDER(0))
		pos = (pos << 1) + 1;

	pos -= BUDDY_MIN_BUDDY_FOR_ORDER(0);

	return pos * BUDDY_BLOCK_SIZE;
}

void buddy_init(struct buddy_t *buddy)
{
	for (int i = 0; i <= BUDDY_MAX_ORDER; i++)
	{
		buddy->freelist[i] = 1;
	}
}

static uint16_t buddy_order_next_free_pos(struct buddy_t *buddy, uint8_t order)
{
	for (
		uint16_t pos = BUDDY_MIN_BUDDY_FOR_ORDER(order);
		pos < BUDDY_MAX_BUDDY_FOR_ORDER(order);
		pos++)
	{
		// skip anything either split or allocated
		if (BUDDY_GET_POSITION(buddy, pos) == 1)
			continue;

		// traverse up the tree to see if anything further up has been split
		uint16_t ppos = pos;
		while (ppos > 0)
		{
			if (buddy_is_parent_allocd(buddy, ppos))
				break;

			if (buddy_is_parent_split(buddy, ppos))
			{
				return pos - BUDDY_MIN_BUDDY_FOR_ORDER(order) + 1;
			}

			ppos = BUDDY_PARENT_POSITION(ppos);
		}
	}

	return 0;
}

static uint16_t buddy_split_mark_parent(struct buddy_t *buddy, uint16_t p)
{
	if (p == 0)
	{
		buddy_set_position(buddy, p, 1);
		return 0;
	}

	if (BUDDY_GET_POSITION(buddy, p) == 1 || buddy_is_parent_allocd(buddy, p))
	{
		return 2;
	}

	if (buddy_is_parent_split(buddy, p))
	{
		buddy_set_position(buddy, p, 1);
		return 0;
	}

	uint16_t ppos = BUDDY_PARENT_POSITION(p);

	int ps = buddy_split_mark_parent(buddy, ppos);
	if (ps != 2)
		buddy_set_position(buddy, p, 1);

	return ps;
}

static void buddy_set_position(struct buddy_t *buddy, uint16_t p, uint8_t v)
{
	uint16_t pChar = p / 8;
	uint8_t pOffset = (p % 8);

	uint8_t cbv = buddy->buddy_tree[pChar];

	v = (v & 0x1) << (pOffset);
	cbv &= ~(1 << pOffset);

	buddy->buddy_tree[pChar] = v | cbv;
}

static void buddy_coalesce(struct buddy_t *buddy, uint8_t order, uint16_t p)
{
	buddy_set_position(buddy, p, 0);

	uint16_t fpos = p - BUDDY_MIN_BUDDY_FOR_ORDER(order) + 1;

	if (fpos < buddy->freelist[order] || buddy->freelist[order] == 0)
	{
		buddy->freelist[order] = fpos;
	}

	if (p == 0)
		return;

	if (BUDDY_GET_POSITION(buddy, BUDDY_COMPANION_POSITION(p)) == 0)
	{
		return buddy_coalesce(buddy, order + 1, BUDDY_PARENT_POSITION(p));
	}
}

void *buddy_alloc(struct buddy_t *buddy, int order)
{
	if (order > BUDDY_MAX_ORDER)
	{
		return 0;
	}

	// find a empty or partial buddy
	struct buddy_t *candidate = buddy;
	while (candidate)
	{
		if (buddy_is_full(candidate, order))
		{
			candidate = candidate->next;
		}
		else
		{
			break;
		}
	}

	if (!candidate)
		return 0;

	// find a slot
	uint16_t freepos = candidate->freelist[order];

	if (freepos > 0 && (freepos - 1) < BUDDY_MAX_BUDDY_FOR_ORDER(order))
	{
		uint16_t pos = BUDDY_ORDER_POSITION_OFFSET(order, freepos - 1);
		uint8_t v = BUDDY_GET_POSITION(candidate, pos);
		void *addr = (void *)((uintptr_t)buddy_pos_to_addr(pos) + (uintptr_t)&candidate->arena[0]);

		if (v)
		{
			// somethings wrong!
			// our freelist says it should be free but is not
			terminal_log("Unexpected value found in buddy freelist!!");
			return 0;
		}

		if (buddy_is_parent_split(candidate, pos))
		{
			// fast alloc
			buddy_set_position(candidate, pos, 1);

			for (int orderToUpdate = order; orderToUpdate >= 0; orderToUpdate--)
			{
				candidate->freelist[orderToUpdate] = buddy_order_next_free_pos(candidate, orderToUpdate);
			}

			buddy->allocs++;

			return addr;
		}

		// split parent
		int split = buddy_split_mark_parent(candidate, pos);
		switch (split)
		{
		case 0:
			// TODO(tcfw) get split mark to return the max order it split up to
			buddy_set_position(buddy, pos, 1);
			break;
		case 1:
			// fell below max order - should never happen
			terminal_log("ran past buddy max order!");
			return 0;
		case 2:
			// position was already allocated, meaning our freelist is corrupt
			break;
		default:
			terminal_log("unexpected return from buddy split mark parent!");
			return 0;
		}

		// recalculate for all orders
		for (int orderToUpdate = BUDDY_MAX_ORDER; orderToUpdate >= 0; orderToUpdate--)
		{
			candidate->freelist[orderToUpdate] = buddy_order_next_free_pos(candidate, orderToUpdate);
		}

		buddy->allocs++;

		return addr;
	}

	return 0;
}

void buddy_free(struct buddy_t *buddy, void *ptr)
{
	struct buddy_t *candidate = buddy;
	uintptr_t reladdr = 0;
	uint64_t l0_offset = 0;

	while (candidate)
	{
		reladdr = (uintptr_t)ptr - (uintptr_t)&candidate->arena[0];
		l0_offset = reladdr / BUDDY_BLOCK_SIZE;

		if (l0_offset < BUDDY_MAX_BUDDY_FOR_ORDER(0))
		{
			break;
		}

		candidate = candidate->next;
	}

	if (!candidate)
		// not a valid address
		return;

	uint8_t order = 0;
	uint16_t pos = l0_offset + BUDDY_MIN_BUDDY_FOR_ORDER(0);

	while (order < BUDDY_MAX_ORDER)
	{
		if (BUDDY_GET_POSITION(candidate, pos) == 1)
		{
			break;
		}

		order++;
		pos = BUDDY_PARENT_POSITION(pos);
		if (order > BUDDY_MAX_ORDER)
		{
			// was not allocd
			return;
		}
	}

	buddy_coalesce(candidate, order, pos);

	buddy->frees++;
}