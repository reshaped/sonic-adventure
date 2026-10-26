/* util.h's helpers, and bitmap.h's text windows */
#include <mathf.h>
#include <ninja.h>
#include <sg_xpt.h>
#include <shinobi.h>
#include <stdarg.h>
#include <string.h>

#include "camera/camera.h"
#include "collision/polygon.h"
#include "collision/world.h"
#include "draw/pose.h"
#include "hud/hud.h"
#include "main.h"
#include "player/player.h"
#include "stage/stage.h"
#include "system/bitmap.h"
#include "system/pad.h"
#include "system/task.h"
#include "system/thread.h"
#include "system/util.h"

/* a glyph is 24 dots square, three bytes a row, with the leftmost dot in
 * the first byte's lowest bit */
#define GLYPH_SIZE 24
#define GLYPH_ROW (GLYPH_SIZE / 8)
#define GLYPH_WORDS (GLYPH_SIZE * GLYPH_SIZE / 32)

/* each Shift JIS lead byte from 0x81 starts a row of WIDE_ROW codes, whose
 * trail bytes run 0x40 to 0xfc less 0x7f. font_wide follows the rows, with
 * gaps. from WIDE_KANA, the kana at 0x829f, a glyph's index is the code's
 * place in the rows less WIDE_KANA_SKIP. from WIDE_KANJI, the kanji at
 * 0x889f, it is less WIDE_KANJI_SKIP, which counts both gaps */
#define WIDE_ROW 188
#define WIDE_KANA 0x11a
#define WIDE_KANA_SKIP 4
#define WIDE_KANJI 0x582
#define WIDE_KANJI_SKIP 0x3b8

/* two-byte characters through the last JIS level 1 kanji, and one-byte ones
 * from a space through 0x7f */
#define WIDE_GLYPHS 3423
#define ASCII_GLYPHS 96

#define INK_WHITE 0xffff

#define QUEUE_SIZE 8

/* the dots bitmap_alloc and bitmap_init give a bitmap between characters
 * and between lines */
#define LETTER_SPACING 2
#define LINE_SPACING 4

/* the message and choice windows' places and sizes on the screen, in dots,
 * and the global indexes their textures load under */
#define WINDOW_X 70
#define WINDOW_WIDTH 500
#define MESSAGE_Y 384
#define MESSAGE_HEIGHT 52
#define MESSAGE_TEXTURE 100
#define CHOICE_Y 128
#define CHOICE_LINE (GLYPH_SIZE + LINE_SPACING)
#define CHOICE_TEXTURE 101

#define PAGE_STEPS 120

#define BITMAP_TEXFMT (NJD_TEXFMT_RECTANGLE | NJD_TEXFMT_ARGB_1555)
#define BITMAP_TEXATTR (NJD_TEXATTR_TYPE_MEMORY | NJD_TEXATTR_GLOBALINDEX)

#define TREE_NODES 32

/* a choice window, kept in its thread's data */
struct choice {
    int count; /* lines to choose from */
    Uint8 unused[4];
    struct bitmap bitmap;
};

extern Uint16 message_window_memory[];

static void bitmap_print_centered(struct bitmap *bitmap, const char *text);
static void bitmap_print_block(struct bitmap *bitmap, const char *text);
static NJS_OBJECT *tree_find_node(NJS_OBJECT *node, NJS_OBJECT *object,
                                  int *count);
static int tree_collect_nodes(NJS_OBJECT **nodes, int *count);

static const Uint32 font_wide[WIDE_GLYPHS * GLYPH_WORDS] = {
#include "system/util/font_wide.inc"
};

static const Uint32 font_ascii[ASCII_GLYPHS * GLYPH_WORDS] = {
#include "system/util/font_ascii.inc"
};

/* each ASCII character's width in dots, less 1 */
static const Sint8 font_widths[ASCII_GLYPHS] = {
#include "system/util/font_widths.inc"
};

Sint8 messages_shown;
static Sint8 pages_active;
static struct bitmap *texture_queue[QUEUE_SIZE];
static int texture_queue_count;
static Uint32 saved_attr_and;
static Uint32 saved_attr_or;
static Uint32 saved_control_3d;
static NJS_MATRIX tree_matrices[TREE_NODES];

static int clamp_byte(int value)
{
    if (value < 0) {
        value = 0;
    }
    if (value > 255) {
        value = 255;
    }
    return value;
}

struct bitmap *bitmap_alloc(struct bitmap *bitmap, int x, int y, int width,
                            int height, int index)
{
    int texture_width;
    int texture_height;
    Uint32 size;
    Uint32 words;
    Sint32 *word;
    Uint16 *pixels;

    texture_width = 8;
    while (texture_width < width) {
        texture_width <<= 1;
    }
    texture_height = 8;
    while (texture_height < height) {
        texture_height <<= 1;
    }
    size = texture_width * texture_height;
    pixels = heap_alloc(size * sizeof(*pixels));
    word = (Sint32 *)pixels;
    for (words = size >> 1; words != 0; words--) {
        *word++ = 0;
    }
    bitmap->mode = BITMAP_PLAIN;
    bitmap->x = x;
    bitmap->y = y;
    bitmap->width = width;
    bitmap->height = height;
    bitmap->texture_width = texture_width;
    bitmap->texture_height = texture_height;
    bitmap->cursor_x = 0;
    bitmap->cursor_y = 0;
    bitmap->letter_spacing = LETTER_SPACING;
    bitmap->line_spacing = LINE_SPACING;
    bitmap->ink = INK_WHITE;
    bitmap->text_color.color = 0xffffffff;
    bitmap->back_color.color = 0;
    bitmap->pixels = pixels;
    bitmap->index = index;
    bitmap->texlist.textures = NULL;
    return bitmap;
}

void bitmap_queue(struct bitmap *bitmap)
{
    bitmap_queue_add(bitmap);
}

void bitmap_free(struct bitmap *bitmap)
{
    if (bitmap->texlist.textures != NULL) {
        njReleaseTexture(&bitmap->texlist);
        bitmap->texlist.textures = NULL;
    }
    if (bitmap->pixels != NULL) {
        heap_free(bitmap->pixels);
        bitmap->pixels = NULL;
    }
}

void bitmap_draw(struct bitmap *bitmap)
{
    Float x;
    Float y;
    Float right;
    Float bottom;

    if (bitmap->mode == BITMAP_EVENT && !messages_shown) {
        return;
    }
    x = bitmap->x;
    y = bitmap->y;
    right = x + bitmap->texture_width;
    bottom = y + bitmap->texture_height;
    njTextureFilterMode(NJD_TEXTUREFILTER_POINT_SAMPLE);
    if (bitmap->back_color.argb.a) {
        NJS_COLOR colors[4];
        NJS_POINT2 points[4];
        NJS_POINT2COL polygon;

        points[0].x = points[1].x = 0.0F;
        points[2].x = points[3].x = SCREEN_WIDTH;
        points[0].y = points[3].y = y - 4.0F;
        points[1].y = points[2].y = y + bitmap->height + 4.0F;
        colors[0].color = colors[1].color = colors[2].color = colors[3].color =
            bitmap->back_color.color;
        polygon.p = points;
        polygon.col = colors;
        polygon.tex = NULL;
        polygon.num = 4;
        njColorBlendingMode(NJD_SOURCE_COLOR, NJD_COLOR_BLENDING_SRCALPHA);
        njColorBlendingMode(NJD_DESTINATION_COLOR,
                            NJD_COLOR_BLENDING_INVSRCALPHA);
        njDrawPolygon2D(&polygon, 4, -1.2F, NJD_FILL | NJD_TRANSPARENT);
    }
    if (bitmap->text_color.argb.a && bitmap->texlist.textures != NULL) {
        NJS_COLOR colors[4];
        NJS_COLOR uv[4] = {
#include "system/util/uv.inc"
        };
        NJS_POINT2 points[4];
        NJS_POINT2COL polygon;

        points[0].x = points[1].x = x;
        points[2].x = points[3].x = right;
        points[0].y = points[3].y = y;
        points[1].y = points[2].y = bottom;
        /* @hack redundant: text_color in each corner, which the tints
         * below replace. the image stores it in three of them first (MOV.L
         * @(24,R14),R0 at 0x8c057d62, then 0x8c057d64 to 0x8c057d6a);
         * without it, the function runs 30 bytes shorter and 106 of 210
         * words differ */
        colors[0].color = colors[1].color = colors[2].color = colors[3].color =
            bitmap->text_color.color;
        /* the text draws in these pale tints, not in text_color. of
         * text_color, only the alpha counts: the text draws while it isn't
         * 0 */
        colors[0].color = 0xeeddddee;
        colors[1].color = 0xeed0d0e0;
        colors[2].color = 0xeeddddee;
        colors[3].color = 0xeed0d0e0;
        polygon.p = points;
        polygon.col = colors;
        polygon.tex = uv;
        polygon.num = 4;
        njSetTexture(&bitmap->texlist);
        njSetTextureNum(0);
        njDrawPolygon2D(&polygon, 4, -1.2F,
                        NJD_USE_TEXTURE | NJD_FILL | NJD_TRANSPARENT);
    }
    njTextureFilterMode(NJD_TEXTUREFILTER_BILINEAR);
}

