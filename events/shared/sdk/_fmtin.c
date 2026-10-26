#include <ctype.h>
#include <errno.h>
#include <stdarg.h>
#include <stdio.h>

#include "shared/sdk/scan.h"

/* the digits an exponent can have after its leading zeros */
#define EXPONENT_DIGITS 3

/* input_state flags */
enum {
    SCAN_UNLIMITED = 1, /* the directive gives no width */
    SCAN_OVERFLOW = 2, /* the integer passed its length's unsigned range */
    SCAN_STREAM = 4, /* the input is a FILE, not a string */
    SCAN_STOP = 8, /* the scan ends after this directive */
    SCAN_STRING_END = 16 /* a %s without a width has copied its bytes */
};

/* text, the string's next byte, or stream, the FILE */
union input_cursor {
    const char *text;
    FILE *stream;
};

struct input_state {
    /* the lookahead, the next byte a directive takes. signed_byte is -1 at
     * the end of the input, and code indexes _ctype */
    union {
        char signed_byte;
        unsigned char code;
    } current;
    char length; /* the length modifier, 'h', 'l' or 'L', or ' ' */
    /* the letter of the last integer conversion, which picks the value an
     * overflow saturates to. %p doesn't set it */
    char conversion;
    char flags;
    union input_cursor cursor;
    /* the bytes fetched, the lookahead included but not the byte that ended
     * the input. it wraps at 32 bits */
    unsigned int fetched_count;
    /* the bytes the directive may still read. advance_input counts it down on
     * every byte. a directive without a width tests it only for %c, which sets
     * it to 1 first, so elsewhere nothing stops it at 0. the step is unsigned,
     * which keeps it defined at INT_MIN */
    int width;
    int (*read)(FILE *);
};

static int read_integer(int radix, void *destination,
                        struct input_state *state);
static int read_decimal(char conversion, void *destination,
                        struct input_state *state);
static int read_set(const char **format, char *destination,
                    struct input_state *state);
static void advance_input(struct input_state *state);
static void restore_input(struct input_state *state);

