#ifndef _KERNEL_LIST_H
#define _KERNEL_LIST_H

struct list_head
{
	struct list_head *next, *prev;
};

#endif