void bitmap_clear(struct bitmap *bitmap)
{
    Sint32 *word = (Sint32 *)bitmap->pixels;
    int words = bitmap->texture_width * bitmap->texture_height / 2;

    for (; words != 0; words--) {
        *word++ = 0;
    }
    bitmap->cursor_x = bitmap->cursor_y = 0;
}

/* code's glyph, for a byte or a two-byte Shift JIS code. a byte below a
 * space takes the space's glyph. a two-byte code past the last level 1
 * kanji reads past font_wide, unchecked */
static const Uint8 *font_glyph(int code)
{
    int row;
    int index;

    if (code < 0x100) {
        code -= ' ';
        if (code < 0) {
            code = 0;
        }
        return (const Uint8 *)&font_ascii[code * GLYPH_WORDS];
    } else {
        row = (Uint8)(code >> 8);
        row -= 0x81;
        index = (Uint8)code;
        if (index <= 0x7e) {
            index -= 0x40;
        } else {
            index -= 0x41;
        }
        index += row * WIDE_ROW;
        if (index >= WIDE_KANJI) {
            index -= WIDE_KANJI_SKIP;
        } else if (index >= WIDE_KANA) {
            index -= WIDE_KANA_SKIP;
        }
        return (const Uint8 *)&font_wide[index * GLYPH_WORDS];
    }
}

/* code's width in dots: GLYPH_SIZE for a two-byte code. a byte below a
 * space reads before font_widths */
static int font_width(int code)
{
    if (code < 0x100) {
        return font_widths[code - ' '] + 1;
    }
    return GLYPH_SIZE;
}

void bitmap_newline(struct bitmap *bitmap)
{
    bitmap->cursor_x = 0;
    bitmap->cursor_y += bitmap->line_spacing + GLYPH_SIZE;
}

/* draw code at the cursor, and move the cursor past it. a character that
 * would cross the texture's right edge starts a new line first. rows above
 * or below the texture are clipped. of the control characters, only a
 * newline does anything */
static void bitmap_put_char(struct bitmap *bitmap, int code)
{
    const Uint8 *glyph;
    int width;
    int texture_width;
    Uint16 ink[2];
    Uint16 *pixel;
    int y;
    Uint32 rows;
    Uint32 row;
    int i;
    /* @hack type: bits signed, which widens each glyph byte with EXTS.B and
     * shifts it with SHAR (0x8c058034, 0x8c05803c). the sign changes no dot,
     * as the eight shifts only ever bring the byte's own bits to bit 0. as
     * Uint8, EXTU.B and SHLR take their places and 22 of 192 words differ */
    Sint8 bits;

    if (code < ' ') {
        if (code == '\n') {
            bitmap_newline(bitmap);
        }
        return;
    }
    glyph = font_glyph(code);
    width = font_width(code);
    texture_width = bitmap->texture_width;
    ink[0] = 0;
    ink[1] = bitmap->ink;
    if (bitmap->cursor_x > texture_width - width) {
        bitmap_newline(bitmap);
    }
    if (bitmap->cursor_y < bitmap->texture_height &&
        bitmap->cursor_y + GLYPH_SIZE - 1 >= 0) {
        pixel = bitmap->pixels + bitmap->cursor_x +
                (bitmap->cursor_y * texture_width);
        y = bitmap->cursor_y;
        rows = GLYPH_SIZE;
        if (y < 0) {
            rows += y;
            glyph += -y * GLYPH_ROW;
            pixel += -y * texture_width;
        }
        /* @hack spelling: row, a counter apart from rows, which counts
         * down a copy of rows in R9 (MOV R5,R9 at 0x8c058022) and leaves
         * width in R10 (MOV R0,R10 at 0x8c057faa). without it, rows counts
         * down itself, width takes R9, the function runs 2 bytes shorter, and
         * 106 of 192 words differ */
        for (row = rows; row != 0; row--) {
            for (i = 0; i < GLYPH_ROW; i++) {
                bits = *glyph++;
                *pixel++ = ink[bits & 1];
                bits >>= 1;
                *pixel++ = ink[bits & 1];
                bits >>= 1;
                *pixel++ = ink[bits & 1];
                bits >>= 1;
                *pixel++ = ink[bits & 1];
                bits >>= 1;
                *pixel++ = ink[bits & 1];
                bits >>= 1;
                *pixel++ = ink[bits & 1];
                bits >>= 1;
                *pixel++ = ink[bits & 1];
                bits >>= 1;
                *pixel++ = ink[bits & 1];
                bits >>= 1;
            }
            y++;
            pixel += texture_width - GLYPH_SIZE;
            if (y >= bitmap->texture_height) {
                break;
            }
        }
    }
    bitmap->cursor_x += bitmap->letter_spacing + width;
}

void bitmap_print(struct bitmap *bitmap, const char *text)
{
    int code;

    if (*text == '\t') {
        bitmap_print_centered(bitmap, text + 1);
    } else if (*text == '\a') {
        bitmap_print_block(bitmap, text + 1);
    } else {
        while ((code = (Uint8)*text++) != 0) {
            if (code >= 0x80) {
                code = (code << 8) | (Uint8)*text++;
            }
            bitmap_put_char(bitmap, code);
        }
    }
}

/* print text up to and including its first newline */
static void bitmap_print_line(struct bitmap *bitmap, const char *text)
{
    int code;

    while ((code = (Uint8)*text++) != 0) {
        if (code >= 0x80) {
            code = (code << 8) | (Uint8)*text++;
        }
        bitmap_put_char(bitmap, code);
        if (code == '\n') {
            break;
        }
    }
}

/* print text, each line centered on its own in the bitmap's width */
static void bitmap_print_centered(struct bitmap *bitmap, const char *text)
{
    const char *next;
    int width;
    int code;

    while (*text != '\0') {
        next = text;
        width = 0;
        /* @hack spelling: *next tested bare, which loads the byte ahead of
         * letter_spacing (MOV.B @R14,R2 at 0x8c058206) and ends the loop in
         * a BF/S (0x8c05820e). compared with '\0', letter_spacing loads
         * first, the loop ends in a BF, and 6 of 69 words differ */
        while (*next) {
            code = (Uint8)*next++;
            if (code == '\n') {
                break;
            }
            if (code >= 0x80) {
                code = (code << 8) | (Uint8)*next++;
            }
            width += font_width(code) + bitmap->letter_spacing;
        }
        width -= bitmap->letter_spacing;
        if (width < 0) {
            width = 0;
        }
        bitmap->cursor_x = (bitmap->width - width) / 2;
        bitmap_print_line(bitmap, text);
        text = next;
    }
}

/* print text's lines as a block, each from the place that centers the
 * widest */
