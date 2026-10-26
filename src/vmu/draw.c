/* drawing on a vmu screen's pixels, and writing them to its lcd */
#include <sg_lcd.h>
#include <sg_xpt.h>

#include "vmu/draw.h"
#include "vmu/screen.h"

#define PIXELS (VMU_WIDTH * VMU_HEIGHT)
#define BITMAP_BYTES (PIXELS / 8)

/* the index of x, y in a screen's pixels, or an offset of x, y from a pixel */
#define AT(x, y) (((y) * VMU_WIDTH) + (x))

/* the byte for a lit pixel, though any byte but 0 shows lit */
#define LIT 15

/* the column and row of the Chao's top left pixel, and its index */
#define CHAO_LEFT ((VMU_WIDTH - VMU_CHAO_SIDE) / 2)
#define CHAO_TOP ((VMU_HEIGHT - VMU_CHAO_SIDE) / 2)
#define CHAO_AT AT(CHAO_LEFT, CHAO_TOP)

#define TILE (VMU_CHAO_SIDE / PUZZLE_SIDE)

/* the most tally marks, and those to a column */
#define TALLY_MAX 30
#define TALLY_COLUMN 10

/* bits of screen->blink. while BLINK_BOX is set, the cursor's box draws a
 * pixel closer to the cell. while BLINK_TALLY is set, the last tally mark
 * is hidden */
#define BLINK_BOX 0x8
#define BLINK_TALLY 0x10

void vmu_clear(Uint8 *pixels)
{
    Uint8 *pixel;
    Uint8 *end;

    for (pixel = pixels, end = pixel + PIXELS; pixel < end; pixel++) {
        *pixel = 0;
    }
}

/* copy the size block of picture at from to the screen at to. picture's
 * rows are VMU_WIDTH bytes. pixels that land off the screen are dropped, and
 * the source isn't clipped. invert's bit 0 copies each byte complemented,
 * and both callers pass 0 */
static void vmu_blit(struct vmu_screen *screen, Uint8 *picture,
                     struct vmu_point *from, struct vmu_point *size,
                     struct vmu_point *to, int invert)
{
    Sint16 sx;
    Sint16 sy;
    Sint16 dx;
    Sint16 dy;
    Uint8 *out;
    Uint8 *in;

    for (dy = to->y, sy = from->y; sy < from->y + size->y; sy++, dy++) {
        for (dx = to->x, sx = from->x; sx < from->x + size->x; sx++, dx++) {
            if (dx >= 0 && dx < VMU_WIDTH && dy >= 0 && dy < VMU_HEIGHT) {
                /* @hack type: both sums through one Sint16 offset, which
                 * keeps the two row offsets in the frame as words (MOV.W at
                 * 0x8c116f00 and 0x8c116f02) and extends each sum (EXTS.W at
                 * 0x8c116f34 and 0x8c116f3a); added in place, the row
                 * offsets take longs and 38 of 101 words differ */
                Sint16 offset;

                offset = AT(sx, sy);
                in = picture + offset;
                offset = AT(dx, dy);
                out = screen->pixels[0] + offset;
                if (invert & 1) {
                    *out = ~*in;
                } else {
                    *out = *in;
                }
            }
        }
    }
}

static void vmu_copy(Uint8 *from, Uint8 *to, int count)
{
    Uint8 *end = from + count;

    /* @hack spelling: each pointer steps as a statement of its own, which moves
     * the byte through R2 (MOV.B @R4+,R2 at 0x8c116fc0); as *to++ = *from++,
     * it goes through R3 and 2 of 11 words differ */
    while (from < end) {
        *to = *from;
        from++;
        to++;
    }
}

void vmu_shift(struct vmu_screen *screen, struct vmu_point *by)
{
    Uint8 picture[PIXELS];
    struct vmu_point from;
    struct vmu_point size;

    vmu_copy(screen->pixels[0], picture, PIXELS);
    vmu_clear(screen->pixels[0]);
    from.x = from.y = 0;
    size.x = VMU_WIDTH;
    size.y = VMU_HEIGHT;
    vmu_blit(screen, picture, &from, &size, by, 0);
}

void vmu_pixelate(struct vmu_screen *screen, struct vmu_point *size)
{
    Uint8 picture[PIXELS];
    int bx;
    int by;
    int x;
    int y;
    int i;
    int j;
    Uint32 count;
    Uint32 lit;

    if (size->x < 1 || size->x > VMU_WIDTH || size->y < 1 ||
        size->y > VMU_HEIGHT || (size->x == 1 && size->y == 1)) {
        return;
    }
    vmu_copy(screen->pixels[0], picture, PIXELS);
    vmu_clear(screen->pixels[0]);
    for (by = 0; by < VMU_HEIGHT; by += size->y) {
        for (bx = 0; bx < VMU_WIDTH; bx += size->x) {
            count = 0;
            lit = 0;
            for (j = 0, y = by; j < size->y && y < VMU_HEIGHT; j++, y++) {
                for (i = 0, x = bx; i < size->x && x < VMU_WIDTH; i++, x++) {
                    if (picture[AT(x, y)]) {
                        lit++;
                    }
                    count++;
                }
            }
            if (lit >= count / 2) {
                for (j = 0, y = by; j < size->y && y < VMU_HEIGHT; j++, y++) {
                    for (i = 0, x = bx; i < size->x && x < VMU_WIDTH;
                         i++, x++) {
                        /* @hack spelling: the pixels indexed as one row, which
                         * adds x to the row's offset unextended (ADD R5,R3 at
                         * 0x8c117142); as pixels[y][x], SHC extends the
                         * offset with EXTS.W and 23 of 171 words differ */
                        (*screen->pixels)[AT(x, y)] = 0xff;
                    }
                }
            }
        }
    }
}

