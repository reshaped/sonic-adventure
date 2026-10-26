/* the C runtime's multiple-precision helpers, which its number conversions
 * share. a number is n bytes, most significant first. a fraction is such a
 * number whose top bit is set, and its exponent is the power of 2 that bit
 * stands for */
#ifndef MULTIPREC_H
#define MULTIPREC_H

/* shift bytes left by bits, from 0 to 7. it returns 1 if a set bit leaves the
 * top, else 0. a shift of 8 changes nothing and returns no value */
int _lsfts(unsigned char *bytes, int n, short bits);
/* shift bytes left by 1, and return the bit shifted out */
int _lsft(unsigned char *bytes, int n);
/* set the sticky bit, the bit numbered bit counting from 0 at the top, if any
 * bit below it is set */
void _setsbit(unsigned char *bytes, int n, int bit);
/* add from into to. a carry out of the first byte is lost */
void _add(unsigned char *to, unsigned char *from, int n);
/* store 10 to the n as an 8-byte fraction and its exponent */
void _power(int n, short *exponent, unsigned char *fraction);
/* multiply a by b, each an 8-byte fraction. a gets the product as 9 bytes, and
 * *a_exponent its exponent. the ninth byte holds three bits for _rnd: the two
 * after the product's first 64, and a sticky bit for the rest */
void _mult64(short *a_exponent, unsigned char *a, short b_exponent,
             const unsigned char *b);
/* divide dividend by divisor, each an 8-byte fraction. the quotient replaces
 * divisor as 9 bytes, the last for rounding, with its top bit set unless it is
 * 0. its exponent replaces *divisor_exponent */
void _div64(short *divisor_exponent, unsigned char *divisor,
            short dividend_exponent, unsigned char *dividend);
/* store *word in the four bytes at bytes, most significant first. bytes must
 * not overlap *word */
void _mvito4b(unsigned char *bytes, const unsigned long *word);

#endif