static void bitmap_print_block(struct bitmap *bitmap, const char *text)
{
    const char *next;
    int code;
    int width;
    int widest;
    int x;

    widest = 0;
    next = text;
    do {
        width = 0;
        while ((code = (Uint8)*next++) != 0 && code != '\n') {
            if (code & 0x80) {
                code = (code << 8) | (Uint8)*next++;
            }
            width += font_width(code) + bitmap->letter_spacing;
        }
        width -= bitmap->letter_spacing;
        if (widest < width) {
            widest = width;
        }
    } while (code != 0);
    x = (bitmap->width - widest) / 2;
    next = text;
    do {
        bitmap->cursor_x = x;
        bitmap_print_line(bitmap, next);
        /* @hack type: the second pass's bytes as plain chars, which leaves
         * each as MOV.B sign-extends it (0x8c058300). read as Uint8, as in
         * the first pass, each takes an EXTU.B, the function runs 2 bytes
         * longer, and 16 of 81 words differ */
        /* NOLINTNEXTLINE(bugprone-signed-char-misuse) */
        while ((code = *next++) != 0 && code != '\n') {
            if (code & 0x80) {
                next++;
            }
        }
    } while (code != 0);
}

void bitmap_printf(struct bitmap *bitmap, const char *format, ...)
{
    char text[128];
    va_list args;

    va_start(args, format);
    string_vformat(text, format, args);
    va_end(args);
    bitmap_print(bitmap, text);
}

void bitmap_fade_in(struct bitmap *bitmap)
{
    int i;

    bitmap->back_color.argb.a = 0;
    bitmap->text_color.argb.a = 0;
    for (i = 10; i != 0; i--) {
        bitmap->back_color.argb.a = clamp_byte(bitmap->back_color.argb.a + 12);
        bitmap_draw(bitmap);
        thread_yield();
    }
    bitmap->back_color.argb.a = 0x80;
    for (i = 10; i != 0; i--) {
        bitmap->text_color.argb.a = clamp_byte(bitmap->text_color.argb.a + 24);
        bitmap_draw(bitmap);
        thread_yield();
    }
    bitmap->text_color.argb.a = 0xff;
}

void bitmap_fade_out(struct bitmap *bitmap)
{
    int i;

    for (i = 10; i != 0; i--) {
        bitmap->text_color.argb.a = clamp_byte(bitmap->text_color.argb.a - 24);
        bitmap_draw(bitmap);
        thread_yield();
    }
    bitmap->text_color.argb.a = 0;
    for (i = 10; i != 0; i--) {
        bitmap->back_color.argb.a = clamp_byte(bitmap->back_color.argb.a - 12);
        bitmap_draw(bitmap);
        thread_yield();
    }
    bitmap->back_color.argb.a = 0;
    bitmap_wait(bitmap, 2);
}

void bitmap_fade_in_text(struct bitmap *bitmap)
{
    int i;

    for (i = 8; i != 0; i--) {
        bitmap->text_color.argb.a = clamp_byte(bitmap->text_color.argb.a + 32);
        bitmap_draw(bitmap);
        thread_yield();
    }
}

void bitmap_fade_out_text(struct bitmap *bitmap)
{
    int i;

    for (i = 8; i != 0; i--) {
        bitmap->text_color.argb.a = clamp_byte(bitmap->text_color.argb.a - 32);
        bitmap_draw(bitmap);
        thread_yield();
    }
    bitmap_wait(bitmap, 2);
}

void bitmap_wait(struct bitmap *bitmap, int steps)
{
    for (; steps != 0; steps--) {
        bitmap_draw(bitmap);
        thread_yield();
    }
}

/* bitmap_wait, cut short by a press of B */
static void bitmap_wait_skip(struct bitmap *bitmap, int steps)
{
    for (; steps != 0; steps--) {
        bitmap_draw(bitmap);
        thread_yield();
        if (per_info[0]->press & PDD_DGT_TB) {
            break;
        }
    }
}

static void choice_destroy(void)
{
    struct choice *choice = thread_get_data(NULL);

    bitmap_free(&choice->bitmap);
}

/* the choice window's thread. up and down move a pulsing bar over the
 * lines, and A or C picks the line under it. the thread then fades the
 * window out, sets its status to the line, counted from 1, and yields until
 * its owner destroys it */
static void choice_run(void)
{
    struct choice *choice;
    struct bitmap *bitmap;
    int chosen;
    int step;
    int top;
    int pulse;
    NJS_COLOR colors[4];
    NJS_POINT2 points[4];
    NJS_POINT2COL polygon;

    choice = thread_get_data(NULL);
    bitmap = &choice->bitmap;
    chosen = 0;
    bitmap_fade_in(bitmap);
    while (!(per_info[0]->press & (PDD_DGT_TA | PDD_DGT_TC))) {
        step = 0;
        if (per_info[0]->press & PDD_DGT_KU) {
            step--;
        }
        if (per_info[0]->press & PDD_DGT_KD) {
            step++;
        }
        if (step != 0) {
            chosen += step;
            if (chosen < 0) {
                chosen = 0;
            } else if (chosen >= choice->count) {
                chosen = choice->count - 1;
            }
        }
        top = bitmap->y + ((bitmap->line_spacing + GLYPH_SIZE) * chosen);
        points[0].x = points[1].x = 0.0F;
        points[2].x = points[3].x = SCREEN_WIDTH - 1.0F;
        points[0].y = points[3].y = (top - 1) + 0.5F;
        points[1].y = points[2].y = (top + GLYPH_SIZE + 1) + 0.5F;
        /* the bar's red rises from 0x80 to 0xff and falls back over each 64
         * steps of the play time */
        pulse = play_time_get();
        pulse = (pulse & 63) << 2;
        if (pulse >= 0x80) {
            pulse = 0xff - pulse;
        }
        colors[0].argb.a = 0x80;
        colors[0].argb.r = pulse + 0x80;
        colors[0].argb.g = 0;
        colors[0].argb.b = 0;
        colors[3] = colors[2] = colors[1] = colors[0];
        polygon.p = points;
        polygon.col = colors;
        polygon.tex = NULL;
        polygon.num = 4;
        njTextureFilterMode(NJD_TEXTUREFILTER_POINT_SAMPLE);
        njDrawPolygon2D(&polygon, 4, -1.2F, NJD_FILL | NJD_TRANSPARENT);
        njTextureFilterMode(NJD_TEXTUREFILTER_BILINEAR);
        bitmap_draw(bitmap);
        thread_yield();
    }
    bitmap_fade_out(bitmap);
    thread_set_status(NULL, chosen + 1);
    for (;;) {
        thread_yield();
    }
}

struct thread *bitmap_choose_lines(int count, const char **lines)
{
    struct thread *thread;
    struct choice *choice;
    int i;

    thread = thread_create_child(NULL, (void *)choice_run, NULL, 0);
    choice = thread_get_data(thread);
    memset(choice, 0, sizeof(*choice));
    bitmap_alloc(&choice->bitmap, WINDOW_X, CHOICE_Y, WINDOW_WIDTH,
                 (count * CHOICE_LINE) - LINE_SPACING, CHOICE_TEXTURE);
    for (i = count; i != 0; i--) {
        bitmap_print_centered(&choice->bitmap, *lines++);
        bitmap_newline(&choice->bitmap);
    }
    bitmap_queue_add(&choice->bitmap);
    /* @hack redundant: choice fetched again, though it holds the pointer
     * already. the fetch makes the image's second call to thread_get_data
     * (JSR at 0x8c0587e6), and count is stored through its result
     * (0x8c0587f0); without it, 18 of 84 words differ */
    choice = thread_get_data(thread);
    choice->count = count;
    thread_set_destructor(thread, choice_destroy);
    return thread;
}

struct thread *bitmap_choose(const char *text)
{
    struct thread *thread;
    struct choice *choice;
    const char *next;
    int count;
    int line;
    int code;

    count = 0;
    thread = thread_create_child(NULL, (void *)choice_run, NULL, 0);
    choice = thread_get_data(thread);
    memset(choice, 0, sizeof(*choice));
    next = text;
    line = 1;
    /* NOLINTNEXTLINE(bugprone-signed-char-misuse) */
    while ((code = *next++) != 0) {
        if (code == '\n') {
            line++;
        } else {
            count = line;
        }
    }
    bitmap_alloc(&choice->bitmap, WINDOW_X, CHOICE_Y, WINDOW_WIDTH,
                 (count * CHOICE_LINE) - LINE_SPACING, CHOICE_TEXTURE);
    bitmap_print_centered(&choice->bitmap, text);
    bitmap_queue_add(&choice->bitmap);
    choice->count = count;
    thread_set_destructor(thread, choice_destroy);
    return thread;
}