int _fmtin(int string_mode, int (*read)(FILE *), const void *input,
           const char *format_input, va_list arguments)
{
    struct input_state storage;
    struct input_state *state = &storage;
    int saved_width;
    const char *format;
    int radix;
    int assignments;
    char *destination;

    saved_width = 0;
    /* @hack redundant: format set to 0, which the image stores to format's
     * word before format_input replaces it (MOV.L R12,@R15 at 0x0cb84fe6 in
     * EV001E); without it, 100 of 568 words differ */
    format = 0;
    state->current.signed_byte = 0;
    state->flags = 0;
    state->length = 0;
    state->conversion = 0;
    state->cursor.text = 0;
    state->fetched_count = 0;
    if (!string_mode) {
        if (!input) {
            errno = PTRERR;
            return -1;
        }
        if (!(((const FILE *)input)->_ioflag1 & (_IOREAD | _IOWRITE | _IORW))) {
            errno = NOTOPN;
            return -1;
        }
        if (!(((const FILE *)input)->_ioflag1 & _IORW) &&
            (((const FILE *)input)->_ioflag1 & _IOWRITE)) {
            errno = EBADF;
            return -1;
        }
        state->read = read;
    }
    state->current.signed_byte = 0;
    /* in a FILE's scan, this sets cursor.stream, which shares the word */
    state->cursor.text = input;
    format = format_input;
    state->flags = 0;
    state->fetched_count = 0;
    assignments = 0;
    if (!string_mode) {
        state->flags |= SCAN_STREAM;
    }
    advance_input(state);
    if (state->current.signed_byte == -1) {
        assignments = -1;
    }
    /* @bug memory: a directive the format's null cuts short leaves
     * format at the null, which the ++format after the directive steps past.
     * this test then reads the byte after the null (MOV.B @R3,R2 at 0x0cb8541e
     * in EV001E) before it tests SCAN_STOP (TST #8,R0 at 0x0cb8542c) */
    while (*format && state->current.signed_byte != -1 &&
           !(state->flags & SCAN_STOP)) {
        if (*format == ' ' || *format == '\t' || *format == '\n') {
            while (*format == ' ' || *format == '\t' || *format == '\n') {
                ++format;
            }
            while (_ctype[state->current.code] & _WHITE) {
                advance_input(state);
            }
        } else if (*format != '%') {
            if (*format != state->current.signed_byte) {
                state->flags |= SCAN_STOP;
                errno = ECSPEC;
            } else {
                ++format;
                advance_input(state);
            }
        } else {
            ++format;
            /* the byte after the % alone decides whether the directive takes
             * an argument. * and % take none. every other byte takes an
             * argument, whatever conversion follows */
            switch (*format) {
            case '*':
                ++format;
            case '%':
                destination = 0;
                break;
            default:
                /* every pointer argument takes one four-byte slot, so the
                 * argument is fetched as char *. each conversion stores
                 * through it as its own type */
                destination = va_arg(arguments, char *);
                break;
            }
            state->width = 0;
            if (_ctype[(unsigned char)*format] & _DIGIT) {
                state->flags &= ~SCAN_UNLIMITED;
                while (_ctype[(unsigned char)*format] & _DIGIT) {
                    int digit = *format - '0';
                    if ((SCAN_WIDTH_MAX - digit) / 10 >= state->width) {
                        state->width = (state->width * 10) + digit;
                    } else {
                        state->flags |= SCAN_STOP;
                        errno = ECSPEC;
                    }
                    ++format;
                }
                saved_width = state->width;
            } else {
                state->flags |= SCAN_UNLIMITED;
            }
            state->length = *format;
            if (state->length == 'h' || state->length == 'l' ||
                state->length == 'L') {
                ++format;
            } else {
                state->length = ' ';
            }
            if (*format != 'c' && *format != '[' && *format != 'n') {
                while (_ctype[state->current.code] & _WHITE) {
                    advance_input(state);
                }
            }
            state->width = saved_width;
            switch (*format) {
            case 'd':
            case 'u':
            case 'i':
            case 'o':
            case 'x':
            case 'X':
                state->conversion = *format;
                if (*format == 'd' || *format == 'u') {
                    radix = 10;
                }
                if (*format == 'i') {
                    radix = 0;
                }
                if (*format == 'o') {
                    radix = 8;
                }
                if (*format == 'x' || *format == 'X') {
                    radix = 16;
                }
                if (read_integer(radix, destination, state)) {
                    state->flags |= SCAN_STOP;
                }
                break;
            case 's':
                state->flags &= ~SCAN_STRING_END;
                if (state->current.signed_byte == -1) {
                    state->flags |= SCAN_STOP;
                }
                while (((state->flags & SCAN_UNLIMITED) || state->width > 0) &&
                       state->current.signed_byte != -1 &&
                       !(state->flags & SCAN_STRING_END)) {
                    if (!(state->flags & SCAN_UNLIMITED)) {
                        while (state->width > 0 &&
                               state->current.signed_byte != -1) {
                            if (destination) {
                                *destination++ = state->current.signed_byte;
                            }
                            advance_input(state);
                        }
                    } else {
                        while (state->current.signed_byte != -1 &&
                               state->current.signed_byte != ' ' &&
                               state->current.signed_byte != '\n' &&
                               state->current.signed_byte != '\t') {
                            if (destination) {
                                *destination++ = state->current.signed_byte;
                            }
                            advance_input(state);
                        }
                        state->flags |= SCAN_STRING_END;
                    }
                }
                if (destination) {
                    *destination = 0;
                }
                break;
            case 'c':
                if (state->flags & SCAN_UNLIMITED) {
                    state->width = 1;
                }
                if (state->current.signed_byte == -1) {
                    state->flags |= SCAN_STOP;
                }
                while (state->width > 0 && state->current.signed_byte != -1) {
                    if (destination) {
                        *destination++ = state->current.signed_byte;
                    }
                    advance_input(state);
                }
                break;
            case 'e':
            case 'E':
            case 'f':
            case 'g':
            case 'G':
                if (read_decimal(*format, destination, state)) {
                    state->flags |= SCAN_STOP;
                }
                break;
            case 'p':
                if (read_integer(16, destination, state)) {
                    state->flags |= SCAN_STOP;
                }
                break;
            case 'n':
                if (destination) {
                    *(int *)destination = state->fetched_count;
                }
                break;
            case '[':
                if (read_set(&format, destination, state)) {
                    state->flags |= SCAN_STOP;
                }
                break;
            case '%':
                if (state->current.signed_byte != '%') {
                    state->flags |= SCAN_STOP;
                    errno = ECSPEC;
                } else {
                    advance_input(state);
                }
                break;
            default:
                errno = ECSPEC;
                state->flags |= SCAN_STOP;
                break;
            }
            if (!(state->flags & SCAN_STOP) && *format != 'n' && destination) {
                ++assignments;
            }
            ++format;
        }
    }
    if (state->current.signed_byte != -1) {
        restore_input(state);
    }
    return assignments;
}

