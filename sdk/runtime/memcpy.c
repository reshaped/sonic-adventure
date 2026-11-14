#include <string.h>
#ifdef DREAMCAST
#ifdef NONMATCHING
#include <zam.h>
#include <sh4zam/shz_mem.h>
#include <stdint.h>

/* the shipped memcpy below defines memmove, since it copies blocks that
 * overlap as memmove must. this side's own memcpy comes after it */
#define memcpy memmove
#endif
#endif

/* it copies backward when s1 is above s2, so blocks that overlap copy as
 * memmove copies them */
void *memcpy(void *s1, const void *s2, size_t n)
{
    size_t i;
    unsigned char *to;
    const unsigned char *from;

    if (s1 != s2) {
        if (s1 < s2) {
            to = s1;
            from = s2;
            for (i = 0; i < n; ++i) {
                *to++ = *from++;
            }
        } else {
            to = (unsigned char *)s1 + n;
            from = (const unsigned char *)s2 + n;
            for (i = 0; i < n; ++i) {
                *--to = *--from;
            }
        }
    }
    return s1;
}

#ifdef DREAMCAST
#ifdef NONMATCHING
#undef memcpy

/* blocks that overlap copy by memmove, as the shipped memcpy copies them.
 * the rest copy by shz_memcpy4 or shz_memcpy1, not by shz_memcpy, which
 * gives a block of 128 bytes or more to shz_memcpy128_sh4_. the pinned
 * SH4ZAM keeps that routine's saves above R15, below which syIntMasterISR,
 * in sdk/shinobi/syIntIsr.src, pushes, but the routine has yet to run on the
 * console */
void *memcpy(void *s1, const void *s2, size_t n)
{
    uintptr_t to = (uintptr_t)s1;
    uintptr_t from = (uintptr_t)s2;

    if (to < from + n && from < to + n) {
        return memmove(s1, s2, n);
    }
    if ((to | from | n) % 4 == 0) {
        return shz_memcpy4(s1, s2, n);
    }
    return shz_memcpy1(s1, s2, n);
}
#endif
#endif
