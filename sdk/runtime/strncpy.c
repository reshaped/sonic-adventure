#include <string.h>

char *strncpy(char *s1, const char *s2, size_t n)
{
    char *to = s1;
    const char *from = s2;
    size_t i;

    for (i = 0; i < n && *from != 0; ++i) {
        *to++ = *from++;
    }
    for (; i < n; ++i) {
        *to++ = 0;
    }
    return s1;
}
