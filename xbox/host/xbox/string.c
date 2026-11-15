/* memcpy, memmove, memset and memcmp for the whole XBE, in place of pdclib's, which
 * nxdk links and which go a byte at a time in a loop (nxdk builds with
 * -fno-builtin, so nothing replaces them). the vertex copy into the ring
 * alone, 28 bytes a vertex into write-combined memory, was 22% of the
 * stage's samples. these go four bytes at a time with the CPU's string
 * instructions, then the rest; as asm, so that no compiler turns them back
 * into calls of themselves */
#include <stddef.h>
#include <stdint.h>

void *memcpy(void *restrict to, const void *restrict from, size_t count)
{
    void *d = to;
    const void *s = from;
    size_t words = count >> 2, bytes = count & 3;

    __asm__ volatile("cld\n\trep movsl\n\tmovl %3, %%ecx\n\trep movsb"
                     : "+D"(d), "+S"(s), "+c"(words)
                     : "r"(bytes)
                     : "memory");
    return to;
}

void *memmove(void *to, const void *from, size_t count)
{
    uint8_t *d = to;
    const uint8_t *s = from;

    if (d <= s || d >= s + count) {
        return memcpy(to, from, count);
    }
    /* overlapping, the destination above: from the end down */
    {
        void *de = d + count - 1;
        const void *se = s + count - 1;
        size_t n = count;

        __asm__ volatile("std\n\trep movsb\n\tcld" : "+D"(de), "+S"(se), "+c"(n) : : "memory");
    }
    return to;
}

void *memset(void *to, int value, size_t count)
{
    void *d = to;
    uint32_t fill = (uint8_t)value * 0x01010101u;
    size_t words = count >> 2, bytes = count & 3;

    __asm__ volatile("cld\n\trep stosl\n\tmovl %2, %%ecx\n\trep stosb"
                     : "+D"(d), "+c"(words)
                     : "r"(bytes), "a"(fill)
                     : "memory");
    return to;
}

/* four bytes at a time while they match, then the first that differs */
int memcmp(const void *a, const void *b, size_t count)
{
    const uint8_t *p = a, *q = b;

    while (count >= 4 && *(const uint32_t *)p == *(const uint32_t *)q) {
        p += 4;
        q += 4;
        count -= 4;
    }
    for (; count != 0; count--, p++, q++) {
        if (*p != *q) {
            return *p - *q;
        }
    }
    return 0;
}
