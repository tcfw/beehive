#ifndef _KERNEL_STRINGS_H
#define _KERNEL_STRINGS_H

#include "unistd.h"

char *itoh(unsigned long i, char *buf);
char *ftoc(double i, int prec, char *buf);

int ksprintf(char *buf, const char *fmt, ...);
int ksprintfz(char *buf, const char *fmt, __builtin_va_list argp);

// strcmp Compare str1 and str2, returning less than (-1), equal (0) to or
// greater than zero (1) if str1 is lexicographically less than,
// equal to or greater than str2.
int strcmp(const char *, const char *);

// strlen calculates the length of a given string
size_t strlen(char *);

void *memcpy(void *dest, const void *src, size_t n);
void *memset(void *dest, int val, size_t len);

#endif