/* read an integer in radix, or with radix 0 in the radix its prefix gives,
 * and store it through destination as the length's integer. returns 0 if it
 * read a digit, not counting a 0x prefix's 0, and 1 if not. an L returns -1
 * and sets errno to ECSPEC */
static int read_integer(int radix, void *destination, struct input_state *state)
{
    unsigned long result;
    unsigned long accumulator;
    unsigned char digit;
    char last_digit;
    int sign = 1;
    int matched;

    matched = result = 0;
    if (state->length == 'L') {
        errno = ECSPEC;
        return -1;
    }
    if (((state->flags & SCAN_UNLIMITED) || state->width > 0) &&
        (state->current.signed_byte == '+' ||
         state->current.signed_byte == '-')) {
        if (state->current.signed_byte == '-') {
            sign = -1;
        }
        advance_input(state);
    }
    if (!radix) {
        if (((state->flags & SCAN_UNLIMITED) || state->width > 0) &&
            state->current.signed_byte == '0') {
            advance_input(state);
            if (((state->flags & SCAN_UNLIMITED) || state->width > 0) &&
                ((_ctype[state->current.code] & _UPPER)
                     ? state->current.signed_byte + 32
                     : state->current.signed_byte) == 'x') {
                advance_input(state);
                radix = 16;
            } else {
                radix = 8;
                matched = 1;
            }
        } else {
            radix = 10;
        }
    } else if (radix == 16) {
        if (((state->flags & SCAN_UNLIMITED) || state->width > 0) &&
            state->current.signed_byte == '0') {
            advance_input(state);
            if (((state->flags & SCAN_UNLIMITED) || state->width > 0) &&
                ((_ctype[state->current.code] & _UPPER)
                     ? state->current.signed_byte + 32
                     : state->current.signed_byte) == 'x') {
                advance_input(state);
            } else {
                matched = 1;
            }
        }
    }
    last_digit = radix > 10 ? 'a' - 11 + radix : '0' - 1 + radix;
    if (((state->flags & SCAN_UNLIMITED) || state->width > 0) &&
        (_ctype[state->current.code] & (_UPPER | _LOWER | _DIGIT)) &&
        ((_ctype[state->current.code] & _UPPER)
             ? state->current.signed_byte + 32
             : state->current.signed_byte) <= last_digit) {
        matched = 1;
        while (((state->flags & SCAN_UNLIMITED) || state->width > 0) &&
               state->current.signed_byte == '0') {
            advance_input(state);
        }
        state->flags &= ~SCAN_OVERFLOW;
        accumulator = 0;
        while (((state->flags & SCAN_UNLIMITED) || state->width > 0) &&
               (_ctype[state->current.code] & (_UPPER | _LOWER | _DIGIT)) &&
               ((_ctype[state->current.code] & _UPPER)
                    ? state->current.signed_byte + 32
                    : state->current.signed_byte) <= last_digit) {
            if (!(state->flags & SCAN_OVERFLOW)) {
                if (_ctype[state->current.code] & _DIGIT) {
                    digit = state->current.signed_byte - '0';
                } else {
                    /* @hack type: the signed byte masked to index _ctype in
                     * place of code, so that one load gives both the index
                     * and the value (MOV.B @R14,R4 at 0x0cb855fa in
                     * EV001E); through code, 320 of 510 words differ */
                    digit =
                        ((_ctype[state->current.signed_byte & 0xff] & _UPPER)
                             ? state->current.signed_byte + 32
                             : state->current.signed_byte) -
                        'a' + 10;
                }
                /* int and long are both 32 bits, so ' ', no length, tests as
                 * 'l' does */
                switch (state->length) {
                case 'h':
                    if (accumulator > (65535UL - digit) / radix) {
                        state->flags |= SCAN_OVERFLOW;
                        switch (state->conversion) {
                        case 'd':
                        case 'i':
                            if (sign == 1) {
                                result = 32767;
                            } else {
                                result = (unsigned long)-32768;
                            }
                            break;
                        case 'o':
                        case 'u':
                        case 'x':
                        case 'X':
                            result = 65535;
                        }
                    } else {
                        accumulator = (accumulator * radix) + digit;
                    }
                    break;
                case 'l':
                    if (accumulator > (0xffffffffUL - digit) / radix) {
                        state->flags |= SCAN_OVERFLOW;
                        switch (state->conversion) {
                        case 'd':
                        case 'i':
                            if (sign == 1) {
                                result = 0x7fffffffUL;
                            } else {
                                result = 0x80000000UL;
                            }
                            break;
                        case 'o':
                        case 'u':
                        case 'x':
                        case 'X':
                            result = 0xffffffffUL;
                        }
                    } else {
                        accumulator = (accumulator * radix) + digit;
                    }
                    break;
                case ' ':
                    if (accumulator > (0xffffffffU - digit) / radix) {
                        state->flags |= SCAN_OVERFLOW;
                        switch (state->conversion) {
                        case 'd':
                        case 'i':
                            if (sign == 1) {
                                result = 0x7fffffffUL;
                            } else {
                                result = 0x80000000UL;
                            }
                            break;
                        case 'o':
                        case 'u':
                        case 'x':
                        case 'X':
                            result = 0xffffffffU;
                        }
                    } else {
                        accumulator = (accumulator * radix) + digit;
                    }
                    break;
                }
            }
            advance_input(state);
        }
        if (!(state->flags & SCAN_OVERFLOW)) {
            if (sign == 1) {
                result = accumulator;
            } else {
                result = -accumulator;
            }
        } else {
            errno = ERANGE;
        }
    }
    if (matched == 1) {
        if (((state->flags & SCAN_UNLIMITED) || state->width > 0) &&
            ((_ctype[state->current.code] & _UPPER)
                 ? state->current.signed_byte + 32
                 : state->current.signed_byte) == 'u') {
            advance_input(state);
            if (((state->flags & SCAN_UNLIMITED) || state->width > 0) &&
                ((_ctype[state->current.code] & _UPPER)
                     ? state->current.signed_byte + 32
                     : state->current.signed_byte) == 'l') {
                advance_input(state);
            }
        } else if (((state->flags & SCAN_UNLIMITED) || state->width > 0) &&
                   ((_ctype[state->current.code] & _UPPER)
                        ? state->current.signed_byte + 32
                        : state->current.signed_byte) == 'l') {
            advance_input(state);
            if (((state->flags & SCAN_UNLIMITED) || state->width > 0) &&
                ((_ctype[state->current.code] & _UPPER)
                     ? state->current.signed_byte + 32
                     : state->current.signed_byte) == 'u') {
                advance_input(state);
            }
        }
    }
    if (destination) {
        switch (state->length) {
        case 'h':
            *(short *)destination = result;
            break;
        case 'l':
            *(long *)destination = result;
            break;
        default:
            *(int *)destination = result;
        }
    }
    if (matched) {
        return 0;
    }
    return 1;
}