/* unpack count pixels from bitmap, msb first, as LIT or 0 */
static void vmu_unpack(const Uint8 *bitmap, Uint8 *pixels, int count)
{
    Uint32 mask = 0x80;

    while (count > 0) {
        if (*bitmap & mask) {
            *pixels = LIT;
        } else {
            *pixels = 0;
        }
        mask >>= 1;
        pixels++;
        if (mask == 0) {
            bitmap++;
            mask = 0x80;
        }
        count--;
    }
}

void vmu_draw_picture(struct vmu_screen *screen)
{
    Uint8 picture[PIXELS];
    struct vmu_point from;
    struct vmu_point size;
    struct vmu_point to;

    if (vmu_screen_due(screen)) {
        vmu_unpack(screen->anim[screen->frame].bitmap, picture, PIXELS);
        from.x = from.y = 0;
        size.x = VMU_WIDTH;
        size.y = VMU_HEIGHT;
        to.x = to.y = 0;
        vmu_blit(screen, picture, &from, &size, &to, 0);
    }
}

void vmu_draw_chao(struct vmu_screen *screen)
{
    Uint8 picture[VMU_CHAO_SIDE * VMU_CHAO_SIDE];
    Uint8 *out;
    Uint8 *in;
    int x;
    int y;

    if (vmu_screen_due(screen)) {
        vmu_unpack(screen->anim[screen->frame].bitmap, picture,
                   VMU_CHAO_SIDE * VMU_CHAO_SIDE);
        out = &screen->pixels[CHAO_TOP][CHAO_LEFT];
        in = picture;
        for (y = 0; y < VMU_CHAO_SIDE; y++) {
            for (x = 0; x < VMU_CHAO_SIDE; x++) {
                *out++ = *in++;
            }
            out += VMU_WIDTH - VMU_CHAO_SIDE;
        }
    }
}

void vmu_step_anim(struct vmu_screen *screen)
{
    const struct vmu_frame *frame;

    if (vmu_screen_due(screen)) {
        frame = &screen->anim[screen->frame];
        screen->timer++;
        if (screen->timer >= frame->updates) {
            screen->timer = 0;
            screen->frame++;
            frame++;
            if (frame->bitmap == NULL) {
                screen->frame = 0;
            }
        }
    }
}

void vmu_set_anim(struct vmu_screen *screen, const struct vmu_frame *anim)
{
    screen->anim = anim;
    screen->frame = 0;
    screen->timer = 0;
}

