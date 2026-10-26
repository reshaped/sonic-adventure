#include <machine.h>
#include <string.h>

int strcmp(const char *s1, const char *s2)
{
    return _builtin_strcmp(s1, s2);
}
