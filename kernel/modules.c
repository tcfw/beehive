#include <kernel/modules.h>
#include <kernel/tty.h>

void mod_init(void)
{
	terminal_log("Loading modules...");

	for (init_func_ptr_t *cb =
			 (init_func_ptr_t *)({
				 extern init_func_ptr_t __start_init;
				 &__start_init;
			 });
		 cb !=
		 (init_func_ptr_t *)({
			 extern init_func_ptr_t __stop_init;
			 &__stop_init;
		 });
		 ++cb)
	{
		cb->callback();
	}

	terminal_log("Loaded modules");
}