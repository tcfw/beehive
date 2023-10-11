#ifndef _KERNEL_STRINGS_H
#define _KERNEL_STRINGS_H

#include "unistd.h"

// Int to 32-bit hex string
char *itoh(unsigned long i, char *buf);

// double float to string
char *ftoc(double i, int prec, char *buf);

// Format a string given the format and arguments
int ksprintf(char *buf, const char *fmt, ...);

// Format a string with the given argument list
int ksprintfz(char *buf, const char *fmt, __builtin_va_list argp);

// strcmp Compare str1 and str2, returning less than (-1), equal (0) to or
// greater than zero (1) if str1 is lexicographically less than,
// equal to or greater than str2.
int strcmp(const char *, const char *);

// strlen calculates the length of a given string
size_t strlen(const char *);

char *strcpy(char *dest, const char *src);

// Copy memory from src to dest at the given size
void *memcpy(void *dest, const void *src, size_t n);

// Set the memory at dest with val for the given length
void *memset(void *dest, int val, size_t len);

#endif