void bitmap_alloc_window(struct bitmap *bitmap)
{
    bitmap_alloc(bitmap, WINDOW_X, MESSAGE_Y, WINDOW_WIDTH, MESSAGE_HEIGHT,
                 MESSAGE_TEXTURE);
}

void bitmap_init_window(struct bitmap *bitmap)
{
    bitmap_init(bitmap, WINDOW_X, MESSAGE_Y, WINDOW_WIDTH, MESSAGE_HEIGHT,
                MESSAGE_TEXTURE, message_window_memory);
}

/* the thread that shows pages, to a null, in the message window, each for
 * PAGE_STEPS steps or until B is pressed. it clears pages_active once the
 * window has gone */
static void pages_run(char **pages)
{
    struct bitmap bitmap;
    Bool shown;
    char *text;
    char **page;

    shown = FALSE;
    /* @hack spelling: page, a copy of pages to step, which keeps the
     * pointer in R9 (MOV R4,R9 at 0x8c058936); stepping pages itself, SHC
     * keeps it in the frame and 66 of 75 words differ */
    page = pages;
    /* @hack spelling: the loop entered at next, which has SHC load the window's
     * constants inside it (MOV.W at 0x8c05893c) and keep shown in R13. as a
     * while loop, SHC loads them once ahead of it, shown takes R14, and 34
     * of 75 words differ */
    goto next;
    for (;;) {
        if (!shown) {
            bitmap_init_window(&bitmap);
            /* @hack redundant: a loop of one pass, which has SHC form
             * WINDOW_WIDTH in R13 for the bitmap_init_window it expands
             * above (ADD #116,R13 at 0x8c05894a), and load MESSAGE_Y ahead
             * of message_window_memory. without it, WINDOW_WIDTH goes
             * straight to R7 and 60 of 75 words differ */
            do {
                bitmap_print(&bitmap, text);
                bitmap_queue_add(&bitmap);
            } while (0);
            bitmap_fade_in(&bitmap);
            shown = TRUE;
        } else {
            bitmap_fade_out_text(&bitmap);
            bitmap_clear(&bitmap);
            bitmap_print(&bitmap, text);
            bitmap_queue_add(&bitmap);
            bitmap_fade_in_text(&bitmap);
        }
        bitmap_wait_skip(&bitmap, PAGE_STEPS);
    next:
        text = *page++;
        if (text == NULL) {
            if (shown) {
                bitmap_fade_out(&bitmap);
                bitmap_release(&bitmap);
            }
            break;
        }
    }
    pages_active = 0;
}

void bitmap_show_pages(char **pages)
{
    pages_active = 1;
    /* @hack type: pages_run uncast, which leaves pages in R5 (MOV R4,R5 at
     * 0x8c0589e0). cast to void *, as choice_run is, SHC keeps pages in the
     * frame across the store to pages_active, and 8 of 8 words differ */
    /* NOLINTNEXTLINE(clang-diagnostic-pedantic) */
    thread_create(pages_run, (void *)pages, 0);
}

/* pages_run, but each page shows until B is pressed */
static void pages_run_wait(char **pages)
{
    struct bitmap bitmap;
    Bool shown;
    char *text;
    char **page;

    shown = FALSE;
    /* @hack spelling: page, a copy of pages to step, which keeps the
     * pointer in R9 (MOV R4,R9 at 0x8c058a06); without it, SHC keeps pages
     * in the frame and 69 of 81 words differ */
    page = pages;
    /* @hack spelling: the loop entered at next, which has SHC load the window's
     * constants inside it (MOV.W at 0x8c058a0c). as a while loop, SHC loads
     * them once ahead of it, and 72 of 81 words differ */
    goto next;
    do {
        if (!shown) {
            bitmap_init_window(&bitmap);
            bitmap_print(&bitmap, text);
            bitmap_queue_add(&bitmap);
            bitmap_fade_in(&bitmap);
            shown = TRUE;
        } else {
            bitmap_fade_out_text(&bitmap);
            bitmap_clear(&bitmap);
            bitmap_print(&bitmap, text);
            bitmap_queue_add(&bitmap);
            bitmap_fade_in_text(&bitmap);
        }
        for (;;) {
            bitmap_wait(&bitmap, 1);
            if (per_info[0]->press & PDD_DGT_TB) {
                break;
            }
        }
    next:
        text = *page++;
    } while (text != NULL);
    if (shown) {
        bitmap_fade_out(&bitmap);
        bitmap_release(&bitmap);
    }
    pages_active = 0;
}

void bitmap_show_pages_until_b(char **pages)
{
    pages_active = 1;
    /* @hack type: pages_run_wait passed uncast, which leaves pages in R5
     * (MOV R4,R5 at 0x8c058ab4). cast to void *, it has SHC keep pages in the
     * frame across the store to pages_active, and 8 of 8 words differ */
    /* NOLINTNEXTLINE(clang-diagnostic-pedantic) */
    thread_create(pages_run_wait, (void *)pages, 0);
}

Bool bitmap_pages_active(void)
{
    return pages_active;
}

struct bitmap *bitmap_init(struct bitmap *bitmap, int x, int y, int width,
                           int height, int index, void *memory)
{
    int texture_width;
    int texture_height;
    Uint32 size;
    Uint32 words;
    Sint32 *word;
    Uint16 *pixels;

    texture_width = 8;
    while (texture_width < width) {
        texture_width <<= 1;
    }
    texture_height = 8;
    while (texture_height < height) {
        texture_height <<= 1;
    }
    size = texture_width * texture_height;
    if (((int)memory & 31) == 0) {
        pixels = memory;
    } else {
        pixels = (Uint16 *)(((int)memory + 32) & ~31);
    }
    word = (Sint32 *)pixels;
    for (words = size >> 1; words != 0; words--) {
        *word++ = 0;
    }
    bitmap->mode = BITMAP_PLAIN;
    bitmap->x = x;
    bitmap->y = y;
    bitmap->width = width;
    bitmap->height = height;
    bitmap->texture_width = texture_width;
    bitmap->texture_height = texture_height;
    bitmap->cursor_x = 0;
    bitmap->cursor_y = 0;
    bitmap->letter_spacing = LETTER_SPACING;
    bitmap->line_spacing = LINE_SPACING;
    bitmap->ink = INK_WHITE;
    bitmap->text_color.color = 0xffffffff;
    bitmap->back_color.color = 0;
    bitmap->pixels = pixels;
    bitmap->index = index;
    bitmap->texlist.textures = NULL;
    return bitmap;
}

void bitmap_release(struct bitmap *bitmap)
{
    if (bitmap->texlist.textures != NULL) {
        njReleaseTexture(&bitmap->texlist);
        bitmap->texlist.textures = NULL;
    }
    bitmap->pixels = NULL;
}

void bitmap_clear_queue(void)
{
    texture_queue_count = 0;
}

void bitmap_flush_queue(void)
{
    NJS_TEXINFO info;
    struct bitmap *bitmap;
    int i;

    if (texture_queue_count) {
        bitmap = texture_queue[0];
        /* @bug memory: bitmap steps from the first queued bitmap by one
         * struct bitmap, 60 bytes (ADD #60,R14 at 0x8c058c36), where the
         * queue's next pointer is meant. each pass after the first takes the
         * 60 bytes after the last for a bitmap, loads a texture from what
         * they hold, and stores its texlist into them (0x8c058c26,
         * 0x8c058c28) */
        for (i = 0; i < texture_queue_count; ++i, ++bitmap) {
            if (bitmap->texlist.textures != NULL) {
                njReleaseTexture(&bitmap->texlist);
            }
            njSetTextureInfo(&info, bitmap->pixels, BITMAP_TEXFMT,
                             bitmap->texture_width, bitmap->texture_height);
            njSetTextureName(&bitmap->texname, &info, bitmap->index,
                             BITMAP_TEXATTR);
            bitmap->texlist.textures = &bitmap->texname;
            bitmap->texlist.nbTexture = 1;
            njLoadTexture(&bitmap->texlist);
        }
        texture_queue_count = 0;
    }
}

void bitmap_queue_add(struct bitmap *bitmap)
{
    if (texture_queue_count >= QUEUE_SIZE) {
        return;
    }
    texture_queue[texture_queue_count] = bitmap;
    ++texture_queue_count;
}

