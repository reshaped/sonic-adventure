#include <ctype.h>

int toupper(int c)
{
    if (_ctype[(unsigned char)c] & _LOWER) {
        return c - ('a' - 'A');
    }
    return c;
}
