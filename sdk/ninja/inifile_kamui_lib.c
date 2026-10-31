/* Kamui profile settings, with no file behind them */
#include <kamui.h>
#include <string.h>

KMINT32 kmiGetProfileInt(const char *section, const char *key, KMINT32 fallback)
{
    KMINT32 value;

#ifdef GCC
    /* repairs the unset value the tag below names: GCC gives fallback for
     * any "HARDWARE" key but "EnableTexture16MB", as for another section */
    value = fallback;
#endif
    if (strcmp(section, "HARDWARE") == 0) {
        /* NOLINTNEXTLINE(clang-diagnostic-sometimes-uninitialized) */
        if (strcmp(key, "EnableTexture16MB") == 0) {
            value = 0;
        }
    } else {
        value = fallback;
    }
    /* @bug uninitialized: in section "HARDWARE", any key but
     * "EnableTexture16MB" returns what strcmp left in R4: the address in key
     * just past the first byte where the two names differ (BF at 0x8c67447c,
     * MOV R4,R0 at 0x8c67448a in 1ST_READ.BIN; 0x8ce1a2e0 and 0x8ce1a2ee in
     * LOADER.BIN) */
    return value;
}