Angle angle_approach(Angle from, Angle to, Angle step)
{
    Sint16 difference;

    from = (Uint16)from;
    to = (Uint16)to;
    difference = to - from;
    if (difference <= step && difference >= -step) {
        return to;
    }
    if (difference & 0x8000) {
        to = from - step;
    } else {
        to = from + step;
    }
    return (Uint16)to;
}

/* @hack spelling: angle_difference expanded where angle_unwrap calls it
 * (EXTU.W and SUB at 0x8c058cf2 to 0x8c058cfc); called, angle_unwrap saves
 * R13, R14 and PR, and 30 of 30 words differ */
#pragma inline(angle_difference)

Angle angle_difference(Angle from, Angle to)
{
    Sint16 turn;

    from = (Uint16)from;
    to = (Uint16)to;
    turn = to - from;
    return turn;
}

Angle angle_distance(Angle from, Angle to)
{
    Uint16 distance;

    from = (Uint16)from;
    to = (Uint16)to;
    distance = to - from;
    if ((Sint16)distance < 0) {
        distance = -distance;
    }
    return distance;
}

Angle angle_unwrap(Angle from, Angle to)
{
    Angle difference;
    Angle unwrapped;

    difference = angle_difference(from, to);
    if (difference > 0) {
        unwrapped = to;
        while (from > unwrapped) {
            unwrapped += 0x10000;
        }
    } else if (difference < 0) {
        unwrapped = to;
        while (from < unwrapped) {
            unwrapped -= 0x10000;
        }
    } else {
        unwrapped = from;
    }
    return unwrapped;
}

int vector_angle(NJS_VECTOR *v1, NJS_VECTOR *v2, NJS_VECTOR *normal)
{
    NJS_VECTOR cross;
    int angle;

    angle = (int)(acosf(njInnerProduct(v1, v2)) * ANGLE_PER_RADIAN);
    njOuterProduct(v1, v2, &cross);
    if (normal != NULL && njInnerProduct(normal, &cross) < 0.0F) {
        angle = 0x10000 - angle;
    }
    return angle;
}

Bool draw_on_screen(struct task_work *work)
{
    NJS_POINT2 point;
    Float distance;
    Float margin;
    Float across;

    njPushMatrix(NULL);
    njProjectScreen(NULL, &work->pos, &point);
    njPopMatrix(1);
    distance = njDistanceP2P(&camera_work->pos, &work->pos);
    /* @bug uninitialized: at distance 0, the BT at 0x8c058df4 skips the
     * division that sets margin. the FADD at 0x8c058e02 then reads FR4 as
     * njDistanceP2P left it, holding the squared distance, 0 */
    /* NOLINTNEXTLINE(clang-diagnostic-sometimes-uninitialized) */
    if (distance != 0.0F) {
        margin = 3000.0F / distance;
    }
    across = (point.x + margin) / (SCREEN_WIDTH + (2.0F * margin));
    if (across < 0.0F || across > 1.0F) {
        return FALSE;
    }
    if (SQUARE(SCREEN_MIDDLE_Y - point.y) < SQUARE(margin + SCREEN_MIDDLE_Y)) {
        return TRUE;
    }
    return FALSE;
}

void model_blend(NJS_MODEL *from, NJS_MODEL *to, NJS_MODEL *out, Float blend)
{
    int count;
    NJS_POINT3 *src;
    NJS_POINT3 *toward;
    NJS_POINT3 *dst;

    /* @hack spelling: the arguments through locals, which SHC keeps in a
     * frame of 16 bytes (ADD #-16,R15 at 0x8c058e52); passed straight, it
     * reserves 8, and 21 of 24 words differ */
    count = from->nbPoint;
    src = from->points;
    toward = to->points;
    dst = out->points;
    points_blend_sine(src, toward, dst, blend, count);
    model_calc_face_normals(out);
    model_calc_point_normals(out);
}

void points_blend(NJS_POINT3 *from, NJS_POINT3 *to, NJS_POINT3 *out,
                  Float blend, int count)
{
    Float keep;
    Uint32 floats;
    Float *src;
    Float *dst;
    Float *toward;

    dst = (Float *)out;
    keep = 1.0F - blend;
    src = (Float *)from;
    toward = (Float *)to;
    floats = count * 3;
    for (; floats > 0; floats--) {
        *dst = (keep * *src) + (*toward * blend);
        src++;
        toward++;
        dst++;
    }
}

void points_blend_sine(NJS_POINT3 *from, NJS_POINT3 *to, NJS_POINT3 *out,
                       Float blend, int count)
{
    Float keep;
    Uint32 floats;
    Float *dst;
    Float *src;
    Float *toward;

    dst = (Float *)out;
    src = (Float *)from;
    toward = (Float *)to;
    blend = njSin((Angle)(blend * 16384.0F));
    keep = 1.0F - blend;
    floats = count * 3;
    for (; floats > 0; floats--) {
        /* @hack spelling: toward stepped inside the product, which keeps to in
         * R12 and out in R14 (MOV R5,R12 at 0x8c058ec2, MOV R6,R14 at
         * 0x8c058ecc); stepped as a statement, the two trade and 5 of 47
         * words differ */
        *dst = (keep * *src) + (*toward++ * blend);
        src++;
        dst++;
    }
}

void points_blend_square(NJS_POINT3 *from, NJS_POINT3 *to, NJS_POINT3 *out,
                         Float blend, int count)
{
    Float keep;
    Uint32 floats;
    Float *src;
    Float *dst;
    Float *toward;

    src = (Float *)from;
    toward = (Float *)to;
    dst = (Float *)out;
    blend = blend * blend;
    keep = 1.0F - blend;
    floats = count * 3;
    for (; floats > 0; floats--) {
        *dst = (keep * *src) + (*toward * blend);
        src++;
        toward++;
        dst++;
    }
}

/* a strip's first word holds its length in these bits */
#define STRIP_LENGTH 0x3fff

#define CROSS(out, a, b)                                                       \
    ((out)->x = ((a)->y * (b)->z) - ((a)->z * (b)->y),                         \
     (out)->y = ((a)->z * (b)->x) - ((a)->x * (b)->z),                         \
     (out)->z = ((a)->x * (b)->y) - ((a)->y * (b)->x))

void model_calc_face_normals(NJS_MODEL *model)
{
    NJS_POINT3 *points = model->points;
    NJS_MESHSET *meshset = model->meshsets;
    int meshset_index;

    for (meshset_index = 0; model->nbMeshset > meshset_index;
         ++meshset, ++meshset_index) {
        NJS_VECTOR *normals = meshset->normals;
        Uint16 count = meshset->nbMesh;
        Sint16 *indices = meshset->meshes;

        switch (meshset->type_matId & NJD_MESHSET_MASK) {
        case NJD_MESHSET_3: {
            NJS_VECTOR edges[2];
            int polygon;

            for (polygon = 0; polygon < count; ++polygon) {
                Uint16 v0 = indices[polygon * 3];
                Uint16 v1 = indices[(polygon * 3) + 1];
                Uint16 v2 = indices[(polygon * 3) + 2];

                VECTOR_DIFF(&edges[0], &points[v1], &points[v0]);
                VECTOR_DIFF(&edges[1], &points[v2], &points[v0]);
                njUnitVector(&edges[0]);
                njUnitVector(&edges[1]);
                CROSS(&normals[polygon], &edges[0], &edges[1]);
            }
            /* @hack spelling: the block closed before its break, which has
             * the quad case copy normals and indices through R2 and R1
             * (0x8c05911a to 0x8c059120); with the break inside, they go
             * through R3 and R2 and 4 of 543 words differ */
        } break;
        case NJD_MESHSET_4: {
            NJS_VECTOR edges[2];
            int polygon;

            for (polygon = 0; polygon < count; ++polygon) {
                Uint16 v0 = indices[polygon * 4];
                Uint16 v1 = indices[(polygon * 4) + 1];
                Uint16 v3 = indices[(polygon * 4) + 3];

                VECTOR_DIFF(&edges[0], &points[v1], &points[v0]);
                VECTOR_DIFF(&edges[1], &points[v3], &points[v0]);
                njUnitVector(&edges[0]);
                njUnitVector(&edges[1]);
                CROSS(&normals[polygon], &edges[0], &edges[1]);
            }
            break;
        }
        case NJD_MESHSET_TRIMESH: {
            NJS_VECTOR edges[2];
            int strip;

            for (strip = 0; strip < count; ++strip) {
                Sint16 length = *indices++;
                int winding;
                Uint16 v0;
                Uint16 v1;
                Uint16 v2;
                int triangle;

                if (length & NJD_TRIMESH_END) {
                    winding = -1;
                } else {
                    winding = 0;
                }
                length &= STRIP_LENGTH;
                v0 = *indices++;
                v1 = *indices++;
                v2 = *indices++;
                /* each pass reads the next index ahead, so the last reads
                 * the word after the strip, which --indices below gives
                 * back */
                /* @bug logic: every strip stores its normals from the
                 * meshset's first (MOV.L @(32,R15),R5 at 0x8c059296), over
                 * the strips before it, where each strip's own are meant.
                 * model_calc_point_normals reads them back the same way, so
                 * a point of an earlier strip sums a later strip's normals */
                for (triangle = 0; triangle < length - 2; ++triangle) {
                    VECTOR_DIFF(&edges[0], &points[v1], &points[v0]);
                    VECTOR_DIFF(&edges[1], &points[v2], &points[v0]);
                    njUnitVector(&edges[0]);
                    njUnitVector(&edges[1]);
                    if (winding >= 0) {
                        CROSS(&normals[triangle], &edges[0], &edges[1]);
                    } else {
                        CROSS(&normals[triangle], &edges[1], &edges[0]);
                    }
                    winding = ~winding;
                    v0 = v1;
                    v1 = v2;
                    v2 = *indices++;
                }
                --indices;
            }
            break;
        }
        default:
            return;
        }
    }
}