/* a result's bytes: 8, a double's size. under -fpu=single the destination
 * takes a float, from bytes 0 to 3, which value aligns for that read */
union decimal_value {
    float value;
    unsigned char bytes[8];
};

/* read a float and store it through destination as the length's float.
 * returns 0 if it read a digit, and 1 if not. conversion goes unread: %e, %f
 * and %g read alike */
static int read_decimal(char conversion, void *destination,
                        struct input_state *state)
{
    char digits[SCAN_DIGITS];
    char exponent_digits[EXPONENT_DIGITS];
    union decimal_value zero;
    /* @hack type: suffix.range_errno stored over the low byte of words[1],
     * byte 36 of the frame (MOV.B R13,@(R0,R15) at 0x0cb85bf2 in EV001E).
     * words holds a double's +infinity, which nothing reads as one. as a char
     * of its own, 21 of 541 words differ */
    union {
        unsigned long words[2];
        struct {
            unsigned long pad;
            char range_errno;
        } suffix;
    } wide_limit;
    struct decimal_limit {
        /* a float's +infinity */
        union {
            unsigned long encoding;
            float value;
        } narrow;
        int range; /* 1 for an overflow, -1 for an underflow, else 0 */
    } limit;
    float converted;
    char *result;
    int digit_count;
    /* the power of 10 that the significant digits take, read as 0.digits. it
     * is their count before the point, or where none stands there, minus the
     * zeros between the point and the first. it is unsigned, so a power below
     * 0 wraps, and its sum with exponent wraps back */
    unsigned int before_point;
    int exponent_digit_count;
    int sign;
    int exponent_sign;
    int matched;
    int exponent;
    int index;
    unsigned char *clear;
    unsigned long *word;

