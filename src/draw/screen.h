/* textures drawn flat on the screen through one quad, and the pictures,
 * boxes and bands built on it. also a cover over the whole screen and a
 * texture viewer */
#ifndef DRAW_SCREEN_H
#define DRAW_SCREEN_H
#include <ninja.h>
#include <sg_xpt.h>

struct bitmap;
struct task;

/* the texture number screen_quad_draw, screen_quad_draw_sized and
 * screen_quad_draw_scaled take. QUAD_TEXTURE holds an index into the current
 * texlist, in bits 0 to 23 and 28 and 29. QUAD_PLACE holds a place, and
 * QUAD_FLIP a flip */
#define QUAD_TEXTURE 0x30ffffff
#define QUAD_PLACE 0x0f000000
#define QUAD_FLIP 0xc0000000
/* the places: where x and y lie on the drawn texture. they run from its top
 * left corner through the middle of its top edge, row by row, to its bottom
 * right corner. the place must be one of these nine */
#define QUAD_TOP_LEFT 0x00000000
#define QUAD_TOP 0x01000000
#define QUAD_TOP_RIGHT 0x02000000
#define QUAD_LEFT 0x03000000
#define QUAD_CENTERED 0x04000000
#define QUAD_RIGHT 0x05000000
#define QUAD_BOTTOM_LEFT 0x06000000
#define QUAD_BOTTOM 0x07000000
#define QUAD_BOTTOM_RIGHT 0x08000000
/* QUAD_FLIP_V flips the texture top to bottom, and QUAD_FLIP_U left to
 * right */
#define QUAD_FLIP_V 0x40000000
#define QUAD_FLIP_U 0x80000000

/* a part of a picture: a texture of the current texlist, and where its top
 * left lies from the picture's x and y */
struct screen_part {
    Sint16 texture;
    Sint16 x;
    Sint16 y;
};

/* a picture of count parts, which run on past the one parts declares */
struct screen_parts {
    Sint16 count;
    struct screen_part parts[1];
};

/* where screen_viewer_create's task draws the texture's top left, at x and
 * y */
struct screen_viewer_place {
    Sint8 player; /* whose pad pages through the textures */
    Float x;
    Float y;
};

/* the functions below draw through one quad: the four corners a texture
 * draws through, its top left, bottom left, top right and bottom right.
 * each corner keeps a color, and the corners keep the u and v the last
 * draw's flip set. the tint multiplies each color the corners take. the
 * quad draws translucent where its last translucency setting says so, or
 * where the texture's pixel format holds alpha. a draw's z is a depth: each
 * corner takes 1/z */

/* reset the quad: its corners white with the texture unflipped, its tint
 * white, and the quad opaque. it also hides the cover and makes its color 0 */
void screen_quad_init(void);
/* the tint */
Uint32 screen_quad_get_tint(void);
/* screen_quad_color gives every corner color times the tint, and
 * screen_quad_colors gives each corner in order its own. the quad is then
 * translucent unless every corner's alpha is 0xff */
void screen_quad_color(Uint32 color);
void screen_quad_colors(Uint32 corner0, Uint32 corner1, Uint32 corner2,
                        Uint32 corner3);
/* the first corner's color. colors, unless null, takes all four */
Uint32 screen_quad_get_colors(Uint32 *colors);
/* set the tint, and multiply each corner's color by it. a color keeps any
 * tint it took before. the quad is then translucent unless every corner's
 * alpha is 0xff */
void screen_quad_tint(Uint32 tint);
/* set the quad translucent or opaque, whatever its corners' alpha, until a
 * color or tint call sets it again */
void screen_quad_set_translucent(Bool translucent);
/* draw texture, a texture number, with its place at x and y and its flip.
 * the three draw it at its size, at width by height, or at its size times
 * scale_x and scale_y */
