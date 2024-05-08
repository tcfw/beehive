#include <kernel/strings.h>
#include <kernel/tty.h>
#include <kernel/stdint.h>
#include <kernel/stdbool.h>

static const char *itoh_map = "0123456789ABCDEF";

// Int to Hex
char *itoh(unsigned long i, char *buf)
{

	int n=8;
	int b=0;

	for (n = 8; n > -1; --n)
	{
		b = i >> (n * 4);
		*buf++ = itoh_map[b & 0xf];
	}

	*buf = 0;
	return buf-8;
}

// long int to Hex
char *litoh(unsigned long long i, char *buf)
{
	int n=16;
	int b=0;

	for (n = 16; n > -1; --n)
	{
		b = i >> (n * 4);
		*buf++ = itoh_map[b & 0xf];
	}

	*buf = 0;
	return buf-16;
}

char *itoa(long i, char *buf, int base)
{
	char *s = buf + 256;
	int wn = 0;
	if (i > 0)
		wn = 1;

	*--s = 0; // null terminate

	if (i == 0)
	{
		*--s = '0';
		return s;
	}

	while (i != 0)
	{
		if (wn == 1)
			*--s = (char)(i % base) + '0';
		else
			*--s = '0' - (char)(i % base);
		i /= base;
	}

	if (wn == 0)
		*--s = '-';

	return s;
}

char *uitoa(unsigned long i, char *buf, int base)
{
	char *s = buf + 256;

	*--s = 0; // null terminate

	while (i != 0)
	{
		*--s = (char)(i % base) + '0';
		i /= base;
	}

	return s;
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
	long long li;
	unsigned int ui;
	double f;
	char *s;
	char fmtbuf[256];
	int x = 0, y = 0;
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

		memset(&fmtbuf, 0, sizeof(fmtbuf));

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
		case 'X':
			li = __builtin_va_arg(argp, uint64_t);
			s = litoh(li, fmtbuf);
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
		case 'd':
			i = __builtin_va_arg(argp, int);
			s = itoa(i, fmtbuf, 10);
			while (*s)
			{
				buf[x++] = *s++;
			}
			break;
		case 'u':
			ui = __builtin_va_arg(argp, unsigned int);
			s = uitoa(ui, fmtbuf, 10);
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

int strcmp(const char *str1, const char *str2)
{
	const unsigned char *s1 = (const unsigned char *)str1;
	const unsigned char *s2 = (const unsigned char *)str2;
	unsigned char c1, c2;

	do
	{
		c1 = (unsigned char)*s1++;
		c2 = (unsigned char)*s2++;
		if (c1 == '\0')
			return c1 - c2;
	} while (c1 == c2);

	return c1 - c2;
}

size_t strlen(const char *str)
{
	size_t len = 0;

	while (*str++)
		len++;

	return len;
}

char *strcpy(char *dest, const char *src)
{
	return memcpy(dest, src, strlen(src) + 1);
}

void *memcpy(void *dest, const void *src, size_t len)
{
	char *d = dest;
	const char *s = src;
	while (len--)
		*d++ = *s++;
	return dest;
}

void *memset(void *dest, int val, size_t len)
{
	unsigned char *ptr = dest;
	while (len-- > 0)
		*ptr++ = val;
	return dest;
}

int memcmp(const void *str1, const void *str2, size_t count)
{
	const unsigned char *s1 = (const unsigned char *)str1;
	const unsigned char *s2 = (const unsigned char *)str2;

	while (count-- > 0)
	{
		if (*s1++ != *s2++)
			return s1[-1] < s2[-1] ? -1 : 1;
	}
	return 0;
}