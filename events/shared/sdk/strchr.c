#include <string.h>

char *strchr(const char *s, int c)
{
    const char *p = s;
    /* @hack redundant: register on ch, which SHC sign-extends from c's R5
     * into R4 (EXTS.B R5,R4 at 0x0cb84fb0 in EV001E); without it, 14 of 14
     * words differ */
    register int ch = (char)c; /* NOLINT(bugprone-signed-char-misuse) */

    while (*p != ch) {
        if (*p == '\0') {
            return NULL;
        }
        p++;
    }
    return (char *)p;
}
