#ifndef _KERNEL_INIT_H
#define _KERNEL_INIT_H

typedef void (*init_cb)(void);

typedef struct init_func_ptr_s
{
	init_cb callback;
} init_func_ptr_t;

#define MOD_INIT(func_cb)                        \
	static init_func_ptr_t ptr_##func_cb         \
		__attribute((used, section("init"))) = { \
			.callback = func_cb,                 \
	}

#endif