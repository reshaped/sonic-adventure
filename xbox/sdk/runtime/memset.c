/* memset for the port: make pc compiles this in place of
 * sdk/runtime/memset.c, whose GCC side is SH-4 code. no_builtin keeps Clang
 * from writing the loops as a call to memset */
#include <stddef.h>
#include <stdint.h>
#include <string.h>

__attribute__((no_builtin)) void *memset(void *s, int c, size_t n)
{
    unsigned char *to = s;
    uint32_t word = (unsigned char)c * 0x01010101u;

    while (n > 0 && ((uintptr_t)to & 3) != 0) {
        *to++ = (unsigned char)c;
        n--;
    }
    for (; n >= 4; n -= 4, to += 4) {
        *(uint32_t *)to = word;
    }
    while (n-- > 0) {
        *to++ = (unsigned char)c;
    }
    return s;
}
