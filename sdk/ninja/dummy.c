/* convert s to uppercase in place, by toupper */
#include <ctype.h>
#include <ninja.h>

void strupr(char *s)
{
    while (*s) {
        *s = toupper(*s);
        s++;
    }
    *s = 0;
}
