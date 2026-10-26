#include <stdlib.h>

extern unsigned long _rnext;

void srand(unsigned int seed)
{
    _rnext = seed;
}
