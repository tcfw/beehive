#ifndef _KERNEL_TTY_H
#define _KERNEL_TTY_H

#include "stddef.h"
#include "stdint.h"

void terminal_initialize(void);
void terminal_set_bar(uint64_t addr);
void terminal_putchar(char c);
void terminal_write(const char *data, size_t size);
void terminal_writestring(char *);
void terminal_log(char *);
void terminal_logf(char *, ...);

#endif