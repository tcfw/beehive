#ifndef _KERNEL_STRINGS_H
#define _KERNEL_STRINGS_H

char *ftoc(double i, int prec, char *buf);

char *itoh(unsigned long i, char *buf);
int ksprintf(char *buf, const char *fmt, ...);

#endif