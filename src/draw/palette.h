/* the palette routines, in assembly: copies, fills, a blend and a rotation of
 * colors. a palette in draw_palettes is 256 entries of 8 bytes, each a base
 * color and then an offset color. its base colors lie 8 bytes apart, as do
 * its offset colors. the colors a routine reads and those it writes must be
 * the same or not overlap */
#ifndef DRAW_PALETTE_H
#define DRAW_PALETTE_H
#include <sg_xpt.h>

/* keep pointer, which fun_8c0566ac returns */
void fun_8c0566a0(void *pointer);
void *fun_8c0566ac(void);
/* the address of draw_palettes[palette]'s base colors, or of its offset
 * colors when offsets is nonzero */
Uint32 *palette_get(int palette, int offsets);
/* copy colors, one after another, from in to out: 256 for fun_8c0566d0 and
 * 512 for fun_8c0566ea. with in equal to out they write nothing */
void fun_8c0566d0(Uint32 *out, Uint32 *in);
void fun_8c0566ea(Uint32 *out, Uint32 *in);
/* store color in 128 colors 8 bytes apart at out, for fun_8c056704.
 * fun_8c056724 stores it in 256 colors one after another, in the same 1024
 * bytes */
void fun_8c056704(Uint32 *out, Uint32 color);
void fun_8c056724(Uint32 *out, Uint32 color);
/* blend 256 colors 8 bytes apart: out gets share of from's and the rest of
 * to's. with to equal to from it writes nothing. for share from 0 to 1, let
 * w be share * 256, truncated. each channel of out, one of a color's low
 * three bytes, is then (from's * w + to's * (256 - w)) >> 8. each color's
 * top byte is 0 */
void palette_mix(Uint32 *out, Uint32 *from, Uint32 *to, Float share);
/* rotate 256 colors 8 bytes apart by shift: in's color k goes to out's color
 * (k + shift) % 256. shift 0 writes nothing. with in equal to out, the colors
 * stay as they are. that path saves the last shift colors, 8 bytes apart, in
 * a buffer with room for 128. a larger shift also writes over the pointer
 * fun_8c0566a0 keeps. above 132 it writes over main's code, at 0x8c056cc0,
 * and what follows it, up to 0x8c057090 at shift 255 */
void fun_8c0567dc(Uint32 *out, Uint32 *in, Uint8 shift);
/* copy 256 colors from in to out. fun_8c056860 reads them one after another
 * and writes them 8 bytes apart. fun_8c05687a reads them 8 bytes apart and
 * writes them one after another. with in equal to out they write nothing */
void fun_8c056860(Uint32 *out, Uint32 *in);
void fun_8c05687a(Uint32 *out, Uint32 *in);
/* return at once */
void fun_8c056896(void);

#endif
