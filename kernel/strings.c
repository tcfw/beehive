#include <kernel/strings.h>
#include <kernel/tty.h>
#include "stdint.h"
#include "stdbool.h"

// Int to Hex
char *itoh(unsigned long i, char *buf)
{
	const char *itoh_map = "0123456789ABCDEF";
	int n;
	int b;
	int z;

	for (z = 0, n = 8; n > -1; --n)
	{
		b = i >> (n * 4);
		buf[z] = itoh_map[b & 0xf];
		++z;
	}
	buf[z] = 0;
	return buf;
}

char *ftoc(double i, int prec, char *buf)
{
	char *s = buf + 256; // go to end of buffer
	uint16_t decimals;	 // variable to store the decimals
	int units;			 // variable to store the units (part to left of decimal place)

	int precMulti = 10;

	for (int i = 1; i < prec; i++)
	{
		precMulti *= 10;
	}

	if (i < 0)
	{ // take care of negative numbers
		decimals = (int)(i * -precMulti) % precMulti;
		units = (int)(-1 * i);
	}
	else
	{ // positive numbers
		decimals = (int)(i * precMulti) % precMulti;
		units = (int)i;
	}

	*--s = 0; // null terminate

	for (int i = 0; i < prec; i++)
	{
		*--s = (decimals % 10) + '0';
		decimals /= 10; // repeat for as many decimal places as you need
	}
	// *--s = (decimals % 10) + '0';
	*--s = '.';

	if (units == 0)
	{
		*--s = '0';
	}

	while (units > 0)
	{
		*--s = (units % 10) + '0';
		units /= 10;
	}
	if (i < 0)
		*--s = '-'; // unary minus sign for negative numbers

	return s;
}

int ksprintfz(char *buf, const char *fmt, __builtin_va_list argp)
{
	const char *p;
	int i;
	double f;
	char *s;
	char fmtbuf[256];
	int x, y;
	int numPrec = 5;
	bool contExpr = false;

	x = 0;
	for (p = fmt; *p != '\0'; p++)
	{
		if (*p == '\\')
		{
			switch (*++p)
			{
			case 'n':
				buf[x++] = '\n';
				break;
			default:
				break;
			}
			continue;
		}

		if (*p != '%' && !contExpr)
		{
			buf[x++] = *p;
			continue;
		}

		if (contExpr)
		{
			contExpr = false;
			p--;
		}

		switch (*++p)
		{
		case 'c':
			i = __builtin_va_arg(argp, int);
			buf[x++] = i;
			break;
		case 's':
			s = __builtin_va_arg(argp, char *);
			for (y = 0; s[y]; ++y)
			{
				buf[x++] = s[y];
			}
			break;
		case 'x':
			i = __builtin_va_arg(argp, int);
			s = itoh(i, fmtbuf);
			for (y = 0; s[y]; ++y)
			{
				buf[x++] = s[y];
			}
			break;
		case '.':
			numPrec = *++p - '0';
			contExpr = true;
			break;
		case 'f':
			f = __builtin_va_arg(argp, double);
			s = ftoc(f, numPrec, fmtbuf);
			while (*s)
			{
				buf[x++] = *s++;
			}
			break;
		case '%':
			buf[x++] = '%';
			break;
		}
	}

	buf[x] = 0;

	return x;
}

int ksprintf(char *buf, const char *fmt, ...)
{
	__builtin_va_list argp;
	__builtin_va_start(argp, fmt);

	int x = ksprintfz(buf, fmt, argp);

	__builtin_va_end(argp);

	return x;
}