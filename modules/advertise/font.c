/* the menus' and the staff roll's bitmap fonts, and the drawing of text in
 * them */
#include <ninja.h>
#include <sg_xpt.h>

#include "advertise/font.h"
#include "draw/screen.h"

#define TEXT_1_LENGTH 7
#define TEXT_2_LENGTH 13
#include "advertise/font/text.inc"

#define WHITE 0xffffffff
#define COLUMNS 16 /* the cells in a row of a font's texture */
/* the dots a character of width 0 takes in a string, where it draws nothing */
#define BLANK_ADVANCE 4

static Uint8 abc_widths[128] = {
#include "advertise/font/abc_widths.inc"
};
static NJS_TEXNAME abc_textures[1] = {{TEXT_1, 0, 0}};
static NJS_TEXLIST abc_texlist = {
#include "advertise/font/abc_texlist.inc"
};
struct advertise_font abc_font = {
    16.0F,
    16.0F,
    256.0F,
    128.0F,
    &abc_texlist,
    abc_widths,
    {WHITE, WHITE, WHITE, WHITE},
};

static Uint8 staffroll_widths[128] = {
#include "advertise/font/staffroll_widths.inc"
};
static NJS_TEXNAME staffroll_textures[1] = {{TEXT_2, 0, 0}};
static NJS_TEXLIST staffroll_texlist = {
#include "advertise/font/staffroll_texlist.inc"
};
struct advertise_font staffroll_font = {
    32.0F,
    32.0F,
    512.0F,
    256.0F,
    &staffroll_texlist,
    staffroll_widths,
    {WHITE, WHITE, WHITE, WHITE},
};

/* the font advertise_font_load last loaded, which nothing reads */
static struct advertise_font *advertise_font_loaded;
/* the font in use: advertise_font_use's copy, which the set_color functions
 * recolor and the draws read */
static struct advertise_font advertise_font_current;

void advertise_font_load(struct advertise_font *font)
{
    if (font) {
        advertise_font_loaded = font;
        if (font->texlist) {
            njLoadTexture(font->texlist);
        }
    }
}

void advertise_font_release(struct advertise_font *font)
{
    if (font->texlist) {
        njReleaseTexture(font->texlist);
    }
}

void advertise_font_init(struct advertise_font *font, NJS_TEXLIST *texlist,
                         Uint8 *widths, Float cell_width, Float cell_height)
{
    Uint32 width;
    Uint32 height;

    if (font) {
        font->texlist = texlist;
        font->widths = widths;
        font->cell_width = cell_width;
        font->cell_height = cell_height;
        /* @hack spelling: the four colors set in one chained assignment. SHC
         * then stores them through R4, from colors[3] down (MOV.L R3,@(12,R4)
         * at 0x8c90b1e0). as four assignments, SHC stores each through R14, the
         * function runs 4 bytes short, the functions after it move, and 32 of
         * 41 words differ */
        font->colors[0] = font->colors[1] = font->colors[2] = font->colors[3] =
            WHITE;
        screen_texture_size(texlist, 0, &width, &height);
        /* @hack type: the sizes cast to Sint32, which SHC converts as signed
         * (FLOAT at 0x8c90b1f4 and 0x8c90b1fe). from the Uint32s, SHC adds a
         * test of each sign and a correction, the functions after it move 32
         * bytes, and 15 of 41 words differ */
        font->texture_width = (Sint32)width;
        font->texture_height = (Sint32)height;
    }
}

void advertise_font_use(struct advertise_font *font)
{
    advertise_font_current = *font;
}

void advertise_font_set_color(Uint32 color)
{
    advertise_font_current.colors[0] = color;
    advertise_font_current.colors[1] = color;
    advertise_font_current.colors[2] = color;
    advertise_font_current.colors[3] = color;
}

void advertise_font_set_colors(Uint32 top_left, Uint32 bottom_left,
                               Uint32 top_right, Uint32 bottom_right)
{
    advertise_font_current.colors[0] = top_left;
    advertise_font_current.colors[1] = bottom_left;
    advertise_font_current.colors[2] = top_right;
    advertise_font_current.colors[3] = bottom_right;
}

