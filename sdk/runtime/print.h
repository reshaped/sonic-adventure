/* C runtime output conversion */
#ifndef RUNTIME_PRINT_H
#define RUNTIME_PRINT_H

#include <stdarg.h>
#include <stdio.h>

/* _fmtout's modes: where its text goes */
enum { PRINT_TO_STREAM, PRINT_TO_STRING };

/* the formatter under sprintf and vsprintf. it writes the text format makes
 * of arguments to destination, and returns the bytes written, less the null.
 *
 * mode is PRINT_TO_STREAM or PRINT_TO_STRING. with PRINT_TO_STRING, it writes
 * the text and a null to the string at destination, and ignores put. with
 * PRINT_TO_STREAM, it gives each byte to put with the FILE at destination.
 * put gets each byte as a char, and bytes 0x80 to 0xff arrive negative. the
 * bytes come in runs. a run is the literal text between directives, a
 * conversion's text whole or split where its zeros go, or one fill or zero
 * byte. when put refuses a byte with -1, that byte and the rest of its run
 * drop, uncounted.
 *
 * with PRINT_TO_STREAM, it returns -1 at once for a null FILE, one not open,
 * one open only for reading, or one with _IOERR set. errno becomes PTRERR,
 * NOTOPN or EBADF for the first three, in that order. once _IOERR is set, the
 * formatter stops before the next directive or literal text.
 *
 * it departs from ISO C as follows:
 * - %e, %f and %g take a float, which is what -fpu=single passes for a
 *   double. L changes nothing for them.
 * - they give 17 significant digits, then zeros, rounded half up: %.2f of
 *   0.125 is 0.13.
 * - %g takes %e when the value's exponent, before rounding, is below -4 or
 *   above the precision: %g of 0.0001f is 1e-04. its precision counts the
 *   digits after the point either way: %g of 123.456f is 123.456001.
 * - an infinity prints as its sign and a NaN as '*', each repeated the larger
 *   of width and precision times. negative zero prints as positive zero.
 * - with the space flag ahead of +, a finite float not below zero gets no
 *   sign. a sign or space doesn't count toward the width: %8.2f of -1.5 is
 *   "    -1.50".
 * - the 0 flag pads %c, %s and %% too. it pads on the side the - flag picks,
 *   ahead of a 0x and even with a precision. %-05d of 42 is 42000, %#06x of
 *   255 is 000xff and %08.3d of 5 is 00000005. after a sign or space, the
 *   zeros follow it, even with -.
 * - an integer's precision counts its sign, space or 0x: %.3d of -5 is -05
 *   and %#.5x of 255 is 0x0ff. %#.0o, %+.0d and % .0d of 0 have no text.
 * - %% takes flags and a width, as %c does. %s reads its string to the null,
 *   even with a precision. a '.' without digits leaves the default
 *   precision. %p is %x of the address. %n stores an int, even for %hn.
 * - errno becomes ECSPEC for an invalid conversion, a width or precision past
 *   512, or a flag after the 0 flag. the output then ends before that
 *   directive, but such a %n still stores the count.
 *
 * its bugs:
 * - a * width of INT_MIN passes the 512 limit. it pads %c and %% with
 *   2^31 - 1 fill bytes, and a %s of nonzero length with 2^31 less that
 *   length. the count returned then wraps past INT_MAX.
 * - a directive the format's null cuts short, such as a trailing %, is
 *   invalid, and the formatter reads the byte after the null.
 * - %#x or %#X of 0 with a precision above 1 writes 0, a null and the
 *   precision less one zeros. it then writes the rest of its text with a
 *   length of -1. a string takes that as a memcpy of (size_t)-1 bytes, which
 *   writes far outside it, and a stream as nothing. the - flag's padding and
 *   the rest of the format follow */
int _fmtout(int mode, int (*put)(int c, FILE *stream), void *destination,
            const char *format, va_list arguments);

/* format_parts' flags */
enum {
    PRINT_STOP = 1, /* the directive is invalid, and the output ends there */
    PRINT_LEFT = 2, /* the - flag */
    PRINT_ALTERNATE = 4, /* the # flag */
    PRINT_PLUS = 8, /* the + flag */
    PRINT_SPACE = 16, /* the space flag, unless + came first */
    PRINT_G = 32 /* %g or %G, set by _xtostr */
};

/* a directive and the text of its conversion. the formatter writes the text
 * in pieces, with zeros between them:
 *
 *   padding fill bytes, which PRINT_LEFT moves to the end
 *   the text up to prefix_end, then prefix_zeros zeros
 *   the text from there up to digits, then digit_zeros zeros
 *   the rest of the text, then suffix_zeros zeros */
struct format_parts {
    char flags;
    char fill; /* ' ', '0' for the 0 flag, or an infinity's sign or NaN's '*' */
    short unk_2;
    int width;
    int precision; /* -1 when the directive gives none */
    int length; /* the length modifier, 'h', 'l' or 'L', or ' ' for none */
    char *prefix_end;
    char *digits;
    char *suffix; /* the end of the text, where suffix_zeros go */
    int prefix_zeros;
    int digit_zeros;
    int suffix_zeros;
    int padding;
};

/* conversion is 'e', 'E', 'f', 'g' or 'G'. write the float at *arguments to
 * text as that conversion does, and step *arguments past it. the text takes at
 * most 24 bytes with its null. parts must give the directive, with prefix_end
 * and digits pointing at text and the three zero counts 0. _xtostr sets
 * parts' pointers, zero counts and padding for the text. it also changes
 * parts' precision and flags. a precision of -1 means 6. an infinity writes
 * "" and sets fill to its sign, and a NaN writes "" and sets fill to '*'.
 * either sets padding to the larger of width and precision. _xtostr returns
 * with FPSCR.PR clear */
void _xtostr(va_list *arguments, char *text, int conversion,
             struct format_parts *parts);

#endif
