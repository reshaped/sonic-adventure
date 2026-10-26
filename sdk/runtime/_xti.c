#include <multiprec.h>
#include <string.h>

extern int _allzero(const unsigned char *bytes, int n);
extern void _unpack(double value, signed char *sign, short *exponent,
                    unsigned char *fraction);
/* an estimate of value's decimal exponent */
extern int _log10(double value);
extern void _pow5(int n, unsigned char *bytes);
/* divide the number exponent and fraction give by 10 to the n. integer gets
 * the quotient rounded to a whole number, in 8 bytes, most significant
 * first */
extern void _calcnpw(short exponent, unsigned char *fraction, int n,
                     unsigned char *integer);

/* split value into *sign, 1 or -1, and *integer times 10 to the *scale.
 * *integer is 8 bytes, most significant first, rounded to digits decimal
 * digits. a zero gives an *integer and *scale of 0. it returns 1 or -1 for
 * plus or minus infinity, 2 for a NaN, and 0 otherwise. an infinity or a NaN
 * leaves *integer and *scale unset */
int _xti(double value, int digits, unsigned char *integer, int *scale,
         signed char *sign)
{
    unsigned char *bytes;
    unsigned char fraction[8];
    unsigned char upper[8];
    unsigned char lower[8];
    unsigned char copy[8];
    short raised;
    int i;
    int shift;
    int byte_bits;
    short exponent;

    /* @hack spelling: byte_bits, raised and bytes set in that order, which
     * gives the prologue its schedule (MOV #0,R14 at 0x8c12721a, MOV #8,R13
     * at 0x8c12722a); in the order they are declared, 11 of 362 words
     * differ */
    /* @hack spelling: the 8 as a variable, which SHC keeps in a frame word
     * (MOV.L R13,@(8,R15) at 0x8c127230) and loads as each division's divisor
     * (MOV.L @(8,R15),R0 at 0x8c127360); as the constant 8, the frame is 4
     * bytes smaller, the function 8 bytes shorter, and 343 of 362 words
     * differ */
    byte_bits = 8;
    raised = 0;
    bytes = (unsigned char *)&value;

    /* zero of either sign, which _unpack does not take */
    if ((bytes[7] & 0x7f) == 0 && _allzero(bytes, 7)) {
        for (i = 0; i < 8; i++) {
            integer[i] = 0;
        }
        *scale = 0;
        if (bytes[7] & 0x80) {
            *sign = -1;
        } else {
            *sign = 1;
        }
        return 0;
    }
    _unpack(value, sign, &exponent, fraction);
    /* @hack spelling: value's sign cleared through a fresh address of value,
     * which SHC forms from R15 (MOV R15,R3 and ADD #52,R3 at 0x8c12729c and
     * 0x8c12729e); through bytes, exponent and the address of value's last
     * byte trade frame words, the function is 4 bytes longer, and 157 of 362
     * words differ */
    if (*sign == -1) {
        ((unsigned char *)&value)[7] &= 0x7f;
    }
    if (exponent == 0x7ff) {
        if (bytes[6] == 0xf0 && _allzero(bytes, 6)) {
            if (*sign == 1) {
                return 1;
            } else if (*sign == -1) {
                return -1;
            }
        } else {
            return 2;
        }
    }
    exponent -= 1023;
    if (exponent >= 0) {
        i = _log10(value);
    } else {
        i = _log10(value) - 1;
    }
    fraction[7] = 0;
    *scale = i - digits + 1;
    /* upper and lower get 10 to the digits and 10 to the digits - 1, the
     * bounds on *integer. each is 5 to that power, shifted left as many
     * bits */
    _pow5(digits, upper);
    shift = digits / byte_bits;
    for (i = 0; i < 8 - shift; i++) {
        upper[i] = upper[i + shift];
    }
    for (; i < 8; i++) {
        upper[i] = 0;
    }
    _lsfts(upper, 8, digits - (shift * 8));
    _pow5(digits - 1, lower);
    shift = (digits - 1) / byte_bits;
    for (i = 0; i < 8 - shift; i++) {
        lower[i] = lower[i + shift];
    }
    for (; i < 8; i++) {
        lower[i] = 0;
    }
    _lsfts(lower, 8, digits - 1 - (shift * 8));
    /* adjust the scale until *integer lies between the bounds; once the scale
     * has been raised it is not lowered again */
retry:
    _calcnpw(exponent, fraction, *scale, integer);
    memcpy(copy, integer, 8);
    if (memcmp(copy, upper, 8) > 0) {
        raised = 1;
        ++*scale;
        goto retry;
    }
    memcpy(copy, integer, 8);
    if (memcmp(copy, upper, 8) == 0) {
        ++*scale;
        _calcnpw(exponent, fraction, *scale, integer);
    } else {
        memcpy(copy, integer, 8);
        if (memcmp(copy, lower, 8) < 0 && raised == 0) {
            --*scale;
            goto retry;
        }
    }
    return 0;
}