void vmu_draw_puzzle(struct vmu_screen *screen)
{
    Uint8 picture[PIXELS];
    Uint8 *top;
    Uint8 *bottom;
    Uint8 *left;
    Uint8 *right;
    Uint8 *in;
    Uint8 *out;
    Uint8 *cell;
    Uint8 *edge1;
    Uint8 *edge2;
    Sint8 *tile;
    int row;
    int col;
    int x;
    int y;
    int i;
    int length;
    int which;

    vmu_copy(screen->pixels[0], picture, PIXELS);
    left = top = &picture[CHAO_AT];
    bottom = top + AT(0, VMU_CHAO_SIDE - 1);
    right = left + VMU_CHAO_SIDE - 1;
    for (i = 0; i < VMU_CHAO_SIDE;
         i++, top++, bottom++, left += VMU_WIDTH, right += VMU_WIDTH) {
        *top = LIT;
        *bottom = LIT;
        *left = LIT;
        *right = LIT;
    }
    vmu_clear(screen->pixels[0]);
    tile = screen->tiles;
    for (row = 0; row < PUZZLE_SIDE; row++) {
        for (col = 0; col < PUZZLE_SIDE; col++) {
            if (screen->blank != *tile) {
                /* NOLINTNEXTLINE(bugprone-signed-char-misuse) */
                which = *tile;
                /* @hack spelling: this address and the three cell addresses
                 * below each sum the base, the column's offset, the row's
                 * and CHAO_AT in that order. SHC then adds CHAO_AT, in R10,
                 * last (ADD R10,R3 at 0x8c11742a, ADD R10,R5 at 0x8c117476).
                 * with all four summed as the base, CHAO_AT, the row's and
                 * the column's, 126 of 286 words differ */
                in = picture + ((which % PUZZLE_SIDE) * TILE) +
                     ((which / PUZZLE_SIDE) * TILE * VMU_WIDTH) + CHAO_AT;
                out = screen->pixels[0] + (col * TILE) +
                      (row * TILE * VMU_WIDTH) + CHAO_AT;
                for (y = 0; y < TILE;
                     y++, in += VMU_WIDTH - TILE, out += VMU_WIDTH - TILE) {
                    for (x = 0; x < TILE; x++, out++, in++) {
                        *out = *in;
                    }
                }
            } else {
                out = screen->pixels[0] + (col * TILE) +
                      (row * TILE * VMU_WIDTH) + CHAO_AT;
                for (y = 0; y < TILE; y++, out += VMU_WIDTH - TILE) {
                    for (x = 0; x < TILE; x++, out++) {
                        if ((x + y) & 1) {
                            *out = LIT;
                        } else {
                            *out = 0;
                        }
                    }
                }
            }
            tile++;
        }
    }
    /* NOLINTNEXTLINE(bugprone-signed-char-misuse) */
    which = screen->cursor;
    col = which % PUZZLE_SIDE;
    row = which / PUZZLE_SIDE;
    cell =
        screen->pixels[0] + (col * TILE) + (row * TILE * VMU_WIDTH) + CHAO_AT;
    if (screen->blink & BLINK_BOX) {
        edge1 = cell + AT(-1, -1);
        edge2 = cell + AT(-1, TILE);
        length = TILE + 2;
    } else {
        edge1 = cell + AT(-2, -2);
        edge2 = cell + AT(-2, TILE + 1);
        length = TILE + 4;
    }
    for (i = 0; i < length; i++) {
        *edge1 = LIT;
        edge1++;
        *edge2 = LIT;
        edge2++;
    }
    if (screen->blink & BLINK_BOX) {
        edge1 = cell + AT(-1, 0);
        edge2 = cell + AT(TILE, 0);
        length = TILE;
    } else {
        edge1 = cell + AT(-2, -1);
        edge2 = cell + AT(TILE + 1, -1);
        length = TILE + 2;
    }
    for (i = 0; i < length; i++) {
        *edge1 = LIT;
        edge1 += VMU_WIDTH;
        *edge2 = LIT;
        edge2 += VMU_WIDTH;
    }
}

void vmu_draw_tally(struct vmu_screen *screen)
{
    Uint8 *mark;
    int i;

    for (i = 1, mark = screen->pixels[VMU_HEIGHT - 2];
         i <= TALLY_MAX && i <= screen->puzzles; i++, mark -= 3 * VMU_WIDTH) {
        if (!(screen->blink & BLINK_TALLY) || i != screen->puzzles) {
            mark[0] = LIT;
            mark[VMU_WIDTH] = LIT;
            if (i % 5 == 0) {
                mark[1] = LIT;
                mark[VMU_WIDTH + 1] = LIT;
            }
        }
        if (i % TALLY_COLUMN == 0) {
            mark += AT(3, TALLY_COLUMN * 3);
        }
    }
}

/* pack the pixels into bitmap, a bit to each, msb first from the top left.
 * a pixel is lit when not 0 */
static void vmu_pack(const Uint8 *pixels, Uint8 *bitmap)
{
    Uint32 mask = 0x80;
    const Uint8 *end = pixels + PIXELS;

    while (pixels < end) {
        if (mask == 0x80) {
            *bitmap = 0;
        }
        if (*pixels != 0) {
            *bitmap |= mask;
        }
        mask >>= 1;
        if (mask == 0) {
            bitmap++;
            mask = 0x80;
        }
        pixels++;
    }
}

/* as vmu_pack, rotated half a turn: the bitmap fills from its last bit back */
static void vmu_pack_turned(const Uint8 *pixels, Uint8 *bitmap)
{
    const Uint8 *end = pixels + PIXELS;
    Uint32 mask = 1;

    bitmap += BITMAP_BYTES - 1;
    while (pixels < end) {
        if (mask == 1) {
            *bitmap = 0;
        }
        if (*pixels != 0) {
            *bitmap |= mask;
        }
        mask <<= 1;
        if (mask == 0x100) {
            bitmap--;
            mask = 1;
        }
        pixels++;
    }
}

void vmu_write(struct vmu_screen *screen)
{
    Uint8 bitmap[BITMAP_BYTES];

    if (vmu_screen_due(screen) && screen->ready == 1) {
        if (screen->direction == PDD_LCD_DIRECTION_FLIP) {
            vmu_pack_turned(screen->pixels[0], bitmap);
        } else {
            vmu_pack(screen->pixels[0], bitmap);
        }
        /* @hack redundant: an empty test of the write's result, which keeps
         * the image's TST R0,R0 (0x8c1177c0); without it, 9 of 39 words
         * differ */
        if (pdVmsLcdWrite1(screen->lcd, bitmap)) {
        }
    }
}