    /* @hack spelling: the count tested by a continue and a break. SHC then
     * counts index up and compares it after each store (CMP/GE R6,R5 at
     * 0x0cb858b8 in EV001E). as a bounded for loop, SHC counts down from 8 and
     * tests first, and 23 of 541 words differ */
    for (index = 0, clear = zero.bytes;;) {
        *clear = 0;
        clear = clear + 1;
        ++index;
        if (index < 8) {
            continue;
        }
        break;
    }
    word = wide_limit.words;
    word[0] = 0;
    word[1] = 2047UL << 20;
    /* @hack spelling: the float infinity stored through word, which held the
     * double's address before, so that both stores go through R4 (MOV R15,R4
     * and MOV.L R3,@R4 at 0x0cb858cc and 0x0cb858d2 in EV001E). stored to
     * limit.narrow.encoding by member, 505 of read_decimal's 541 words differ
     * and the section is 4 bytes shorter */
    word = &limit.narrow.encoding;
    *word = 255UL << 23;
    digit_count = 0;
    exponent_digit_count = 0;
    sign = 1;
    exponent_sign = 1;
    before_point = 0;
    limit.range = 0;
    matched = 0;
    if (((state->flags & SCAN_UNLIMITED) || state->width > 0) &&
        (state->current.signed_byte == '+' ||
         state->current.signed_byte == '-')) {
        if (state->current.signed_byte == '-') {
            sign = -1;
        }
        advance_input(state);
    }
    while (((state->flags & SCAN_UNLIMITED) || state->width > 0) &&
           state->current.signed_byte == '0') {
        matched = 1;
        advance_input(state);
    }
    while (((state->flags & SCAN_UNLIMITED) || state->width > 0) &&
           (_ctype[state->current.code] & _DIGIT)) {
        matched = 1;
        if (digit_count < SCAN_DIGITS) {
            digits[digit_count++] = state->current.signed_byte;
        } else {
            errno = ETLN;
        }
        advance_input(state);
        ++before_point;
    }
    if (((state->flags & SCAN_UNLIMITED) || state->width > 0) &&
        state->current.signed_byte == '.') {
        advance_input(state);
        if (!digit_count) {
            while (((state->flags & SCAN_UNLIMITED) || state->width > 0) &&
                   state->current.signed_byte == '0') {
                matched = 1;
                advance_input(state);
                --before_point;
            }
        }
        while (((state->flags & SCAN_UNLIMITED) || state->width > 0) &&
               (_ctype[state->current.code] & _DIGIT)) {
            matched = 1;
            if (digit_count < SCAN_DIGITS) {
                digits[digit_count++] = state->current.signed_byte;
            } else {
                errno = ETLN;
            }
            advance_input(state);
        }
    }
    if (!digit_count) {
        result = (char *)zero.bytes;
        if (matched == 1) {
            if (((state->flags & SCAN_UNLIMITED) || state->width > 0) &&
                ((_ctype[state->current.code] & _UPPER)
                     ? state->current.signed_byte + 32
                     : state->current.signed_byte) == 'e') {
                advance_input(state);
                if (((state->flags & SCAN_UNLIMITED) || state->width > 0) &&
                    (state->current.signed_byte == '+' ||
                     state->current.signed_byte == '-')) {
                    advance_input(state);
                }
                while (((state->flags & SCAN_UNLIMITED) || state->width > 0) &&
                       (_ctype[state->current.code] & _DIGIT)) {
                    advance_input(state);
                }
            }
        }
        if (((state->flags & SCAN_UNLIMITED) || state->width > 0) &&
            (((_ctype[state->current.code] & _UPPER)
                  ? state->current.signed_byte + 32
                  : state->current.signed_byte) == 'f' ||
             ((_ctype[state->current.code] & _UPPER)
                  ? state->current.signed_byte + 32
                  : state->current.signed_byte) == 'l')) {
            advance_input(state);
        }
        goto store_decimal;
    }
    if (((state->flags & SCAN_UNLIMITED) || state->width > 0) &&
        ((_ctype[state->current.code] & _UPPER)
             ? state->current.signed_byte + 32
             : state->current.signed_byte) == 'e') {
        advance_input(state);
        if (((state->flags & SCAN_UNLIMITED) || state->width > 0) &&
            (state->current.signed_byte == '+' ||
             state->current.signed_byte == '-')) {
            if (state->current.signed_byte == '-') {
                exponent_sign = -1;
            }
            advance_input(state);
        }
        while (((state->flags & SCAN_UNLIMITED) || state->width > 0) &&
               state->current.signed_byte == '0') {
            advance_input(state);
        }
        while (((state->flags & SCAN_UNLIMITED) || state->width > 0) &&
               (_ctype[state->current.code] & _DIGIT)) {
            if (exponent_digit_count < EXPONENT_DIGITS) {
                exponent_digits[exponent_digit_count++] =
                    state->current.signed_byte;
            } else {
                errno = EEXP;
                limit.range = exponent_sign;
            }
            advance_input(state);
        }
    }
    if (((state->flags & SCAN_UNLIMITED) || state->width > 0) &&
        (((_ctype[state->current.code] & _UPPER)
              ? state->current.signed_byte + 32
              : state->current.signed_byte) == 'f' ||
         ((_ctype[state->current.code] & _UPPER)
              ? state->current.signed_byte + 32
              : state->current.signed_byte) == 'l')) {
        /* @hack type: the signed byte masked to index _ctype in place of code,
         * so that one load gives both the index and the value (MOV.B
         * @R14,R4 at 0x0cb85bcc in EV001E); through code, 29 of 541 words
         * differ */
        if (((_ctype[state->current.signed_byte & 0xff] & _UPPER)
                 ? state->current.signed_byte + 32
                 : state->current.signed_byte) == 'l') {
            wide_limit.suffix.range_errno = RANGE_ERRNO_LONG_DOUBLE;
        } else {
            wide_limit.suffix.range_errno = RANGE_ERRNO_FLOAT;
        }
        advance_input(state);
    } else {
        wide_limit.suffix.range_errno = RANGE_ERRNO_DOUBLE;
    }
    if (!limit.range) {
        while (digit_count < SCAN_DIGITS) {
            digits[digit_count++] = '0';
        }
        exponent = 0;
        for (index = 0; index < exponent_digit_count; ++index) {
            exponent = (exponent * 10) + exponent_digits[index] - '0';
        }
        if (exponent_sign == -1) {
            exponent = -exponent;
        }
        exponent += before_point - 1;
        if (exponent >= 1000) {
            errno = EEXPN;
            limit.range = 1;
        } else if (exponent <= -1000) {
            errno = EEXPN;
            limit.range = -1;
        }
    }
    switch (limit.range) {
    case 1:
        /* @hack redundant: a switch of only a default, which loads the length
         * and tests nothing (MOV.B @(1,R14),R0 at 0x0cb85c88 in EV001E);
         * without it, 86 of 541 words differ */
        switch (state->length) {
        default:
            result = (char *)&limit;
            break;
        }
        /* @bug logic: a negative overflow sets the sign bit in limit's
         * byte 7, where a double keeps its sign. here that byte is range's
         * high byte (MOV.B R0,@(7,R12) at 0x0cb85c9a in EV001E). the
         * destination takes the float in bytes 0 to 3 (FMOV.S @R12,FR3 at
         * 0x0cb85cd2 or 0x0cb85cda), so -inf comes back as +inf */
        if (sign == -1) {
            result[7] |= 0x80;
        }
        break;
    case -1:
        result = (char *)zero.bytes;
        /* @bug logic: a negative underflow sets the sign bit of zero's
         * byte 7 (MOV.B R0,@(7,R12) at 0x0cb85cac in EV001E). the destination
         * takes the float in bytes 0 to 3, so -0 comes back as +0 */
        if (sign == -1) {
            result[7] |= 0x80;
        }
        break;
    case 0:
        /* @hack redundant: a switch of only a default, as above (MOV.B
         * @(1,R14),R0 at 0x0cb85cb2 in EV001E); without it, 70 of 541 words
         * differ */
        switch (state->length) {
        default:
            converted =
                _stf(sign, digits, exponent, wide_limit.suffix.range_errno);
            result = (char *)&converted;
            break;
        }
        break;
    }
store_decimal:
    /* under -fpu=single, double and long double are floats too, so each
     * store writes one float */
    if (destination) {
        if (state->length == 'L') {
            *(long double *)destination = *(float *)result;
        } else if (state->length == 'l') {
            *(double *)destination = *(float *)result;
        } else {
            *(float *)destination = *(float *)result;
        }
    }
    return !matched;
}

