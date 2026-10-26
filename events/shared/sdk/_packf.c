/* a float from its biased exponent and its fraction */
#include <multiprec.h>
#include <string.h>

#include "shared/sdk/scan.h"

float _packf(short exponent, unsigned char *fraction)
{
    float value;
    unsigned char word[4];

    fraction[0] &= 0x7f;
    memcpy(word + 1, fraction, 3);
    /* word holds the float most significant byte first: the sign, 0, and the
     * exponent's top 7 bits, then the exponent's low bit above the fraction */
    word[0] = exponent / 2;
    word[1] |= (exponent % 2) << 7;
    /* _mvito4b reverses word's bytes into value: it reads them as a
     * little-endian long and writes the long most significant first.
     * reversed, they are the float in memory order */
    _mvito4b((unsigned char *)&value, (const unsigned long *)word);
    return value;
}
