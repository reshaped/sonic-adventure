#include <string.h>
#ifdef GCC
#include <zam.h>
#include <sh4zam/shz_mem.h>
#include <stdint.h>

/* the shipped memcpy below defines memmove, since it copies blocks that
 * overlap as memmove must. GCC's own memcpy comes after it */
#define memcpy memmove
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

#ifdef GCC
#undef memcpy

/* blocks that overlap copy by memmove, as the shipped memcpy copies them.
 * memcpy calls no SH4ZAM routine that stores below R15, where
 * syIntMasterISR, in sdk/shinobi/syIntIsr.src, saves the registers of the
 * code an interrupt stops. shz_memcpy128_sh4_ stores FR12 to FR15 and
 * XMTRX there without moving R15. shz_memcpy128 calls it for a block of any
 * size. shz_memcpy32 calls it from 128 bytes, and shz_memcpy64 from 256.
 * shz_memcpy reaches it through shz_memcpy32 */
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
