#include <multiprec.h>

void _mvito4b(unsigned char *bytes, const unsigned long *word)
{
    int i;

    for (i = 0; i < 4; ++i) {
        bytes[i] = *word >> (24 - (i * 8));
    }
}
