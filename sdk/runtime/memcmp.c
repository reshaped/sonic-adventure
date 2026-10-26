#include <string.h>

int memcmp(const void *s1, const void *s2, size_t n)
{
    const unsigned char *p1 = s1;
    const unsigned char *p2 = s2;
    size_t i;

    if (n == 0) {
        return 0;
    }
    for (i = 0; i < n; ++i) {
        if (*p1++ != *p2++) {
            break;
        }
    }
    return *(p1 - 1) - *(p2 - 1);
}
