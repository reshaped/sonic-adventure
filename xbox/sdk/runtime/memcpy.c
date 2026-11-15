/* memcpy and memmove for the port: make pc compiles this in place of
 * sdk/runtime/memcpy.c, whose GCC side is SH-4 code. memcpy keeps the
 * shipped one's way with blocks that overlap, which it copies as memmove
 * does. no_builtin keeps Clang from writing the loops as calls to the
 * functions they define */
#include <stddef.h>
#include <stdint.h>
#include <string.h>

__attribute__((no_builtin)) void *memmove(void *s1, const void *s2, size_t n)
{
    unsigned char *to = s1;
    const unsigned char *from = s2;

    if (to == from || n == 0) {
        return s1;
    }
    if (to < from) {
        if ((((uintptr_t)to | (uintptr_t)from) & 3) == 0) {
            for (; n >= 4; n -= 4, to += 4, from += 4) {
                *(uint32_t *)to = *(const uint32_t *)from;
            }
        }
        while (n-- > 0) {
            *to++ = *from++;
        }
    } else {
        to += n;
        from += n;
        if ((((uintptr_t)to | (uintptr_t)from) & 3) == 0) {
            for (; n >= 4; n -= 4) {
                to -= 4;
                from -= 4;
                *(uint32_t *)to = *(const uint32_t *)from;
            }
        }
        while (n-- > 0) {
            *--to = *--from;
        }
    }
    return s1;
}

__attribute__((no_builtin)) void *memcpy(void *s1, const void *s2, size_t n)
{
    return memmove(s1, s2, n);
}
