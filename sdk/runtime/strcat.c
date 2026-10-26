#include <string.h>

char *strcat(char *s1, const char *s2)
{
    char *to = s1;
    const char *from = s2;

    while (*to != 0) {
        to++;
    }
    /* @hack type: the cast makes the test zero-extend each copied byte, with
     * EXTU.B at 0x8c11c352 in 1ST_READ.BIN and 0x8ce049da in LOADER.BIN;
     * without it, SHC sign-extends the byte there with EXTS.B, and 1 of 15
     * words differ */
    while ((unsigned char)(*to++ = *from++) != 0) {
    }
    return s1;
}
