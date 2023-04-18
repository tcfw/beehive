#include <kernel/strings.h>

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

void ksprintf(char *buf, const char *fmt, ...)
{
	const char *p;
	__builtin_va_list argp;
	int i;
	char *s;
	char fmtbuf[256];
	int x, y;

	__builtin_va_start(argp, fmt);

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

		if (*p != '%')
		{
			buf[x++] = *p;
			continue;
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
		case '%':
			buf[x++] = '%';
			break;
		}
	}

	__builtin_va_end(argp);
	buf[x] = 0;
}