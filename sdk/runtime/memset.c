#include <string.h>

void *memset(void *s, int c, size_t n)
{
    size_t i;
    unsigned char *to = s;

    for (i = 0; i < n; ++i) {
        *to++ = (unsigned char)c;
    }
    return s;
}
