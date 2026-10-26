/* the formatter under sprintf and vsprintf */
#include <ctype.h>
#include <errno.h>
#include <machine.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include "runtime/print.h"

/* FPSCR's double-precision mode bit */
#define FPSCR_PR 0x80000

/* where _fmtout's text goes, and count, the bytes written there so far */
struct output_cursor {
    int mode;
    void *destination;
    int count;
    int (*put)(int c, FILE *stream);
};

static void integer_text(long value, char *text, int conversion,
                         struct format_parts *parts);
static void emit_text(const char *text, int length,
                      struct output_cursor *output);
static void terminate_text(struct output_cursor *output);
static int output_error(struct output_cursor *output);

int _fmtout(int mode, int (*put)(int c, FILE *stream), void *destination,
            const char *format, va_list arguments)
{
    struct output_cursor output_storage;
    struct format_parts parts_storage;
    char buffer[128];
    char *buffer_start;
    int length;
    char zero;
    char *text;
    const char *end;
    int *count_target;
    int digit;
    int emitted;
    int segment;
    int fpscr;
    /* @hack spelling: output and parts reached through pointers, which SHC
     * keeps in R10 and R14 (MOV.B @R14,R0 at 0x8c122d90 reads parts->flags); as
     * plain locals, their fields take R0 offsets from the frame, the function
     * is 364 bytes longer, and 867 of 876 words differ */
    struct output_cursor *output = &output_storage;
    struct format_parts *parts = &parts_storage;

    output->mode = PRINT_TO_STREAM;
    output->destination = NULL;
    output->count = 0;
    parts->flags = 0;
    parts->fill = 0;
    parts->unk_2 = 0;
    parts->width = 0;
    parts->precision = 0;
    parts->prefix_end = NULL;
    parts->digits = NULL;
    parts->suffix = NULL;
    parts->prefix_zeros = 0;
    parts->digit_zeros = 0;
    parts->suffix_zeros = 0;
    parts->padding = 0;
    if (mode == PRINT_TO_STREAM) {
        if (destination == NULL) {
            errno = PTRERR;
            return -1;
        }
        if (!(((FILE *)destination)->_ioflag1 & (_IOREAD | _IOWRITE | _IORW))) {
            errno = NOTOPN;
            return -1;
        }
        if (!(((FILE *)destination)->_ioflag1 & _IORW) &&
            (((FILE *)destination)->_ioflag1 & _IOREAD)) {
            errno = EBADF;
            return -1;
        }
        /* @hack redundant: a store of put to mode that never runs; without it,
         * the l and h fetches' cursor temporaries trade R5 and R6 and 18 of 876
         * words differ */
        if (0) {
            mode = (int)put;
        }
        output->put = put;
    }
    output->mode = mode;
    output->destination = destination;
    output->count = 0;
    parts->flags = 0;
    zero = '0';
    if (output_error(output)) {
        return -1;
    }
    /* @hack spelling: the %c and %% stores through a copy of buffer's address;
     * through buffer[0], the .* precision, l, unsigned h, signed h, %d and %p
     * fetches trade their temporaries' registers (MOV.L @(R0,R15),R4 at
     * 0x8c122e94), and 40 of 876 words differ */
    buffer_start = &buffer[0];
    /* @bug memory: a directive the format's null cuts short is
     * invalid, and the format++ after it steps past the null. this test then
     * reads the byte after the null (0x8c123396 in 1ST_READ.BIN, 0x8ce04706 in
     * LOADER.BIN) before it tests PRINT_STOP */
    while (*format && !output_error(output) && !(parts->flags & PRINT_STOP)) {
        if (*format == '%') {
            format++;
            parts->flags = 0;
            parts->prefix_end = buffer;
            parts->digits = buffer;
            parts->suffix = NULL;
            parts->prefix_zeros = 0;
            parts->digit_zeros = 0;
            parts->suffix_zeros = 0;
            parts->padding = 0;
            while (*format == '-' || *format == '+' || *format == ' ' ||
                   *format == '#') {
                switch (*format) {
                case '-':
                    parts->flags |= PRINT_LEFT;
                    break;
                case '+':
                    parts->flags |= PRINT_PLUS;
                    break;
                case ' ':
                    if (!(parts->flags & PRINT_PLUS)) {
                        parts->flags |= PRINT_SPACE;
                    }
                    break;
                case '#':
                    parts->flags |= PRINT_ALTERNATE;
                    break;
                }
                format++;
            }
            parts->fill = ' ';
            if (*format == '0') {
                parts->fill = '0';
                format++;
            }
            parts->width = 0;
            if (*format == '*') {
                parts->width = va_arg(arguments, int);
                if (parts->width < 0) {
                    parts->flags |= PRINT_LEFT;
                    /* @bug arithmetic: an INT_MIN width stays negative after
                     * its negation (NEG R3,R3 at 0x8c122e08), and the signed
                     * test of the 512 limit below passes it (CMP/GT R2,R1 at
                     * 0x8c122e10). %c and %% then pad with 2^31 - 1 fill
                     * bytes, and a %s of nonzero length with 2^31 less that
                     * length */
                    parts->width = -(unsigned int)parts->width;
                }
                if (parts->width > 512) {
                    parts->flags |= PRINT_STOP;
                    errno = ECSPEC;
                }
                format++;
            }
            if (_ctype[(unsigned char)*format] & _DIGIT) {
                parts->width = 0;
                while (_ctype[(unsigned char)*format] & _DIGIT) {
                    digit = *format - '0';
                    if ((512 - digit) / 10 >= parts->width) {
                        parts->width = (parts->width * 10) + digit;
                    } else {
                        parts->flags |= PRINT_STOP;
                        errno = ECSPEC;
                    }
                    format++;
                }
            }
            parts->precision = -1;
            if (*format == '.') {
                if (*++format == '*') {
                    parts->precision = va_arg(arguments, int);
                    if (parts->precision < 0) {
                        parts->precision = -1;
                    }
                    if (parts->precision > 512) {
                        parts->flags |= PRINT_STOP;
                        errno = ECSPEC;
                    }
                    format++;
                }
                if (_ctype[(unsigned char)*format] & _DIGIT) {
                    parts->precision = 0;
                    while (_ctype[(unsigned char)*format] & _DIGIT) {
                        digit = *format - '0';
                        if ((512 - digit) / 10 >= parts->precision) {
                            parts->precision = (parts->precision * 10) + digit;
                        } else {
                            parts->flags |= PRINT_STOP;
                            errno = ECSPEC;
                        }
                        format++;
                    }
                }
            }
            parts->length = ' ';
            if (*format == 'h' || *format == 'l' || *format == 'L') {
                parts->length = (int)*format;
                format++;
            }
            switch (*format) {
            case 'd':
            case 'i':
            case 'o':
            case 'u':
            case 'x':
            case 'X': {
                long value;

                if (parts->length == 'l') {
                    value = va_arg(arguments, long);
                } else if (parts->length == 'h') {
                    if (*format == 'u' || *format == 'X' || *format == 'x' ||
                        *format == 'o') {
                        value = (unsigned short)va_arg(arguments, int);
                    } else {
                        value = (short)va_arg(arguments, int);
                    }
                } else {
                    if (*format == 'u' || *format == 'X' || *format == 'x' ||
                        *format == 'o') {
                        value = va_arg(arguments, unsigned int);
                    } else {
                        value = va_arg(arguments, int);
                    }
                }
                if (parts->precision == -1) {
                    parts->precision = 1;
                }
                integer_text(value, buffer, *format, parts);
                text = buffer;
                length = strlen(text);
                break;
            }
            case 'e':
            case 'E':
            case 'f':
            case 'g':
            case 'G':
                _xtostr(&arguments, buffer, *format, parts);
                /* _xtostr returns with FPSCR.PR clear, and this clears it
                 * again */
                /* @hack spelling: FPSCR read, masked and written in three
                 * statements, which give FPSCR and its mask R4 and R3; in one
                 * statement, they take R3 and R2, the * width, unsigned h and
                 * %p fetches trade registers (MOV.L @(R0,R15),R4 at
                 * 0x8c122dcc), and 29 of 876 words differ. masked as it is
                 * read, or with &=, the %p fetch's R4 and R5 trade, and 8
                 * differ */
                fpscr = get_fpscr();
                fpscr = fpscr & ~FPSCR_PR;
                set_fpscr(fpscr);
                text = buffer;
                length = strlen(text);
                break;
            case 'c':
                *buffer_start = va_arg(arguments, int);
                text = buffer;
                length = 1;
                /* @hack redundant: digits and digit_zeros stored again, though
                 * the directive's start set both, which lets %c branch into
                 * the stores of %% (BRA at 0x8c123158 to 0x8c12324a); without
                 * them, the function is 8 bytes longer and 353 of 876 words
                 * differ */
                parts->digits = buffer;
                parts->digit_zeros = 0;
                /* unsigned, so that the subtraction wraps for an INT_MIN width
                 * in C as on the machine, here and in the %s and %% padding
                 * below */
                parts->padding = (int)((unsigned int)parts->width - 1);
                break;
            case 's':
                text = va_arg(arguments, char *);
                length = strlen(text);
                if (parts->precision != -1 && parts->precision < length) {
                    length = parts->precision;
                }
                /* @hack redundant: digits and digit_zeros stored again, though
                 * the directive's start set both (0x8c1231a6, 0x8c1231a8);
                 * without them, the function is 4 bytes shorter and 325 of
                 * 876 words differ */
                parts->digits = buffer;
                parts->digit_zeros = 0;
                parts->padding = (int)((unsigned int)parts->width - length);
                break;
            case 'p': {
                long value = (long)va_arg(arguments, void *);

                if (parts->precision == -1) {
                    parts->precision = 1;
                }
                integer_text(value, buffer, 'x', parts);
                text = buffer;
                length = strlen(text);
                break;
            }
            case 'n':
                /* @hack spelling: the store through count_target, for which the
                 * count takes R1 and the pointer R4 (0x8c12323c, 0x8c12323e);
                 * stored through the fetch, the count takes R2, the pointer
                 * and the %% store's address R1, the .* precision and %d
                 * fetches trade registers, and 19 of 876 words differ */
                count_target = va_arg(arguments, int *);
                *count_target = output->count;
                break;
            case '%':
                *buffer_start = '%';
                text = buffer;
                length = 1;
                /* @hack redundant: digits and digit_zeros stored again, though
                 * the directive's start set both (0x8c123250, 0x8c123252), in
                 * code %c shares; without them, the function is 8 bytes longer
                 * and 354 of 876 words differ */
                parts->digits = buffer;
                parts->digit_zeros = 0;
                parts->padding = (int)((unsigned int)parts->width - 1);
                break;
            default:
                errno = ECSPEC;
                parts->flags |= PRINT_STOP;
                break;
            }
            if (*format != 'n' && !(parts->flags & PRINT_STOP)) {
                /* @hack redundant: padding tested ahead of the loop, which
                 * repeats the loop's own test (CMP/PL R2 at 0x8c12327e);
                 * without it, the function is 12 bytes shorter and 182 of 876
                 * words differ */
                if (!(parts->flags & PRINT_LEFT) && parts->padding > 0) {
                    while (parts->padding > 0) {
                        emit_text(&parts->fill, 1, output);
                        parts->padding--;
                    }
                }
                if (parts->prefix_zeros == 0 && parts->digit_zeros == 0 &&
                    parts->suffix_zeros == 0) {
                    emit_text(text, length, output);
                } else {
                    segment = parts->prefix_end - text;
                    emitted = segment;
                    if (segment > 0) {
                        emit_text(text, segment, output);
                    }
                    while (parts->prefix_zeros > 0) {
                        emit_text(&zero, 1, output);
                        parts->prefix_zeros--;
                    }
                    segment = parts->digits - parts->prefix_end;
                    emitted += segment;
                    if (segment > 0) {
                        emit_text(parts->prefix_end, segment, output);
                    }
                    while (parts->digit_zeros > 0) {
                        emit_text(&zero, 1, output);
                        parts->digit_zeros--;
                    }
                    emit_text(parts->digits, length - emitted, output);
                    while (parts->suffix_zeros > 0) {
                        emit_text(&zero, 1, output);
                        parts->suffix_zeros--;
                    }
                }
                /* @hack redundant: padding tested ahead of the loop, which
                 * repeats the loop's own test (CMP/PL R2 at 0x8c123350);
                 * without it, the function is 8 bytes shorter and 182 of 876
                 * words differ */
                if ((parts->flags & PRINT_LEFT) && parts->padding > 0) {
                    while (parts->padding > 0) {
                        emit_text(&parts->fill, 1, output);
                        parts->padding--;
                    }
                }
            }
            format++;
        } else {
            for (end = format; *end != '\0' && *end != '%'; end++) {
            }
            emit_text(format, end - format, output);
            format = end;
        }
    }
    terminate_text(output);
    return output->count;
}

