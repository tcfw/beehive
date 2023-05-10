#ifndef _KERNEL_STRINGS_H
#define _KERNEL_STRINGS_H

char *itoh(unsigned long i, char *buf);
char *ftoc(double i, int prec, char *buf);

int ksprintf(char *buf, const char *fmt, ...);
int ksprintfz(char *buf, const char *fmt, __builtin_va_list argp);

#endif