/* C runtime input conversion */
#ifndef EVENT_SCAN_H
#define EVENT_SCAN_H

#include <stdarg.h>
#include <stdio.h>

/* the largest width a directive may give */
#define SCAN_WIDTH_MAX 512

/* the scanner under sscanf. with string_mode nonzero it reads the string at
 * input and ignores read. with string_mode 0 it reads the FILE at input
 * through read, which must take each byte at _bufptr and count _bufcnt down.
 * at its end the scan puts its lookahead back, unless the lookahead ended
 * the input. for a FILE, that steps _bufptr back and _bufcnt up. it returns
 * how many conversions with a destination succeeded, %n aside. it returns -1
 * when the input ends at its first byte, or when the FILE is unusable. it
 * sets errno to PTRERR for a null FILE, NOTOPN for one not open, and EBADF
 * for one open only for writing. it departs from ISO C as follows.
 *
 * a byte 0xff ends the input, as the string's null does. input that ends
 * before the first conversion, but not at its first byte, returns 0 where ISO
 * returns EOF.
 *
 * only ' ', '\t' and '\n' in the format skip input whitespace, and only they
 * end a %s without a width. %Ns copies up to N bytes, whitespace included.
 *
 * a number or %s that fails still stores through its destination, 0 or "",
 * and isn't counted. an integer with L sets errno to ECSPEC and stores
 * nothing. an integer may end in u, l, ul or lu. a float may end in f or l.
 * each suffix may be in either case. a float's f or l is read even where no
 * digits came before it. an integer wraps within its length's unsigned range:
 * "3000000000" with %d stores -1294967296. past that range it saturates and
 * sets ERANGE.
 *
 * %[ ends its set at the first ']' and has no ranges. a %[ that matches
 * nothing stores "" and counts. %n stores an int, even for %hn. its count is
 * the bytes fetched, the held lookahead included, so "12x" with "%d%n" stores
 * 3. the scan stops as soon as the input ends, so "12" with "%d%n" stores
 * nothing through %n.
 *
 * a width past SCAN_WIDTH_MAX sets errno to ECSPEC and ends the scan. so do
 * an unknown conversion and a format byte the input doesn't match, %%
 * included. a directive with too wide a width still stores, but isn't
 * counted. a directive the format's null cuts short, such as a trailing %,
 * ends as an unknown conversion does. unless it begins %*, it first takes a
 * variadic argument. the scan then reads the byte past the null */
int _fmtin(int string_mode, int (*read)(FILE *), const void *input,
           const char *format, va_list arguments);

/* the significant digits _fmtin keeps of a float */
#define SCAN_DIGITS 17

/* which errno a result out of range sets: EFLOATU or EFLOATO, EDBLU or
 * EDBLO, ELDBLU or ELDBLO, or ERANGE for either. _fmtin picks one of the
 * first three by a float's suffix: f, none or l */
enum {
    RANGE_ERRNO_FLOAT = 1,
    RANGE_ERRNO_DOUBLE,
    RANGE_ERRNO_LONG_DOUBLE,
    RANGE_ERRNO_RANGE
};

/* sign times digits[0].digits[1]...digits[16], times 10 to exponent, as
 * _calcfpow converts it. digits are SCAN_DIGITS decimal digits, not all '0' */
float _stf(int sign, const char *digits, int exponent, char range_errno);

/* set integer to integer * 10 + digit. integer is 8 bytes, most significant
 * first, and below 10 to the count. count is at most 18: the multiply reaches
 * only the low bytes that count + 1 digits need */
void _calcint(unsigned char *integer, int digit, int count);

/* integer times 10 to the n, as a float. integer is 8 bytes, most
 * significant first, and isn't 0. _calcfpow shifts integer's bytes up to its
 * top set bit, in place. the float's fraction is the top 24 bits of the
 * product, truncated. a result below a float's normal range becomes 0, and a
 * result above it becomes infinity. either sets errno to the code range_errno
 * picks. a range_errno outside RANGE_ERRNO_FLOAT to RANGE_ERRNO_RANGE leaves
 * errno as it was */
float _calcfpow(unsigned char *integer, int n, char range_errno);

/* a float with sign 0, the biased exponent exponent, and the fraction in
 * fraction's 3 bytes, most significant first. exponent runs from 0 to 255.
 * fraction's top bit stands for the bit a float leaves out, and _packf
 * clears it in place */
float _packf(short exponent, unsigned char *fraction);

#endif
