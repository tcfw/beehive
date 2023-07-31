#ifndef _KERNEL_LIST_H
#define _KERNEL_LIST_H

struct list_head
{
	struct list_head *next, *prev;
};

int list_is_head(struct list_head *pos, struct list_head *head);
struct list_head *_list_prepend(struct list_head *list, struct list_head *entry);
struct list_head *_list_append(struct list_head *list, struct list_head *entry);

#define list_head_prepend(type, list, entry) (type) _list_prepend((struct list_head *)list, (struct list_head *)entry);
#define list_head_append(type, list, entry) (type) _list_append((struct list_head *)list, (struct list_head *)entry);

#define list_head_foreach(pos, head) \
	for (pos = (head)->list.next; !list_is_head((struct list_head *)pos, (struct list_head *)head); pos = pos->list.next)

#endif