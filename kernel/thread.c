#include <kernel/thread.h>
#include <kernel/context.h>

void init_thread(thread_t *thread)
{
	init_context(&thread->ctx);
}