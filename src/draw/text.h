/* debug text: a copy of ninja's print with a color and size of its own,
 * drawn with the font and polygon header njInitPrint sets up */
#ifndef DRAW_TEXT_H
#define DRAW_TEXT_H
#include <ninja.h>

/* make the color 0xffbfbfbf and a character's cell 16 pixels a side, and
 * copy the font's polygon header from the vertex context njInitPrint set
 * up. call it after njInitPrint. its arguments are njInitPrint's, and go
 * unread */
void text_style_init(Uint32 *texture, Uint32 size, Uint32 attr);
/* do nothing: the font is njInitPrint's, and njExitPrint frees it */
void text_exit(void);
/* set the side of a character's cell, in pixels */
void text_set_size(Uint16 size);
/* set the color, a packed ARGB word */
void text_set_color(Uint32 color);

/* draw string from the cell at loc, a column and row as NJM_LOCATION packs
 * them, one character a cell. text_print_front draws it in front of what
 * text_print draws. every print here draws only on a frame's first step,
 * while frame_step is 0. a character is a signed char, so a code past 127
 * maps outside the font's texture */
void text_print(Int loc, const char *string);
void text_print_front(Int loc, const char *string);
/* draw value in decimal, zero-filled to digits places, after a '-' when it
 * is negative. value must fit in digits places, and digits be 10 at most */
void text_print_number(Int loc, Int value, Int digits);
/* draw value's low digits binary or hexadecimal digits, the highest first */
void text_print_binary(Int loc, Uint32 value, Int digits);
void text_print_hex(Int loc, Uint32 value, Int digits);
/* draw value with its whole part zero-filled to digits places, then its
 * point and five places, after a '-' when it is negative. the whole part
 * must fit in digits places, and digits be 13 at most */
void text_print_float(Int loc, Float value, Int digits);
/* text_print the string vsprintf makes of format and its arguments, which
 * must come to 127 characters at most */
void text_printf(Int loc, const char *format, ...);

#endif
