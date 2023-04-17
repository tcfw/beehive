#ifndef _KERNEL_STRINGS_H
#define _KERNEL_STRINGS_H

char *itoh(unsigned long i, char *buf);
void ksprintf(char *buf, const char *fmt, ...);

#endif