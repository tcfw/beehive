#ifndef _KERNEL_BUDDY_H
#define _KERNEL_BUDDY_H

#include <kernel/unistd.h>
#include <kernel/stdint.h>
#include <kernel/stdbool.h>
#include <kernel/vm.h>

#define BUDDY_TREE_SIZE 1024 // for 1 bit per ordered slot (2 ^ (n+1) / 8) where n is max order
#define BUDDY_MAX_ORDER 12
#define BUDDY_MAX_BITS 8192
#define BUDDY_BLOCK_SIZE 4096

/*
The budy allocator uses 12 order binary buddies for 4KiB pages provides
management over 8MiB buddy.

Each buddy is linked to the next to provide managed over all available
memory space. The last buddy may not be able to provide all 12 orders.

In the buddy_tree bitmap, each bit provides a mapping of the binary tree,
1 being split/allocated, 0 being unallocated.
The first bit in the bitmap represents the whole 8MiB, the second and third
bits represent if first and second 4MiB blocks if the 8MiB block was split, the
next order down, and so on until each bit represents the individual 4KiB pages.

Order |   0  |   1  |   2   |   3   |    4  |    5   |    6   |    7   |  8   |  9   |  10  |  11  |  12   |
Size  | 4KiB | 8KiB | 16KiB | 32KiB | 64KiB | 128KiB | 256KiB | 512KiB | 1MiB | 2MiB | 4MiB | 8MiB | 16MiB |

Order 12 - [               1               ] - 8MiB
Order 11 - [       1       |       0       ] - 4MiB
Order 10 - [   1   |   1   |   0   |   0   ] - 2MiB
Order  9 - [ 0 | 0 | 0 | 1 | 0 | 0 | 0 | 0 ] - 1MiB
Order  8 - [0|0|0|0|0|0|0|0|0|0|0|0|0|0|0|0] - 512KiB
...

The bitmap example above shows that:
- Order 12 has been split
- Order 11:1 has bee split
- Order 11:2 is unallocated
- Order 10:1 has been allocated
- Order 10:2 is split
- Order 9:1-2 has been allocated at a higher order
- Order 9:3 is unallocated
- Order 9:4 has been allocated
- Order 8:1-4 has been allocated at a higher order
- Order 8:5-6 is unallocated
- Order 8:7-8 has been allocated at a higher order
...

if the children of an order are all 0, and the parent is 1, the parent is allocated
if any children of an order are 1, and the parent is 1, the parent is split

As a flatten bitmap this is represented in binary as:
11011000 00100000 00000000 0000000...
^^ ^   ^        ^                ^
|| |   |- Start of order 9
|| |- Start of order 10
||- Start of order 11
|- Start of order 12

Based on the bitmap, we can see that the addresses allocated are:
arena + 0x0 at a size of 2MiB
arena + 0x800000 at a size of 1MiB

During a free(void *), the allocated order coalesces, updating the buddy_tree and
trying to merge order back into higher orders. A buddy can be merged to a higher
order if it's companion buddy is unallocated and not split.

More info see https://en.wikipedia.org/wiki/Buddy_memory_allocation

To speed up allocs, a pointer to the next free slot in each order is maintained.
For example, give then bitmap above, the free list would slow
freelist[BUDDY_MAX_ORDER] = {
...
  7 - 5 //bit 5 is free
  8 - 3 //bit 3 is free
  9 - 3 //bit 3 is free
 10 - 2 //bit 2 is free
 11 - 0 //none free
}

Algos:
- bitmap position from order = 2^(m-n)-1 where m = max order, n = order
- parent bit = floor(p/2) or p>>1 where p = position of child bit
- child bits = p*2 & p*2+1 or p<<1 & p<<1+1 where p = position of parent bit
- char position in bitmap = floor(p/8) (+ p%8 for bit in char), where b = position of bit
- companion buddy: if(p&0x1) ? c=p-1 : c=p+1, where b = position of bit
- position to addr: while(p<mb) {p<<1} & p>>1 - BUDDY_MIN_BUDDY_FOR_ORDER(order) * block size + arena; where mb=max bit (2^13-1)
- addr to position: addr-=arena; p=2^m+(addr/bs); while(p!=1) p>>1 //start at lowest order, get offset, travel up until a 1
*/

struct buddy_t
{
	struct buddy_t *next;
	size_t size;

	uint64_t allocs;
	uint64_t frees;

	unsigned char *arena;

	uint16_t freelist[BUDDY_MAX_ORDER + 1];
	uint8_t buddy_tree[BUDDY_TREE_SIZE];
};

#define BUDDY_MAX_BUDDY_FOR_ORDER(o) ((1 << ((BUDDY_MAX_ORDER - o) + 1)) - 1)
#define BUDDY_MIN_BUDDY_FOR_ORDER(o) ((1 << (BUDDY_MAX_ORDER - o)) - 1)
#define BUDDY_PARENT_POSITION(p) (((p & 0x1) == 1) ? (p >> 1) : ((p - 1) >> 1))
#define BUDDY_CHILD_POSITION(p) (p << 1)
#define BUDDY_COMPANION_POSITION(p) (((p & 0x1) == 1) ? (p + 1) : (p - 1))
#define BUDDY_ORDER_POSITION_OFFSET(o, f) (BUDDY_MIN_BUDDY_FOR_ORDER(o) + (f))
#define BUDDY_GET_POSITION(buddy, p) ((buddy->buddy_tree[p / 8] >> (p % 8)) & 0x1)
#define BUDDY_ARENA_SIZE ((1 << (BUDDY_MAX_ORDER + 1)) * PAGE_SIZE)

/*
Note: the functions below not are thread safe. It's assumed the caller, usually mm.c, will do correct locking
*/

// buddy_init inits the buddy freelist
void buddy_init(struct buddy_t *buddy);

// buddy_alloc get a free section given the order
void *buddy_alloc(struct buddy_t *buddy, int order);

// buddy_free frees a page given a pointer
void buddy_free(struct buddy_t *buddy, void *ptr);

#endif