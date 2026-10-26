/* the menus' and the staff roll's bitmap fonts. a font's texture holds a cell
 * for each character, sixteen cells to a row */
#ifndef ADVERTISE_FONT_H
#define ADVERTISE_FONT_H

#include <ninja.h>
#include <sg_xpt.h>

struct advertise_font {
    Float cell_width;
    Float cell_height;
    Float texture_width;
    Float texture_height;
    NJS_TEXLIST *texlist;
    Uint8 *widths; /* by character, or NULL for the cell width */
    /* at the top left, bottom left, top right and bottom right */
    Uint32 colors[4];
};

/* abc_txt's, 16 by 16 cells, which the sound test draws in */
extern struct advertise_font abc_font;
/* staffroll_txt's, 32 by 32 cells, which the staff roll's credits draw in */
extern struct advertise_font staffroll_font;

/* load or release font's texlist, if it has one. advertise_font_load takes a
 * null font and does nothing, but advertise_font_release requires a font */
void advertise_font_load(struct advertise_font *font);
void advertise_font_release(struct advertise_font *font);
/* fill font in from texlist, widths and the cell's size, with every color
 * white. the texture's size is the size of texlist's first texture. a null
 * font does nothing */
void advertise_font_init(struct advertise_font *font, NJS_TEXLIST *texlist,
                         Uint8 *widths, Float cell_width, Float cell_height);
/* copy font into the font in use, which the calls below draw with. later
 * changes to font don't reach the copy */
void advertise_font_use(struct advertise_font *font);
/* color every corner of the font in use */
void advertise_font_set_color(Uint32 color);
/* color each corner of the font in use */
void advertise_font_set_colors(Uint32 top_left, Uint32 bottom_left,
                               Uint32 top_right, Uint32 bottom_right);
/* draw character's cell with its top left at x and y. z is its depth, which
 * the cell's vertices take as 1 / |z|. it returns the advance to the next
 * character: the character's width plus 2, or the cell width for a font
 * without widths. character must be 0 to 127: char is signed, so a byte
 * above 127 indexes the widths below their start */
int advertise_font_draw_char(char character, Float x, Float y, Float z);
/* draw text from x. a character of width w takes w + 1 dots. its cell is
 * drawn left of where the character starts by (cell width - w) / 2,
 * truncated. a character of width 0 draws nothing and takes 4 dots. the font
 * in use must have widths */
void advertise_font_draw_string(const char *text, Float x, Float y, Float z);
/* how far advertise_font_draw_string moves across text. the font in use must
 * have widths */
int advertise_font_string_width(const char *text);

#endif