void screen_quad_draw(int texture, Float x, Float y, Float z);
void screen_quad_draw_sized(int texture, Float x, Float y, Float z, Float width,
                            Float height);
void screen_quad_draw_scaled(int texture, Float x, Float y, Float z,
                             Float scale_x, Float scale_y);
/* draw the current texlist's texture through the current matrix, from
 * (0, 0) to its width and height. texture is an index alone, with no place
 * or flip. the depth is the z of matrix's translation. the corners keep the u
 * and v the last draw left */
void screen_quad_draw_matrix(int texture, NJS_MATRIX *matrix);
/* draw each part of picture with screen_quad_draw, at x and y plus the
 * part's own */
void screen_parts_draw(const struct screen_parts *picture, Float x, Float y,
                       Float z);
/* screen_blend_alpha sets Ninja's source blend to
 * NJD_COLOR_BLENDING_SRCALPHA and its destination blend to INVSRCALPHA.
 * screen_blend sets them to source and destination */
void screen_blend_alpha(void);
void screen_blend(int source, int destination);
/* create a task that draws one of texlist's textures at place's x and y
 * each step. it prints the texture's number, global index, size, pixel
 * format and surface flags below it. left and right on place's player's pad
 * page through the textures. a null place draws at 0, 0 for player 0.
 * texlist must not be null. returns the task, or null when the task pool or
 * the heap runs out */
struct task *screen_viewer_create(NJS_TEXLIST *texlist,
                                  struct screen_viewer_place *place);
/* store texture's width and height in pixels, through each pointer that
 * isn't null. texture indexes texlist, or for a null texlist the current
 * one */
void screen_texture_size(NJS_TEXLIST *texlist, int texture, Uint32 *width,
                         Uint32 *height);
/* texture's global index, in texlist or for a null texlist the current one */
Uint32 screen_texture_index(NJS_TEXLIST *texlist, int texture);
/* draw bitmap's texture at its x and y, 0.66666 times its texture's width
 * and height. it draws translucent, in its text_color, at depth z, and makes
 * the bitmap's texlist current. a null bitmap, or one with no texture, draws
 * nothing */
void screen_bitmap_draw(struct bitmap *bitmap, Float z);
/* the cover is a polygon over the whole screen. set its color, with its
 * alpha made 0xff, and show or hide it */
void screen_cover_set_color(Uint32 color);
void screen_cover_show(Bool shown);
/* draw the cover, while it is shown, at alpha from 0 to 1. an alpha of 1 or
 * more draws it opaque */
void screen_cover_draw(Float alpha);
/* set Ninja's view to the task's: at its work's pos, turned by its work's
 * ang about z, then x, then y */
void screen_view_from_task(struct task *task);
/* screen_palette_grays fills the base colors of draw_palettes[palette]
 * with opaque grays, and screen_palettes_grays those of all eight palettes.
 * entry 0 is white, and each entry after it a level darker, to black at
 * entry 255 */
void screen_palette_grays(int palette);
void screen_palettes_grays(void);
/* adv_window's corner and fill textures, which must be loaded before
 * screen_box_draw */
extern NJS_TEXLIST screen_box_texlist;
/* draw a box of screen_box_texlist's textures, its top left at x and y,
 * width by height, at depth z. its color runs from the quad's top left color
 * at its top to its bottom left color at its bottom. the tint, which those
 * colors hold already, multiplies them again. the box changes the quad's
 * colors, makes screen_box_texlist current and clamps textures */
void screen_box_draw(Float x, Float y, Float z, Float width, Float height);
/* draw a green band, its top left at x and y, width by height, at depth z.
 * its color is 0xff007014 at its left edge and 0x00007014 at its right, each
 * times the tint. it blends by NJD_COLOR_BLENDING_SRCCOLOR and INVDESTCOLOR,
 * then leaves the blend as screen_blend_alpha sets it */
void screen_band_draw(Float x, Float y, Float z, Float width, Float height);

#endif
