#include <kernel/list.h>

struct list_head *_list_prepend(struct list_head *list, struct list_head *entry)
{
	if (list != 0)
	{
		entry->prev = list->prev;
		list->prev = entry;
	}

	entry->next = list;
	return entry;
}

struct list_head *_list_append(struct list_head *list, struct list_head *entry)
{
	if (list == 0)
	{
		entry->next = entry->prev = entry;
		return entry;
	}

	entry->next = list;
	list->prev->next = entry;
	entry->prev = list->prev;
	list->prev = entry;

	return list;
}

int list_is_head(struct list_head *pos, struct list_head *head)
{
	return pos == head;
}