void model_calc_point_normals(NJS_MODEL *model)
{
    Uint32 points = model->nbPoint;
    NJS_VECTOR *point_normals = model->normals;
    Uint32 point;

    for (point = 0; point < points; ++point) {
        NJS_VECTOR *normal = &point_normals[point];
        NJS_MESHSET *meshset;
        Uint32 meshset_index;
        Float count;

        /* @hack spelling: the zero stored through the array's entry, which
         * keeps 0 in R7 and the entry's address in R14 (MOV #0,R7 at
         * 0x8c05944a, 0x8c059464 to 0x8c05947c); through normal, the two
         * trade and 36 of 312 words differ */
        point_normals[point].x = point_normals[point].y =
            point_normals[point].z = 0.0F;
        meshset = model->meshsets;
        count = 0.0F;
        /* @hack spelling: meshset_index = meshset_index + 1, which keeps
         * the strip loop's triangle in R8 and v0 in R9 (MOV R7,R8 at
         * 0x8c059602, MOV.W @R5+,R9 at 0x8c05960e). as ++meshset_index, the
         * two trade and 7 of 312 words differ */
        for (meshset_index = 0; model->nbMeshset > meshset_index;
             ++meshset, meshset_index = meshset_index + 1) {
            NJS_VECTOR *normals = meshset->normals;
            Uint16 meshes = meshset->nbMesh;
            Sint16 *indices = meshset->meshes;

            switch (meshset->type_matId & NJD_MESHSET_MASK) {
            case NJD_MESHSET_3: {
                Uint32 polygon;

                for (polygon = 0; polygon < meshes; ++polygon) {
                    Uint16 v0 = indices[polygon * 3];
                    Uint16 v1 = indices[(polygon * 3) + 1];
                    Uint16 v2 = indices[(polygon * 3) + 2];

                    if (point == v0 || point == v1 || point == v2) {
                        count += 1.0F;
                        normal->x += normals[polygon].x;
                        normal->y += normals[polygon].y;
                        normal->z += normals[polygon].z;
                    }
                }
                /* @hack spelling: the block closed before its break, which has
                 * the quad case start polygon at 0 in R2 and meshes in R1
                 * (0x8c059560, 0x8c059564); with the break inside, they take
                 * R3 and R2 and 7 of 312 words differ */
            } break;
            case NJD_MESHSET_4: {
                Uint32 polygon;

                for (polygon = 0; polygon < meshes; ++polygon) {
                    Uint16 v0 = indices[polygon * 4];
                    Uint16 v1 = indices[(polygon * 4) + 1];
                    Uint16 v2 = indices[(polygon * 4) + 2];
                    Uint16 v3 = indices[(polygon * 4) + 3];

                    if (point == v0 || point == v1 || point == v2 ||
                        point == v3) {
                        count += 1.0F;
                        normal->x += normals[polygon].x;
                        normal->y += normals[polygon].y;
                        normal->z += normals[polygon].z;
                    }
                }
                /* @hack spelling: the block closed before its break, which has
                 * the strip case start strip at 0 in R1 and meshes in R2
                 * (0x8c0595f6, 0x8c0595f8); with the break inside, they take
                 * R2 and R3 and 3 of 312 words differ */
            } break;
            case NJD_MESHSET_TRIMESH: {
                Uint32 strip;

                for (strip = 0; strip < meshes; ++strip) {
                    Sint16 length = *indices++;
                    Uint16 v0;
                    Uint16 v1;
                    Uint16 v2;
                    int triangle;

                    length &= STRIP_LENGTH;
                    v0 = *indices++;
                    v1 = *indices++;
                    v2 = *indices++;
                    for (triangle = 0; triangle < length - 2; ++triangle) {
                        if (point == v0 || point == v1 || point == v2) {
                            count += 1.0F;
                            normal->x += normals[triangle].x;
                            normal->y += normals[triangle].y;
                            normal->z += normals[triangle].z;
                        }
                        v0 = v1;
                        v1 = v2;
                        v2 = *indices++;
                    }
                    --indices;
                }
                break;
            }
            default:
                return;
            }
        }
        /* @hack spelling: the division through the array's entry, which forms
         * the entry's address again, in R5 (0x8c05968c to 0x8c05969a);
         * through normal, SHC uses R14's, the function runs 14 bytes
         * shorter, and 53 of 312 words differ */
        point_normals[point].x /= count;
        point_normals[point].y /= count;
        point_normals[point].z /= count;
    }
}

int tree_node_index(NJS_OBJECT *root, NJS_OBJECT *object)
{
    int count = 0;
    /* @hack spelling: root through node, which SHC stores to a frame slot
     * it never reads (MOV.L R4,@(4,R15) at 0x8c059708); passed straight,
     * the frame is 4 bytes smaller and 16 of 19 words differ */
    NJS_OBJECT *node = root;
    NJS_OBJECT *found;

    found = tree_find_node(node, object, &count);
    if (found == NULL) {
        return -1;
    }
    return count;
}

/* object when the walk from node reaches it, and null otherwise. the walk
 * takes each node, then its child's tree, then its next sibling's. *count
 * gains 1 for each node the walk passes before object */
static NJS_OBJECT *tree_find_node(NJS_OBJECT *node, NJS_OBJECT *object,
                                  int *count)
{
    NJS_OBJECT *next;

    if (node == object) {
        return object;
    }
    (*count)++;
    /* @hack spelling: each recursive result through next, which copies R0
     * to R4 for the test (MOV R0,R4 at 0x8c05974a, 0x8c05975c); compared
     * in place, the copies go and 22 of 38 words differ */
    if ((next = node->child) != NULL) {
        next = tree_find_node(next, object, count);
        if (next == object) {
            return object;
        }
    }
    if ((next = node->sibling) != NULL) {
        next = tree_find_node(next, object, count);
        if (next == object) {
            return object;
        }
    }
    return NULL;
}

Float float_wrap(Float value, Float period)
{
    if (period == 0.0F) {
        return 0.0F;
    }
    return value - (floorf(value / period) * period);
}

int tree_list_nodes(NJS_OBJECT *root, NJS_OBJECT **nodes)
{
    int count = 0;

    if (nodes) {
        nodes[count] = root;
    }
    return tree_collect_nodes(nodes, &count);
}

/* list the walk from the node at nodes[*count] into the entries after it,
 * and return the new *count. each node goes into nodes when the walk
 * reaches it, and the walk reads it back from there */
