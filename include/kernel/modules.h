#ifndef _KERNEL_MODULES_H
#define _KERNEL_MODULES_H

typedef void (*init_cb)(void);

typedef struct init_func_ptr_s
{
	init_cb callback;
} init_func_ptr_t;

#define MOD_INIT(func_cb)                            \
	static init_func_ptr_t ptr_##func_cb             \
		__attribute((used, section("mod.init"))) = { \
			.callback = func_cb,                     \
	}

void mod_init(void);

#endif