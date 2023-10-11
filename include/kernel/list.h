#ifndef _KERNEL_LIST_H
#define _KERNEL_LIST_H

#include "stdbool.h"

struct list_head
{
	struct list_head *next, *prev;
};

#define LIST_HEAD_INIT(name) \
	{                        \
		&(name), &(name)     \
	}

#define LIST_HEAD(name) \
	struct list_head name = LIST_HEAD_INIT(name)

#define list_empty(list) \
	(list.next == &list)

static inline void INIT_LIST_HEAD(struct list_head *list)
{
	list->next = list;
	list->prev = list;
}

static inline void list_del(struct list_head *entry)
{
	struct list_head *prev = entry->prev;
	struct list_head *next = entry->next;
	prev->next = next;
	next->prev = prev;
}

static inline bool list_is_empty(struct list_head *head)
{
	return head->next == head;
}

int list_is_head(struct list_head *pos, struct list_head *head);
struct list_head *_list_prepend(struct list_head *list, struct list_head *entry);
struct list_head *_list_append(struct list_head *list, struct list_head *entry);

static inline void __list_add(struct list_head *new,
							  struct list_head *prev,
							  struct list_head *next)
{
	next->prev = new;
	new->next = next;
	new->prev = prev;
	prev->next = new;
}

static inline void list_add(struct list_head *new, struct list_head *head)
{
	__list_add(new, head, head->next);
}

static inline void list_add_tail(struct list_head *new, struct list_head *head)
{
	__list_add(new, head->prev, head);
}

#define list_head_foreach(pos, head) \
	for (pos = (head)->next; !list_is_head((struct list_head *)pos, (struct list_head *)head); pos = pos->list.next)

#endif