int advertise_font_draw_char(char character, Float x, Float y, Float z)
{
    struct advertise_font *font = &advertise_font_current;
    NJS_TEXTURE_VTX vertices[4];
    NJS_TEXTURE_VTX *vertex;
    Float u0;
    Float u1;
    Float v0;
    Float v1;
    Float column;
    Float row;
    Float right;
    Float bottom;
    Float cell_width;
    Float cell_height;
    /* @hack spelling: glyph, a copy of character, which SHC keeps in R12 (MOV
     * R4,R12 at 0x8c90b28c). without it, SHC keeps character in the frame,
     * which takes 112 bytes, the function runs 2 bytes short, the functions
     * after it move, and 92 of 139 words differ */
    char glyph = character;
    Uint32 texture;

    /* @hack redundant: bottom's address taken, which keeps bottom in the frame
     * at sp+4 (FMOV.S FR3,@(R0,R15) at 0x8c90b30e, read at 0x8c90b34a).
     * without it, the cell's products take other registers, the function
     * runs 2 bytes short, the functions after it move, and 103 of 139 words
     * differ */
    (void)&bottom;
    /* @hack redundant: texture's address taken, which keeps texture in the
     * frame at sp+0 (MOV.L R0,@R15 at 0x8c90b36a). without it, the frame takes
     * 112 bytes, the function runs 2 bytes longer, the functions after it move,
     * and 113 of 139 words differ */
    (void)&texture;
    z = 1.0F / njAbs(z);
    column = glyph & (COLUMNS - 1);
    /* NOLINTNEXTLINE(bugprone-integer-division) */
    row = glyph / COLUMNS;
    /* @hack spelling: cell_width assigned within u0's product. SHC then reads
     * texture_width ahead of it (FMOV.S @(R0,R14),FR11 at 0x8c90b2b6).
     * assigned first, cell_width is read first, into FR9, the products take
     * other registers, the function runs 2 bytes longer, the functions after
     * it move, and 91 of 139 words differ */
    u0 = column * (cell_width = font->cell_width) / font->texture_width;
    u1 = (column + 1.0F) * cell_width / font->texture_width;
    /* @hack redundant: a store of 1.0F to u1 that never runs, which weighs u1
     * in SHC's choice of registers. without it, the frame takes 112 bytes
     * (0x8c90b288), the function runs 4 bytes longer, the functions after it
     * move, and 113 of 139 words differ */
    if (0) {
        u1 = 1.0F;
    }
    /* @hack redundant: a second such store to u1, of 2.0F. without it, the
     * frame takes 112 bytes (0x8c90b288), the function runs 6 bytes longer,
     * the functions after it move, and 113 of 139 words differ */
    if (0) {
        u1 = 2.0F;
    }
    /* @hack redundant: a test of row that never runs, which weighs row in
     * SHC's choice of registers. without it, the cell's products load into
     * other registers (0x8c90b2b6 to 0x8c90b306), and 29 of 139 words differ */
    if (0) {
        if (row) {
        }
    }
    /* @hack spelling: cell_height assigned within v0's product. SHC then reads
     * it third of the four sizes, into FR0 (FMOV.S @(R0,R14),FR0 at
     * 0x8c90b2c0). assigned first, SHC reads the sizes in another order, into
     * other registers, and 32 of 139 words differ */
    v0 = row * (cell_height = font->cell_height) / font->texture_height;
    v1 = (row + 1.0F) * cell_height / font->texture_height;
    /* @hack redundant: a store of 5.0F to v1 that never runs, which weighs v1
     * in SHC's choice of registers. without it, the frame takes 112 bytes
     * (0x8c90b288), the function runs 6 bytes longer, the functions after it
     * move, and 113 of 139 words differ */
    if (0) {
        v1 = 5.0F;
    }
    /* @hack redundant: a second such store to v1, of 6.0F. without it, the
     * frame takes 112 bytes (0x8c90b288), the function runs 6 bytes longer,
     * the functions after it move, and 113 of 139 words differ */
    if (0) {
        v1 = 6.0F;
    }
    vertex = &vertices[0];
    vertex->x = x;
    vertex->y = y;
    vertex->z = z;
    vertex->u = u0;
    vertex->v = v0;
    vertex->col = font->colors[0];
    right = x + cell_width;
    vertex = &vertices[1];
    vertex->x = x;
    vertex->y = bottom = y + cell_height;
    vertex->z = z;
    vertex->u = u0;
    vertex->v = v1;
    vertex->col = font->colors[1];
    vertex = &vertices[2];
    vertex->x = right;
    vertex->y = y;
    vertex->z = z;
    vertex->u = u1;
    vertex->v = v0;
    vertex->col = font->colors[2];
    vertex = &vertices[3];
    vertex->x = right;
    vertex->y = bottom;
    vertex->z = z;
    vertex->u = u1;
    vertex->v = v1;
    vertex->col = font->colors[3];
    texture = screen_texture_index(font->texlist, 0);
    njDrawTexture(vertices, 4, texture, 1);
    if (font->widths) {
        return font->widths[(int)glyph] + 2;
    }
    return (int)font->cell_width;
}

void advertise_font_draw_string(const char *text, Float x, Float y, Float z)
{
    struct advertise_font *font = &advertise_font_current;
    const char *cursor = text;
    int width;
    int inset;
    char character;

    while ((character = *cursor)) {
        width = font->widths[(int)character];
        if (width > 0) {
            inset = (font->cell_width - width) * 0.5F;
            /* @hack spelling: the character read again through cursor. SHC then
             * loads it in the BSR's slot (MOV.B @R13,R4 at 0x8c90b3ea).
             * passed as character, it goes in R4 as the loop loaded it, the
             * function runs 2 bytes short, the functions after it move, and
             * 36 of 63 words differ */
            advertise_font_draw_char(*cursor, x - inset, y, z);
            x += width + 1;
        } else {
            x += BLANK_ADVANCE;
        }
        cursor++;
    }
}

int advertise_font_string_width(const char *text)
{
    struct advertise_font *font = &advertise_font_current;
    const char *cursor = text;
    int total = 0;
    int width;
    char character;

    while ((character = *cursor)) {
        width = font->widths[(int)character];
        if (width > 0) {
            total += width + 1;
        } else {
            total += BLANK_ADVANCE;
        }
        cursor++;
    }
    return total;
}