/* read the set after the '[' at *format, leave *format at the set's ']',
 * and store the bytes it matches through destination. returns 1 when the input
 * has ended or the set has no ']', and else 0. a set without ']' also sets
 * errno to ECSPEC */
static int read_set(const char **format, char *destination,
                    struct input_state *state)
{
    const char *begin;
    const char *scan;
    char invert;

    ++*format;
    if (**format == '^') {
        invert = 1;
        ++*format;
    } else {
        invert = 0;
    }
    begin = *format;
    while (**format != ']' && **format != '\0') {
        ++*format;
    }
    if (**format == '\0') {
        errno = ECSPEC;
        return 1;
    }
    if (state->current.signed_byte == -1) {
        return 1;
    }
    while (((state->flags & SCAN_UNLIMITED) || state->width > 0) &&
           state->current.signed_byte != -1) {
        for (scan = begin; scan < *format; ++scan) {
            if (state->current.signed_byte == *scan) {
                break;
            }
        }
        /* @hack spelling: invert compared with 0, which loads the set's end
         * into R2 on its branch (MOV.L @R14,R2 at 0x0cb85d8a in EV001E); as
         * !invert, 2 of 101 words differ */
        if (invert == 0) {
            if (scan >= *format) {
                break;
            }
        } else if (scan < *format) {
            break;
        }
        if (destination) {
            *destination++ = state->current.signed_byte;
        }
        advance_input(state);
    }
    if (destination) {
        *destination = 0;
    }
    return 0;
}

/* set current to the next byte, or to -1 at the end of the input. a byte
 * that doesn't end the input counts in fetched_count and against width */
static void advance_input(struct input_state *state)
{
    if (!(state->flags & SCAN_STREAM)) {
        if (!(state->current.signed_byte = *state->cursor.text++)) {
            state->current.signed_byte = -1;
        }
    } else {
        state->current.signed_byte = state->read(state->cursor.stream);
    }
    if (state->current.signed_byte != -1) {
        ++state->fetched_count;
        /* @bug uninitialized: _fmtin calls here (BSR at 0x0cb85048, 0x0cb85098
         * and 0x0cb850e0 in EV001E) before its first store of width (MOV.L
         * R12,@(12,R14) at 0x0cb85132). this read of width (MOV.L
         * @(12,R14),R3 at 0x0cb85e12) then takes a word no path has written.
         * the value goes unused, since each directive sets width before it
         * reads it */
        state->width = (unsigned int)state->width - 1;
    }
}

/* step the input back over current */
static void restore_input(struct input_state *state)
{
    if (!(state->flags & SCAN_STREAM)) {
        --state->cursor.text;
    } else {
        ++state->cursor.stream->_bufcnt;
        --state->cursor.stream->_bufptr;
    }
}