static int tree_collect_nodes(NJS_OBJECT **nodes, int *count)
{
    NJS_OBJECT *node = nodes[*count];
    NJS_OBJECT *next;

    (*count)++;
    next = node->child;
    if (next != NULL) {
        if (nodes) {
            nodes[*count] = next;
        }
        tree_collect_nodes(nodes, count);
    }
    next = node->sibling;
    if (next != NULL) {
        if (nodes) {
            nodes[*count] = next;
        }
        tree_collect_nodes(nodes, count);
    }
    return *count;
}

void vector_get_angles(const NJS_VECTOR *dir, const NJS_VECTOR *from,
                       Angle angles[3])
{
    Float across = njSqrt((dir->x * dir->x) + (dir->z * dir->z));

    angles[1] = (int)(atan2f(dir->x, -dir->z) * ANGLE_PER_RADIAN);
    angles[2] = (int)(atan2f(-dir->y, across) * ANGLE_PER_RADIAN);
    if (from != NULL) {
        Float from_across = njSqrt((from->x * from->x) + (from->z * from->z));

        angles[1] -= (int)(atan2f(from->x, -from->z) * ANGLE_PER_RADIAN);
        angles[2] -= (int)(atan2f(-from->y, from_across) * ANGLE_PER_RADIAN);
    }
}

void draw_attr_save(void)
{
    saved_attr_and = _nj_constant_attr_and_;
    saved_attr_or = _nj_constant_attr_or_;
}

void draw_attr_restore(void)
{
    _nj_constant_attr_and_ = saved_attr_and;
    _nj_constant_attr_or_ = saved_attr_or;
}

void draw_attr_set(Uint32 and_attr, Uint32 or_attr)
{
    _nj_constant_attr_and_ |= and_attr;
    _nj_constant_attr_or_ |= or_attr;
}

void draw_attr_clear(Uint32 and_attr, Uint32 or_attr)
{
    _nj_constant_attr_and_ &= ~and_attr;
    _nj_constant_attr_or_ &= ~or_attr;
}

void draw_control_save(void)
{
    saved_control_3d = _nj_control_3d_flag_;
}

void draw_control_restore(void)
{
    _nj_control_3d_flag_ = saved_control_3d;
}

void draw_control_set(Uint32 flags)
{
    _nj_control_3d_flag_ |= flags;
}

void draw_control_clear(Uint32 flags)
{
    _nj_control_3d_flag_ &= ~flags;
}

Bool world_sweep_all_hit(NJS_POINT3 *pos, NJS_VECTOR *speed, struct angles *ang,
                         Float radius)
{
    return world_sweep_ball(pos, speed, ang, 0, radius) != 0;
}

Bool world_sweep_ball_hit(NJS_POINT3 *pos, NJS_VECTOR *speed,
                          struct angles *ang, int skip, Float radius)
{
    return world_sweep_ball(pos, speed, ang, skip, radius) != 0;
}

int world_sweep_all(NJS_POINT3 *pos, NJS_VECTOR *speed, struct angles *ang,
                    Float radius)
{
    return world_sweep_ball(pos, speed, ang, 0, radius);
}

int world_sweep_ball(NJS_POINT3 *pos, NJS_VECTOR *speed, struct angles *ang,
                     int skip, Float radius)
{
    struct polygon_ball hit;
    struct polygon_ball *contact;
    struct polygon_ball best;
    struct angles offset;
    struct world_object *objects;
    struct task *owner;
    struct task_force *force;
    Uint16 i;
    int result;
    Angle slope_x;
    Angle slope_z;
    Angle flipped_x;
    Angle flipped_z;
    int gap_x;
    int gap_z;
    Float nearest;
    Float distance;

    result = 0;
    objects = world_objects;
    contact = &hit;
    offset.x = 0;
    offset.y = 0;
    offset.z = 0;
    if (ang == NULL) {
        contact->normal.x = 0.0F;
        contact->normal.y = 1.0F;
        contact->normal.z = 0.0F;
    } else {
        /* @hack spelling: up and unit_y in a scope of their own, which gives
         * them turn's and moved's frame slots, R15+64 and R15+52 (ADD #64,R6
         * and ADD #52,R5 at 0x8c059aca and 0x8c059ac8). declared with the
         * function's locals, they take slots of their own, the frame grows
         * from 352 bytes (MOV.W at 0x8c059a1a) to 376, and 10 of 380 words
         * differ */
        NJS_VECTOR up;
        NJS_VECTOR unit_y;

        unit_y.x = 0.0F;
        unit_y.y = 1.0F;
        unit_y.z = 0.0F;
        njPushMatrix(&_unit_matrix_);
        TURN(njRotateZ, ang->z);
        TURN(njRotateX, ang->x);
        TURN(njRotateY, ang->y);
        njCalcVector(NULL, &unit_y, &up);
        njPopMatrix(1);
        contact->normal = up;
    }
    contact->pos = *pos;
    contact->radius = radius;
    contact->speed = *speed;
    polygon_collect_near(contact);
    for (i = 0; i < world_count; i++) {
        struct angles turn;
        NJS_VECTOR moved;

        if (objects[i].flags & skip) {
            continue;
        }
        if (polygon_sweep(contact, objects[i].object, objects[i].flags)) {
            if (ang != NULL && contact->hit.normal.y < 0.0F) {
                slope_x =
                    (Angle)(asinf(contact->hit.normal.z) * ANGLE_PER_RADIAN);
                slope_z =
                    -(Angle)(asinf(contact->hit.normal.x) * ANGLE_PER_RADIAN);
                flipped_x = (Uint16)(0x8000 - slope_x);
                flipped_z = (Uint16)(0x8000 - slope_z);
                gap_x = angle_distance(ang->x, flipped_x);
                gap_z = angle_distance(ang->z, flipped_z);
                if (gap_x < gap_z) {
                    contact->hit.ang_x = flipped_x;
                    contact->hit.ang_z = slope_z;
                } else {
                    contact->hit.ang_x = slope_x;
                    contact->hit.ang_z = flipped_z;
                }
            }
            owner = objects[i].owner;
            /* @bug uninitialized: for an object with an owner but no force,
             * the BT at 0x8c059bc6 skips every store to turn. turn keeps
             * what its frame slot held: an earlier hit's turn, or before
             * any, up's floats when ang isn't null. when the hit is the
             * nearest, offset takes that (0x8c059c8a to 0x8c059c98,
             * 0x8c059cb6 to 0x8c059cc2), and ang adds offset */
            if (owner != NULL) {
                force = owner->force;
                if (force != NULL) {
                    VECTOR_ADD(&contact->hit.pos, &force->speed);
                    turn = force->turn;
                }
            } else {
                turn.x = 0;
                turn.y = 0;
                turn.z = 0;
            }
            moved.x = contact->hit.pos.x - pos->x;
            moved.y = contact->hit.pos.y - pos->y;
            moved.z = contact->hit.pos.z - pos->z;
            if (result == 0) {
                best = hit;
                nearest = njScalor(&moved);
                offset = turn;
            } else {
                distance = njScalor(&moved);
                if (nearest > distance) {
                    best = hit;
                    nearest = distance;
                    offset = turn;
                }
            }
            if (njInnerProduct(&contact->hit.normal, &world_gravity) > -0.5F) {
                result |= HIT_WALL;
            } else {
                result |= HIT_GROUND;
            }
        }
    }
    if (result != 0) {
        contact = &best;
        *pos = contact->hit.pos;
        *speed = contact->hit.speed;
        if (ang != NULL) {
            ang->x = contact->hit.ang_x + offset.x;
            ang->y += offset.y;
            ang->z = contact->hit.ang_z + offset.z;
        }
    }
    return result;
}

void evalflags_set_all(NJS_OBJECT **objects, Uint32 flags)
{
    while (*objects != NULL) {
        (*objects)->evalflags |= flags;
        objects++;
    }
}

void evalflags_clear_all(NJS_OBJECT **objects, Uint32 flags)
{
    while (*objects != NULL) {
        (*objects)->evalflags &= ~flags;
        objects++;
    }
}

/* store object's matrix at *matrices, and its descendants' after it, in
 * walk order. a node's matrix is its parent's, translated, turned and
 * scaled by the node's pos, ang and scl. *matrices moves past each. the
 * result is object's next sibling, for the caller to take next */
