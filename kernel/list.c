#include <kernel/list.h>

int list_is_head(struct list_head *pos, struct list_head *head)
{
	return pos == head;
}