#ifndef _KERNEL_TTY_H
#define _KERNEL_TTY_H

#include "stddef.h"
#include "stdint.h"

// Initiate terminal output
void terminal_initialize(void);

// Set the base address of the terminal out device
void terminal_set_bar(uint64_t addr);

// Put a character to terminal
void terminal_putchar(char c);

// Put a string on the terminal at a specific length
void terminal_write(const char *data, size_t size);

// Put a null terminated string on the terminal
void terminal_writestring(char *);

// Log an entry to the terminal
void terminal_log(char *);

// Log a formatted entry to the terminal
void terminal_logf(char *, ...);

#endif