static NJS_OBJECT *tree_calc_node(NJS_MATRIX **matrices, NJS_OBJECT *object,
                                  NJS_MATRIX *parent)
{
    NJS_MATRIX matrix;
    NJS_OBJECT *child;

    njSetMatrix(&matrix, parent);
    njTranslate(&matrix, object->pos[0], object->pos[1], object->pos[2]);
    TURN_MATRIX(njRotateZ, &matrix, object->ang[2]);
    TURN_MATRIX(njRotateY, &matrix, object->ang[1]);
    TURN_MATRIX(njRotateX, &matrix, object->ang[0]);
    njScaleV(&matrix, (NJS_VECTOR *)object->scl);
    njSetMatrix(*matrices, &matrix);
    (*matrices)++;
    child = object->child;
    while (child != NULL) {
        child = tree_calc_node(matrices, child, &matrix);
    }
    return object->sibling;
}

NJS_MATRIX *tree_calc_matrices(NJS_MATRIX *base, NJS_OBJECT *object, int index)
{
    NJS_MATRIX *matrices;
    NJS_MATRIX matrix;

    if (base == NULL) {
        njGetMatrix(&matrix);
    } else {
        njSetMatrix(&matrix, base);
    }
    matrices = tree_matrices;
    tree_calc_node(&matrices, object, &matrix);
    return &tree_matrices[index];
}

void tree_get_matrix(NJS_MATRIX *out, int index)
{
    njSetMatrix(out, &tree_matrices[index]);
}

void sprite_init(NJS_SPRITE *sprite, NJS_TEXLIST *texlist, NJS_TEXANIM *texanim)
{
    sprite->tlist = texlist;
    sprite->tanim = texanim;
    sprite->ang = 0;
    sprite->sx = 1.0F;
    sprite->sy = 1.0F;
    sprite->p.x = 0.0F;
    sprite->p.y = 0.0F;
    sprite->p.z = 0.0F;
}

/* write value's digits back from end, a single 0 for 0, and return the
 * first. format_hex and format_hex_upper write hex digits */
static char *format_decimal(char *end, Uint32 value)
{
    do {
        *--end = (char)((value % 10) + '0');
        value /= 10;
    } while (value != 0);
    return end;
}

static char *format_hex(char *end, Uint32 value)
{
    do {
        int digit = value & 15;

        *--end = (char)(digit + (digit < 10 ? '0' : 'a' - 10));
        value >>= 4;
    } while (value != 0);
    return end;
}

static char *format_hex_upper(char *end, Uint32 value)
{
    do {
        int digit = value & 15;

        *--end = (char)(digit + (digit < 10 ? '0' : 'A' - 10));
        value >>= 4;
    } while (value != 0);
    return end;
}

int string_vformat(char *buffer, const char *format, va_list args)
{
    char *out;
    char *field;
    char *end;
    int ch;
    int sign;
    Bool zero;
    int width;
    Bool left;
    int length;
    int pad;
    int value;
    char text[64];

    out = buffer;
    end = &text[sizeof(text) - 1];
    *end = '\0';
    /* NOLINTNEXTLINE(bugprone-signed-char-misuse) */
    while ((ch = *format++) != 0) {
        if (ch == '%') {
            width = 0;
            left = FALSE;
            sign = 0;
            zero = FALSE;
        flag:
            /* NOLINTNEXTLINE(bugprone-signed-char-misuse) */
            ch = *format++;
            if (ch == '-') {
                left = TRUE;
                goto flag;
            } else if (ch == '+') {
                sign = 1;
                goto flag;
            } else if (ch == '0') {
                zero = TRUE;
                goto flag;
            }
            while (ch >= '0' && ch <= '9') {
                width = (width * 10) + ch - '0';
                /* NOLINTNEXTLINE(bugprone-signed-char-misuse) */
                ch = *format++;
            }
            switch (ch) {
            case 's':
                field = va_arg(args, char *);
                break;
            case 'c':
                field = end;
                *--field = va_arg(args, int);
                break;
            case 'u':
                field = format_decimal(end, va_arg(args, Uint32));
                if (zero) {
                    while (field > end - width + sign) {
                        *--field = '0';
                    }
                }
                if (sign) {
                    *--field = '+';
                }
                break;
            case 'd':
                value = va_arg(args, int);
                if (value >= 0) {
                    field = format_decimal(end, value);
                } else {
                    sign = 1;
                    field = format_decimal(end, -value);
                }
                if (zero) {
                    while (field > end - width + sign) {
                        *--field = '0';
                    }
                }
                if (sign) {
                    *--field = value < 0 ? '-' : '+';
                }
                break;
            case 'x':
                field = format_hex(end, va_arg(args, Uint32));
                if (zero) {
                    while (field > end - width + sign) {
                        *--field = '0';
                    }
                }
                break;
            case 'X':
                field = format_hex_upper(end, va_arg(args, Uint32));
                if (zero) {
                    while (field > end - width + sign) {
                        *--field = '0';
                    }
                }
                break;
            default:
                (void)va_arg(args, int);
                field = end;
                break;
            }
            length = strlen(field);
            pad = width - length;
            if (left) {
                strcpy(out, field);
                out += length;
                while (pad > 0) {
                    *out++ = ' ';
                    pad--;
                }
            } else {
                while (pad > 0) {
                    *out++ = ' ';
                    pad--;
                }
                strcpy(out, field);
                out += length;
            }
        } else {
            *out++ = ch;
        }
    }
    *out = '\0';
    return out - buffer;
}

int string_format(char *buffer, const char *format, ...)
{
    va_list args;
    int length;

    va_start(args, format);
    length = string_vformat(buffer, format, args);
    va_end(args);
    return length;
}

Float float_approach(Float from, Float to, Float step)
{
    Float result;

    /* @hack spelling: one return, of result, which has SHC copy step
     * through FR3 and build result in FR6 (0x8c05a2fc to 0x8c05a300); with
     * a return in each branch, SHC subtracts in place and 20 of 21 words
     * differ */
    if (from > to) {
        result = from - step;
        if (to > result) {
            result = to;
        }
    } else if (to > from) {
        result = from + step;
        if (result > to) {
            result = to;
        }
    } else {
        result = from;
    }
    return result;
}

void model_weld(NJS_OBJECT *object, NJS_MOTION *motion, Float frame,
                struct model_weld *welds)
{
    NJS_ACTION action;
    NJS_OBJECT *nodes[64];
    NJS_OBJECT *object_a;
    NJS_OBJECT *object_b;
    NJS_MATRIX *matrix_a;
    NJS_POINT3 *points_a;
    NJS_VECTOR *normals_a;
    NJS_POINT3 *points_b;
    NJS_VECTOR *normals_b;
    /* @hack type: i a Sint32, which keeps pose_get_matrix's address in R9
     * and object_b in R10 (0x8c05a362, MOV.L @(R0,R12),R10 at 0x8c05a372);
     * as Uint32, the two trade and 85 of 135 words differ */
    Sint32 i;
    Uint32 count;
    int index_a;
    int index_b;

    action.object = object;
    action.motion = motion;
    count = tree_list_nodes(object, nodes);
    pose_action(&_unit_matrix_, &action, frame, count, NULL);
    while (welds->count != 0) {
        object_a = nodes[welds->node_a];
        object_b = nodes[welds->node_b];
        njPushMatrix(NULL);
        pose_get_matrix(welds->node_a, NULL);
        matrix_a = _nj_current_matrix_ptr_;
        points_a = object_a->model->points;
        normals_a = object_a->model->normals;
        njPushMatrix(NULL);
        pose_get_matrix(welds->node_b, NULL);
        points_b = object_b->model->points;
        normals_b = object_b->model->normals;
        njInvertMatrix(NULL);
        njMultiMatrix(NULL, matrix_a);
        for (i = 0; i < welds->count; i++) {
            index_a = welds->pairs[i * 2];
            index_b = welds->pairs[(i * 2) + 1];
            njCalcPoint(NULL, &points_a[index_a], &points_b[index_b]);
            njCalcVector(NULL, &normals_a[index_a], &normals_b[index_b]);
        }
        njPopMatrix(2);
        welds++;
    }
}
