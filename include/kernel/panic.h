#ifndef _KERNEL_PANIC_H
#define _KERNEL_PANIC_H

void panic(char *msg);

void panicf(char *format, ...);

#endif