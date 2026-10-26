#include <string.h>

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
