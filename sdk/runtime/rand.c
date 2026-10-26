#include <stdlib.h>

extern unsigned long _rnext;

int rand(void)
{
    _rnext = (_rnext * 1103515245) + 12345;
    return (_rnext >> 16) & RAND_MAX;
}
