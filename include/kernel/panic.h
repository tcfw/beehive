#ifndef _KERNEL_PANIC_H
#define _KERNEL_PANIC_H

// Print a panic message and then halt
void panic(char *msg);

// Print a formatted panic message and then halt
void panicf(char *format, ...);

#endif