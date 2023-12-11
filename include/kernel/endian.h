#ifndef _KERNEL_ENDIAN_H
#define _KERNEL_ENDIAN_H

#include <stdint.h>
#define REVERSE_UINT16(n) ((uint16_t)((((n) & 0xFF) << 8) | \
									  (((n) & 0xFF00) >> 8)))

#define REVERSE_UINT32(n) __builtin_bswap32(n)

#define REVERSE_UINT64(n) __builtin_bswap64(n)

#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
#define LITTLE_ENDIAN_UINT16(n) (n)
#define LITTLE_ENDIAN_UINT32(n) (n)
#define BIG_ENDIAN_UINT16(n) REVERSE_UINT16(n)
#define BIG_ENDIAN_UINT32(n) REVERSE_UINT32(n)
#define BIG_ENDIAN_UINT64(n) REVERSE_UINT64(n)
#elif __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
#define LITTLE_ENDIAN_UINT16(n) REVERSE_UINT16(n)
#define LITTLE_ENDIAN_UINT32(n) REVERSE_UINT32(n)
#define BIG_ENDIAN_UINT16(n) (n)
#define BIG_ENDIAN_UINT32(n) (n)
#define BIG_ENDIAN_UINT64(n) (n)
#else
#error unsupported endianness
#endif

#endif