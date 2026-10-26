#include <machine.h>
#include <string.h>

char *strcpy(char *s1, const char *s2)
{
    return _builtin_strcpy(s1, s2);
}
