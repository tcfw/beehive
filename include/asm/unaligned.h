#ifndef _GENERIC_UNALIGNED_H
#define _GENERIC_UNALIGNED_H

#include <kernel/stdint.h>
#include <kernel/endian.h>

#define __packed   __attribute__((packed))

#define __get_unaligned_t(type, ptr) ({						\
	const struct { type x; } __packed * __pptr = (typeof(__pptr))(ptr);	\
	__pptr->x;								\
})

#define __put_unaligned_t(type, val, ptr) do {					\
	struct { type x; } __packed * __pptr = (typeof(__pptr))(ptr);		\
	__pptr->x = (val);							\
} while (0)

#define get_unaligned(ptr)	__get_unaligned_t(typeof(*(ptr)), (ptr))
#define put_unaligned(val, ptr) __put_unaligned_t(typeof(*(ptr)), (val), (ptr))

static inline uint16_t get_unaligned_le16(const void *p)
{
	return LITTLE_ENDIAN_UINT16(__get_unaligned_t(int16_t, p));
}

static inline uint32_t get_unaligned_le32(const void *p)
{
	return LITTLE_ENDIAN_UINT32(__get_unaligned_t(int32_t, p));
}

static inline uint64_t get_unaligned_le64(const void *p)
{
	return LITTLE_ENDIAN_UINT64(__get_unaligned_t(int64_t, p));
}

static inline void put_unaligned_le16(uint16_t val, void *p)
{
	__put_unaligned_t(int16_t, LITTLE_ENDIAN_UINT16(val), p);
}

static inline void put_unaligned_le32(uint32_t val, void *p)
{
	__put_unaligned_t(int32_t, LITTLE_ENDIAN_UINT32(val), p);
}

static inline void put_unaligned_le64(uint64_t val, void *p)
{
	__put_unaligned_t(int64_t, LITTLE_ENDIAN_UINT64(val), p);
}

static inline uint16_t get_unaligned_be16(const void *p)
{
	return BIG_ENDIAN_UINT16(__get_unaligned_t(int16_t, p));
}

static inline uint32_t get_unaligned_be32(const void *p)
{
	return BIG_ENDIAN_UINT32(__get_unaligned_t(int32_t, p));
}

static inline uint64_t get_unaligned_be64(const void *p)
{
	return BIG_ENDIAN_UINT64(__get_unaligned_t(int64_t, p));
}

static inline void put_unaligned_be16(uint16_t val, void *p)
{
	__put_unaligned_t(int16_t, BIG_ENDIAN_UINT16(val), p);
}

static inline void put_unaligned_be32(uint32_t val, void *p)
{
	__put_unaligned_t(int32_t, BIG_ENDIAN_UINT32(val), p);
}

static inline void put_unaligned_be64(uint64_t val, void *p)
{
	__put_unaligned_t(int64_t, BIG_ENDIAN_UINT64(val), p);
}

/* Allow unaligned memory access */
void allow_unaligned(void);

#endif