/* write value's text to text: its sign, space or 0x, its digits and a null.
 * conversion is an integer conversion's character, or 'x' for %p. it sets
 * parts->digits, digit_zeros and padding for the text */
static void integer_text(long value, char *text, int conversion,
                         struct format_parts *parts)
{
    char *start;
    int digits;
    int total;
    int effective;
    int radix;
    unsigned long magnitude;
    char reversed[12];
    int i;
    int pending;

    start = text;
    /* @hack spelling: the null stored through start, for which start takes R10
     * and the zero R9 (0x8c1233e2, 0x8c1233e4); stored through text, the two
     * trade and 14 of 257 words differ */
    *start = '\0';
    switch (conversion) {
    case 'd':
    case 'i':
    case 'u':
        radix = 10;
        break;
    case 'o':
        radix = 8;
        break;
    case 'x':
    case 'X':
        radix = 16;
        break;
    }
    if ((conversion == 'd' || conversion == 'i') && value < 0) {
        magnitude = -(unsigned long)value;
    } else {
        magnitude = value;
    }
    digits = 0;
    total = 0;
    if (magnitude == 0 && parts->precision != 0) {
        reversed[0] = '0';
        digits = 1;
    } else {
        while (magnitude > 0) {
            reversed[digits] = magnitude % radix;
            if (reversed[digits] <= 9) {
                reversed[digits] += '0';
            } else if (conversion == 'x') {
                reversed[digits] += 'a' - 10;
            } else {
                reversed[digits] += 'A' - 10;
            }
            magnitude = magnitude / radix;
            digits++;
        }
    }
    if (parts->flags & PRINT_ALTERNATE) {
        switch (conversion) {
        case 'o':
            if (value != 0) {
                reversed[digits++] = '0';
            }
            break;
        case 'x':
        case 'X':
            if (value != 0) {
                *text++ = '0';
                *text++ = conversion;
                total = 2;
            }
            break;
        }
    }
    /* @hack spelling: a copy of digits for the loop below, which keeps the
     * count in a frame word (MOV.L R13,@R15 at 0x8c1234d0); with digits alone,
     * the count stays in R13 and 75 of 257 words differ */
    pending = digits;
    total += digits;
    if ((conversion == 'd' || conversion == 'i') &&
        (parts->precision != 0 || value != 0)) {
        if (value >= 0) {
            if (parts->flags & PRINT_PLUS) {
                total++;
                *text++ = '+';
            } else if (parts->flags & PRINT_SPACE) {
                total++;
                *text++ = ' ';
            }
        } else {
            total++;
            *text++ = '-';
        }
    }
    if (*start == '+' || *start == '-' || *start == ' ') {
        parts->digits = start + 1;
    } else if (parts->flags & PRINT_ALTERNATE) {
        switch (conversion) {
        case 'X':
        case 'x':
            /* @bug memory: digits points past a 0x even for 0, which
             * has none (0x8c123548 in 1ST_READ.BIN, 0x8ce048b8 in LOADER.BIN).
             * with a precision above 1, _fmtout then writes the rest of the
             * text as -1 bytes, which a string takes as a memcpy of
             * (size_t)-1 bytes */
            parts->digits = start + 2;
            break;
        case 'o':
            parts->digits = start + 1;
            break;
        /* @hack redundant: an empty default, for which the precision test loads
         * into R3 (MOV.L @(8,R7),R3 at 0x8c123550); without it, the loads in
         * the precision test and in the width test after a sign take R2, a
         * failed width test retests the width, and 5 of 257 words differ */
        default:
            break;
        }
    } else {
        parts->digits = start;
    }
    if (parts->precision > total) {
        effective = parts->precision;
        parts->digit_zeros = parts->precision - total;
    } else {
        effective = total;
        parts->digit_zeros = 0;
    }
    if ((*start == '+' || *start == '-' || *start == ' ') &&
        parts->width > effective && parts->fill == '0') {
        parts->digit_zeros += parts->width - effective;
        parts->padding = 0;
    } else if (parts->width > effective) {
        parts->padding = parts->width - effective;
    } else {
        parts->padding = 0;
    }
    /* @hack spelling: i stepped in the subscript, which indexes reversed
     * through R0 (0x8c1235aa to 0x8c1235b2); stepped in the loop's header, the
     * subscript keeps a pointer of its own, the function is 4 bytes longer,
     * and 22 of 257 words differ */
    for (i = pending - 1; i >= 0;) {
        *text++ = reversed[i--];
    }
    *text = '\0';
}

/* write length bytes of text and count them. a string takes them at once. a
 * stream takes them a byte at a time through put, and the first byte put
 * refuses with -1 ends the call, uncounted. the count wraps past INT_MAX (the
 * ADDs at 0x8c1235fe and 0x8c12361c in 1ST_READ.BIN, 0x8ce0496e and
 * 0x8ce0498c in LOADER.BIN) */
static void emit_text(const char *text, int length,
                      struct output_cursor *output)
{
    int i;

    if (output->mode == PRINT_TO_STRING) {
        memcpy(output->destination, text, length);
        output->destination = (char *)output->destination + length;
        output->count += length;
    } else {
        for (i = 0; i < length; i++) {
            if (output->put(*text++, output->destination) == -1) {
                return;
            }
            output->count++;
        }
    }
}

static void terminate_text(struct output_cursor *output)
{
    if (output->mode == PRINT_TO_STRING) {
        *(char *)output->destination = '\0';
    }
}

/* the stream's _IOERR bit, or 0 for a string */
static int output_error(struct output_cursor *output)
{
    if (output->mode == PRINT_TO_STRING) {
        return 0;
    }
    return ((FILE *)output->destination)->_ioflag1 & _IOERR;
}
