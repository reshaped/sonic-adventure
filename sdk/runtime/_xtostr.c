/* float conversion text for the formatter */
#include <stdarg.h>
#include <string.h>

#include "runtime/print.h"

extern int _xti(long double value, int digits, unsigned char *integer,
                int *scale, signed char *sign);
extern void _its(unsigned char *value, char *digits, int n);

static void round_digits(char *digits, int position);

void _xtostr(va_list *arguments, char *text, int conversion,
             struct format_parts *parts)
{
    long double value;
    int scale;
    signed char sign;
    unsigned char magnitude[8];
    char digits[32];
    char *begin;
    char *lead;
    char *low;
    char *high;
    char *sign_end;
    char *end;
    char swap;
    char sign_flags;
    int status;
    int length;
    int count;
    int position;
    int exponent;
    int source;
    int skip;
    int i;
    int j;

#ifdef GCC
    /* a float argument arrives as a double, which is float under
     * -m4-single-only. GCC compiles va_arg of float to a trap */
    value = va_arg(*arguments, double);
#else
    value = va_arg(*arguments, float);
#endif
    /* @hack redundant: a square of value that never runs; without it, the
     * argument fetch's cursor and mask take R5 and R7 for R7 and R6, its frame
     * words and conversion's frame word move, the reversal keeps length / 2 in
     * R7 and spills swap, and 52 of 875 words differ */
    if (0) {
        value = value * value;
    }
    if (parts->precision == -1) {
        parts->precision = 6;
    }
    begin = text;
    digits[0] = '0';
    if (value != 0.0) {
        status = _xti(value, 17, magnitude, &scale, &sign);
        if (status != 0) {
            *text = '\0';
            if (parts->precision > parts->width) {
                parts->padding = parts->precision;
            } else {
                parts->padding = parts->width;
            }
            switch (status) {
            case 1:
                parts->fill = '+';
                break;
            case -1:
                parts->fill = '-';
                break;
            default:
                parts->fill = '*';
                break;
            }
            return;
        }
        _its(magnitude, digits + 1, 17);
    } else {
        sign = 1;
        digits[1] = '\0';
    }
    /* _its writes the digits least significant first, so these are the low
     * zeros */
    for (i = 1; digits[i] == '0'; i++) {
    }
    if (i > 1) {
        length = strlen(digits + i);
        scale += i - 1;
        for (j = 1; j <= length; j++, i++) {
            digits[j] = digits[i];
        }
        digits[j] = '\0';
    }
    /* @hack spelling: lead assigned inside strlen's argument, which SHC builds
     * in R4 (MOV R12,R4 and ADD #1,R4 at 0x8c126680 and 0x8c126686); in a
     * statement of its own, lead is built in R3 and copied to R4, the
     * argument fetch's cursor and mask trade R7 and R6 (0x8c126562,
     * 0x8c126564), and 92 of 875 words differ */
    length = strlen(lead = digits + 1);
    for (i = 0, high = lead + length - 1, low = lead; i < length / 2; i++) {
        swap = *low;
        *low = *high;
        *high = swap;
        high--;
        low++;
    }
    if (length == 0) {
        *lead = '0';
        length = 1;
        scale = 0;
    }
    if (conversion == 'g' || conversion == 'G') {
        parts->flags |= PRINT_G;
        exponent = scale + length - 1;
        if (exponent < -4 || exponent > parts->precision) {
            conversion -= 2;
        } else {
            conversion = 'f';
        }
    }
    if (sign == 1) {
        sign_flags = parts->flags & (PRINT_PLUS | PRINT_SPACE);
        switch (sign_flags) {
        case PRINT_PLUS:
            *text++ = '+';
            break;
        case PRINT_SPACE:
            *text++ = ' ';
            break;
        }
    } else {
        *text++ = '-';
    }
    /* count measures the text against the width, leaving out a sign or
     * space, which ends at sign_end */
    sign_end = begin + 1;
    if (conversion == 'f') {
        if (scale >= 0) {
            for (i = 0, low = lead; i < length; i++) {
                *text++ = *low++;
            }
            parts->digits = text;
            parts->digit_zeros = scale;
            count = 0;
            if ((!(parts->flags & PRINT_G) && parts->precision > 0) ||
                (parts->flags & PRINT_ALTERNATE)) {
                *text++ = '.';
                count++;
            }
            /* @hack redundant: PRINT_G tested again where the || has ruled it
             * out (MOV #32,R1 and TST R5,R1 at 0x8c12679c and 0x8c12679e);
             * tested once, the function is 4 bytes shorter and 593 of 875
             * words differ */
            if (!(parts->flags & PRINT_G) ||
                ((parts->flags & PRINT_G) != 0 &&
                 (parts->flags & PRINT_ALTERNATE))) {
                parts->suffix = text;
                parts->suffix_zeros = parts->precision;
                count += parts->precision;
            }
            count += scale;
            count += length;
            if ((*begin == '+' || *begin == '-' || *begin == ' ') &&
                parts->width > count && parts->fill == '0') {
                parts->prefix_end = sign_end;
                parts->prefix_zeros = parts->width - count;
                parts->padding = 0;
            } else if (parts->width > count) {
                parts->padding = parts->width - count;
            } else {
                parts->padding = 0;
            }
        } else {
            scale += length;
            position = scale + parts->precision;
            if (position < length && position >= 0) {
                round_digits(digits, position + 1);
            }
            if (scale > 0) {
                if (digits[0] == '0') {
                    source = 1;
                    skip = 1;
                } else {
                    length++;
                    source = 0;
                    skip = 0;
                    scale++;
                }
                count = 0;
                while (scale > 0) {
                    *text++ = digits[source];
                    source++;
                    scale--;
                    count++;
                }
                *text++ = '.';
                count++;
                while (source - skip < length && parts->precision > 0) {
                    *text++ = digits[source++];
                    parts->precision--;
                    count++;
                }
                /* @hack redundant: PRINT_G tested again where the || has
                 * ruled it out (MOV #32,R1 and TST R5,R1 at 0x8c126892 and
                 * 0x8c126894); tested once, the function is 4 bytes shorter
                 * and 476 of 875 words differ */
                /* @hack redundant: text[-1] tested ahead of the loop, which
                 * repeats the loop's own test (CMP/EQ #48,R0 at 0x8c1268b0);
                 * without it, the function is 16 bytes shorter and 464 of 875
                 * words differ */
                if (!(parts->flags & PRINT_G) ||
                    ((parts->flags & PRINT_G) != 0 &&
                     (parts->flags & PRINT_ALTERNATE))) {
                    count += parts->precision;
                    parts->digits = text;
                    parts->digit_zeros = parts->precision;
                } else if (text[-1] == '0') {
                    while (text[-1] == '0') {
                        text--;
                        count--;
                    }
                }
                if (!(parts->flags & PRINT_ALTERNATE) && text[-1] == '.' &&
                    (parts->digit_zeros == 0 || (parts->flags & PRINT_G))) {
                    count = count - parts->digit_zeros - 1;
                    text--;
                    parts->digit_zeros = 0;
                    parts->digits = begin;
                }
                if ((*begin == '+' || *begin == '-' || *begin == ' ') &&
                    parts->width > count && parts->fill == '0') {
                    parts->prefix_end = sign_end;
                    parts->prefix_zeros = parts->width - count;
                    parts->padding = 0;
                } else if (parts->width > count) {
                    parts->padding = parts->width - count;
                } else {
                    parts->padding = 0;
                }
                if (parts->digits == begin) {
                    parts->digits = parts->prefix_end;
                }
            } else {
                if (digits[0] == '0') {
                    source = 1;
                    skip = 1;
                } else {
                    source = 0;
                    skip = 0;
                    scale++;
                    length++;
                }
                if (scale > 0) {
                    source = 1;
                    *text++ = digits[0];
                } else {
                    *text++ = '0';
                }
                count = 1;
                *text++ = '.';
                count++;
                if (scale < 0) {
                    parts->digits = text;
                    if (parts->precision > -scale) {
                        parts->digit_zeros = -scale;
                        count -= scale;
                    } else {
                        parts->digit_zeros = parts->precision;
                        count += parts->precision;
                    }
                    parts->precision += scale;
                }
                while (source - skip < length && parts->precision > 0) {
                    *text++ = digits[source];
                    count++;
                    source++;
                    parts->precision--;
                }
                /* @hack redundant: PRINT_G tested again where the || has
                 * ruled it out (MOV #32,R1 and TST R5,R1 at 0x8c1269d8 and
                 * 0x8c1269da); tested once, the function is 8 bytes shorter
                 * and 314 of 875 words differ */
                /* @hack redundant: text[-1] tested ahead of the loop, which
                 * repeats the loop's own test (CMP/EQ #48,R0 at 0x8c1269fc);
                 * without it, the function is 16 bytes shorter and 301 of 875
                 * words differ */
                if (!(parts->flags & PRINT_G) ||
                    ((parts->flags & PRINT_G) != 0 &&
                     (parts->flags & PRINT_ALTERNATE))) {
                    if (parts->precision > 0) {
                        parts->suffix = text;
                        parts->suffix_zeros = parts->precision;
                        count += parts->precision;
                    }
                } else if (text[-1] == '0') {
                    while (text[-1] == '0') {
                        text--;
                        count--;
                    }
                }
                if (!(parts->flags & PRINT_ALTERNATE) && text[-1] == '.' &&
                    ((parts->digit_zeros == 0 && parts->suffix_zeros == 0) ||
                     (parts->flags & PRINT_G))) {
                    count =
                        count - parts->digit_zeros - parts->suffix_zeros - 1;
                    parts->suffix_zeros = 0;
                    parts->digit_zeros = 0;
                    text--;
                    parts->digits = begin;
                    parts->suffix = NULL;
                }
                if ((*begin == '+' || *begin == '-' || *begin == ' ') &&
                    parts->width > count && parts->fill == '0') {
                    parts->prefix_end = sign_end;
                    parts->prefix_zeros = parts->width - count;
                    parts->padding = 0;
                } else if (parts->width > count) {
                    parts->padding = parts->width - count;
                } else {
                    parts->padding = 0;
                }
                if (parts->digits == begin) {
                    parts->digits = parts->prefix_end;
                }
            }
        }
    } else {
        if (parts->precision + 1 < length) {
            round_digits(digits, parts->precision + 2);
        }
        if (digits[0] == '0') {
            skip = 1;
        } else {
            skip = 0;
            scale--;
            length++;
        }
        scale += length - skip;
        *text++ = digits[skip];
        source = skip;
        count = 1;
        /* @hack redundant: a copy of skip to count that never runs; without it,
         * skip takes R4 for R6, conversion's frame word trades with the
         * argument fetch's second frame word, the reversal keeps length / 2 in
         * R7, spills swap and gives low R4 and high R5, and 47 of 875 words
         * differ */
        if (0) {
            count = skip;
        }
        source++;
        count++;
        *text++ = '.';
        while (source <= length && parts->precision > 0) {
            *text++ = digits[source];
            count++;
            source++;
            parts->precision--;
        }
        /* @hack redundant: PRINT_G tested again where the || has ruled it out
         * (MOV #32,R1 and TST R5,R1 at 0x8c126b10 and 0x8c126b12); tested
         * once, the function is 8 bytes shorter and 162 of 875 words differ */
        /* @hack redundant: text[-1] tested ahead of the loop, which repeats the
         * loop's own test (CMP/EQ #48,R0 at 0x8c126b2e); without it, the
         * function is 16 bytes shorter and 152 of 875 words differ */
        if (!(parts->flags & PRINT_G) || ((parts->flags & PRINT_G) != 0 &&
                                          (parts->flags & PRINT_ALTERNATE))) {
            parts->digits = text;
            parts->digit_zeros = parts->precision;
            count += parts->precision;
        } else if (text[-1] == '0') {
            while (text[-1] == '0') {
                text--;
                count--;
            }
        }
        if (!(parts->flags & PRINT_ALTERNATE) && text[-1] == '.' &&
            (parts->digit_zeros == 0 || (parts->flags & PRINT_G))) {
            count = count - parts->digit_zeros - 1;
            text--;
            parts->digits = begin;
            parts->digit_zeros = 0;
        }
        if (conversion == 'e') {
            *text++ = 'e';
        } else {
            *text++ = 'E';
        }
        count++;
        if (scale >= 0) {
            *text++ = '+';
        } else {
            *text++ = '-';
            scale = -scale;
        }
        count++;
        if (scale >= 100) {
            text += 2;
            count += 3;
        } else {
            *text++ = '0';
            *text = '0';
            count += 2;
        }
        /* text is at the exponent's last place */
        end = text + 1;
        while (scale > 0) {
            *text-- = (scale % 10) + '0';
            scale = scale / 10;
        }
        text = end;
        if ((*begin == '+' || *begin == '-' || *begin == ' ') &&
            parts->width > count && parts->fill == '0') {
            parts->prefix_end = sign_end;
            parts->prefix_zeros = parts->width - count;
            parts->padding = 0;
        } else if (parts->width > count) {
            parts->padding = parts->width - count;
        } else {
            parts->padding = 0;
        }
        if (parts->digits == begin) {
            parts->digits = parts->prefix_end;
        }
    }
    *text = '\0';
}

/* round half up at digits[position], the first digit dropped, carrying as far
 * as digits[0] */
static void round_digits(char *digits, int position)
{
    if (digits[position] > '4') {
        digits[position] = '0';
        position--;
        digits[position]++;
        for (; position > 0; position--) {
            if (digits[position] > '9') {
                digits[position] -= 10;
                /* @hack spelling: the carry stored through digits + position -
                 * 1, which SHC forms from the digit's own address (MOV R6,R3
                 * and ADD #-1,R3 at 0x8c126c7a and 0x8c126c7c); as
                 * digits[position - 1], the function is 4 bytes longer and 39
                 * of 50 words differ */
                *(digits + position - 1) += 1;
            }
        }
        /* @hack redundant: a pass at digits[1] after the loop, whose test
         * never holds, since digits[1] is a digit by then (CMP/GT R0,R2 at
         * 0x8c126c96); without it, the function is 24 bytes shorter and 42
         * of 50 words differ */
        if (digits[1] > '9') {
            digits[1] -= 10;
            digits[0]++;
        }
    }
}
