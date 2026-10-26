/* debug text: a copy of ninja's print with a color and size of its own,
 * drawn with the font and polygon header njInitPrint sets up */
#include <kamui.h>
#include <machine.h>
#include <ninja.h>
#include <sg_xpt.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include "draw/text.h"
#include "system/frame.h"

/* a store queue writes to the 64 MB area its QACR register selects, at the
 * offset its own address carries. SQ_AREA(ptr) is ptr's area as QACR takes
 * it: ptr's bits 28 to 26, in bits 4 to 2 */
#define SQ_AREA(ptr) (((ptr) >> 24) & 0x1c)
/* the bits of a pointer that a store queue's address carries: its offset */
#define SQ_ADDRESS 0x03ffffff
/* the store queues' own addresses start here */
#define SQ_BASE 0xe0000000
/* the bits of a list's write pointer above its offset */
#define LIST_TOP 0xfc000000

/* the SH-4's store queue address control registers, one for each queue */
#define QACR0 (*(Uint32 *)0xff000038)
#define QACR1 (*(Uint32 *)0xff00003c)

/* a list takes ENTRY_SIZE bytes at a time, as a store queue holds them: a
 * vertex, or a polygon header. the text writes a vertex's first
 * VERTEX_WRITTEN bytes, all but its offset color, and a polygon header's
 * first HEADER_WRITTEN, its four parameter words */
#define ENTRY_SIZE 32
#define VERTEX_WRITTEN 28
#define HEADER_WRITTEN 16

/* the index in _nj_context_ of the vertex context njInitPrint sets up */
#define DEBUG_CONTEXT 4

/* a global index no texture has. with _nj_current_texture at it, ninja's
 * next textured draw sets its texture again */
#define NO_TEXTURE (-2)

/* the color text_style_init sets, an opaque light gray, and the side of a
 * character's cell, in pixels */
#define TEXT_COLOR 0xffbfbfbf
#define TEXT_SIZE 16.0F

/* ninja's vertex contexts. njInitPrint has kamui compute the font's polygon
 * header into DEBUG_CONTEXT's */
/* NOLINTBEGIN(bugprone-reserved-identifier,readability-identifier-naming) */
extern KMVERTEXCONTEXT _nj_context_[];
/* NOLINTEND(bugprone-reserved-identifier,readability-identifier-naming) */
/* ninja's vertex buffer, whose pCurrentPtr holds each list's write pointer */
extern KMVERTEXBUFFDESC _nj_vertex_buffer_;

static char text_hex_digits[16] = {'0', '1', '2', '3', '4', '5', '6', '7',
                                   '8', '9', 'A', 'B', 'C', 'D', 'E', 'F'};

static Uint32 text_color;
static Float text_size;
/* the four words of the font's polygon header, copied from DEBUG_CONTEXT */
static Uint32 text_tex_param;
static Uint32 text_tsp_param;
static Uint32 text_global_param;
static Uint32 text_isp_param;
static Uint8 *text_sq; /* where the text writes, as a store queue address */
/* the translucent list's write pointer, its area as QACR takes it, and its
 * bits above the offset, as TEXT_OPEN finds them */
static Uint32 text_list_ptr;
static Uint32 text_sq_area;
static Uint32 text_list_top;

void text_style_init(Uint32 *texture, Uint32 size, Uint32 attr)
{
    text_color = TEXT_COLOR;
    text_size = TEXT_SIZE;
    text_global_param = _nj_context_[DEBUG_CONTEXT].GLOBALPARAMBUFFER;
    text_isp_param = _nj_context_[DEBUG_CONTEXT].ISPPARAMBUFFER;
    text_tsp_param = _nj_context_[DEBUG_CONTEXT].TSPPARAMBUFFER;
    text_tex_param = _nj_context_[DEBUG_CONTEXT].TexturePARAMBUFFER;
}

void text_exit(void)
{
}

void text_set_size(Uint16 size)
{
    text_size = size;
}

void text_set_color(Uint32 color)
{
    text_color = color;
}

/* step text_sq back 4 bytes and store value there */
#define SQ_PUT(type, value) (*(type *)(text_sq -= 4) = (value))

/* the font's texture holds 16 rows of 16 characters. a character takes its
 * cell less FONT_INSET at the cell's top and left */
#define FONT_COLUMNS 16
#define FONT_CELL (1.0F / FONT_COLUMNS)
#define FONT_INSET (1.0F / 1024)
#define FONT_SPAN (FONT_CELL - FONT_INSET)

/* a vertex's 1/w, which kamui takes as its depth: a larger 1/w draws
 * nearer. text_draw_char's vertices take 1 / 1.1, and text_draw_char_front's
 * 2 */
#define TEXT_INV_W 0.90909094F
#define FRONT_INV_W 2.0F

/* send ch at x, y in pixels, the top left corner of its place on the
 * screen, as a strip of four vertices. each is written from its color back
 * to its control word, then sent by a prefetch of its start. the character
 * leans: its top runs from x + 1 to x + text_size + 4. its bottom runs from
 * x - 4 to x + text_size - 1. a '0' draws as the font's 'O' */
static void text_draw_char(Sint16 x, Sint16 y, char ch)
{
    Sint16 col;
    Sint16 row;
    Float left;
    Float top;
    Float right;
    Float bottom;

    if (ch == '0') {
        ch = 'O';
    }
    col = ch % FONT_COLUMNS;
    row = ch / FONT_COLUMNS;
    left = (col * FONT_CELL) + FONT_INSET;
    top = (row * FONT_CELL) + FONT_INSET;
    right = left + FONT_SPAN;
    bottom = top + FONT_SPAN;
    text_sq += VERTEX_WRITTEN;
    SQ_PUT(Uint32, text_color);
    SQ_PUT(Float, top);
    SQ_PUT(Float, left);
    SQ_PUT(Float, TEXT_INV_W);
    SQ_PUT(Float, y);
    SQ_PUT(Float, x + 1.0F);
    SQ_PUT(Uint32, KM_VERTEXPARAM_NORMAL);
    prefetch(text_sq);
    text_sq += ENTRY_SIZE + VERTEX_WRITTEN;
    SQ_PUT(Uint32, text_color);
    SQ_PUT(Float, bottom);
    SQ_PUT(Float, left);
    SQ_PUT(Float, TEXT_INV_W);
    SQ_PUT(Float, y + text_size);
    SQ_PUT(Float, x - 4.0F);
    SQ_PUT(Uint32, KM_VERTEXPARAM_NORMAL);
    prefetch(text_sq);
    text_sq += ENTRY_SIZE + VERTEX_WRITTEN;
    SQ_PUT(Uint32, text_color);
    SQ_PUT(Float, top);
    SQ_PUT(Float, right);
    SQ_PUT(Float, TEXT_INV_W);
    SQ_PUT(Float, y);
    SQ_PUT(Float, x + text_size + 4.0F);
    SQ_PUT(Uint32, KM_VERTEXPARAM_NORMAL);
    prefetch(text_sq);
    text_sq += ENTRY_SIZE + VERTEX_WRITTEN;
    SQ_PUT(Uint32, text_color);
    SQ_PUT(Float, bottom);
    SQ_PUT(Float, right);
    SQ_PUT(Float, TEXT_INV_W);
    SQ_PUT(Float, y + text_size);
    SQ_PUT(Float, x + text_size - 1.0F);
    SQ_PUT(Uint32, KM_VERTEXPARAM_ENDOFSTRIP);
    prefetch(text_sq);
    text_sq += ENTRY_SIZE;
}

/* text_draw_char at FRONT_INV_W, in front of text_draw_char's */
static void text_draw_char_front(Sint16 x, Sint16 y, char ch)
{
    Sint16 col;
    Sint16 row;
    Float left;
    Float top;
    Float right;
    Float bottom;

    if (ch == '0') {
        ch = 'O';
    }
    col = ch % FONT_COLUMNS;
    row = ch / FONT_COLUMNS;
    left = (col * FONT_CELL) + FONT_INSET;
    top = (row * FONT_CELL) + FONT_INSET;
    right = left + FONT_SPAN;
    bottom = top + FONT_SPAN;
    text_sq += VERTEX_WRITTEN;
    SQ_PUT(Uint32, text_color);
    SQ_PUT(Float, top);
    SQ_PUT(Float, left);
    SQ_PUT(Float, FRONT_INV_W);
    SQ_PUT(Float, y);
    SQ_PUT(Float, x + 1.0F);
    SQ_PUT(Uint32, KM_VERTEXPARAM_NORMAL);
    prefetch(text_sq);
    text_sq += ENTRY_SIZE + VERTEX_WRITTEN;
    SQ_PUT(Uint32, text_color);
    SQ_PUT(Float, bottom);
    SQ_PUT(Float, left);
    SQ_PUT(Float, FRONT_INV_W);
    SQ_PUT(Float, y + text_size);
    SQ_PUT(Float, x - 4.0F);
    SQ_PUT(Uint32, KM_VERTEXPARAM_NORMAL);
    prefetch(text_sq);
    text_sq += ENTRY_SIZE + VERTEX_WRITTEN;
    SQ_PUT(Uint32, text_color);
    SQ_PUT(Float, top);
    SQ_PUT(Float, right);
    SQ_PUT(Float, FRONT_INV_W);
    SQ_PUT(Float, y);
    SQ_PUT(Float, x + text_size + 4.0F);
    SQ_PUT(Uint32, KM_VERTEXPARAM_NORMAL);
    prefetch(text_sq);
    text_sq += ENTRY_SIZE + VERTEX_WRITTEN;
    SQ_PUT(Uint32, text_color);
    SQ_PUT(Float, bottom);
    SQ_PUT(Float, right);
    SQ_PUT(Float, FRONT_INV_W);
    SQ_PUT(Float, y + text_size);
    SQ_PUT(Float, x + text_size - 1.0F);
    SQ_PUT(Uint32, KM_VERTEXPARAM_ENDOFSTRIP);
    prefetch(text_sq);
    text_sq += ENTRY_SIZE;
}

/* point the store queues at the translucent list and send the font's
 * polygon header. _nj_current_texture becomes NO_TEXTURE. the statements
 * stand bare: wrapped in a do-while (0), text_print_number and
 * text_print_hex compile to other code */
/* @hack spelling: _nj_vertex_buffer_ through its address, here and in
 * TEXT_CLOSE, which loads the address after the store to _nj_current_texture
 * (0x8c04348a in text_print_number); as _nj_vertex_buffer_ itself, 511 of
 * 1,013 words differ */
#define TEXT_OPEN()                                                            \
    _nj_current_texture = NO_TEXTURE;                                          \
    text_list_ptr =                                                            \
        (Uint32)(&_nj_vertex_buffer_)->pCurrentPtr[KM_TRANS_POLYGON];          \
    text_list_top = text_list_ptr & LIST_TOP;                                  \
    text_sq_area = SQ_AREA(text_list_ptr);                                     \
    QACR0 = text_sq_area;                                                      \
    QACR1 = text_sq_area;                                                      \
    text_sq = (Uint8 *)(SQ_BASE | (text_list_ptr & SQ_ADDRESS));               \
    text_sq += HEADER_WRITTEN;                                                 \
    SQ_PUT(Uint32, text_tex_param);                                            \
    SQ_PUT(Uint32, text_tsp_param);                                            \
    SQ_PUT(Uint32, text_isp_param);                                            \
    SQ_PUT(Uint32, text_global_param);                                         \
    prefetch(text_sq);                                                         \
    text_sq += ENTRY_SIZE;

/* set the translucent list's write pointer to the end of what the text
 * sent: text_sq's offset, with the pointer's own bits above it */
#define TEXT_CLOSE()                                                           \
    ((&_nj_vertex_buffer_)->pCurrentPtr[KM_TRANS_POLYGON] =                    \
         (Uint32 *)(((Uint32)text_sq & SQ_ADDRESS) | text_list_top))

void text_print(Int loc, const char *string)
{
    Sint16 x;
    Sint16 y;
    Sint16 length;
    Sint16 i;

    if (frame_step != 0) {
        return;
    }
    TEXT_OPEN();
    x = (Sint16)(loc >> 16) * text_size;
    y = (Sint16)loc * text_size;
    length = strlen(string);
    for (i = 0; i < length; i++) {
        text_draw_char(x, y, string[i]);
        x += text_size;
    }
    TEXT_CLOSE();
}

void text_print_front(Int loc, const char *string)
{
    Sint16 x;
    Sint16 y;
    Sint16 length;
    Sint16 i;

    if (frame_step != 0) {
        return;
    }
    TEXT_OPEN();
    x = (Sint16)(loc >> 16) * text_size;
    y = (Sint16)loc * text_size;
    length = strlen(string);
    for (i = 0; i < length; i++) {
        text_draw_char_front(x, y, string[i]);
        x += text_size;
    }
    TEXT_CLOSE();
}

void text_print_number(Int loc, Int value, Int digits)
{
    int digit;
    Sint16 x;
    Sint16 y;

    if (frame_step != 0) {
        return;
    }
    TEXT_OPEN();
    x = (Sint16)(loc >> 16) * text_size;
    y = (Sint16)loc * text_size;
    if (value < 0) {
        text_draw_char(x, y, '-');
        value = -value;
        x += text_size;
    }
    for (; digits > 0; digits--) {
        switch (digits) {
        case 1:
            digit = value % 10;
            break;
        case 2:
            digit = value / 10;
            value -= digit * 10;
            break;
        case 3:
            digit = value / 100;
            value -= digit * 100;
            break;
        case 4:
            digit = value / 1000;
            value -= digit * 1000;
            break;
        case 5:
            digit = value / 10000;
            value -= digit * 10000;
            break;
        case 6:
            digit = value / 100000;
            value -= digit * 100000;
            break;
        case 7:
            digit = value / 1000000;
            value -= digit * 1000000;
            break;
        case 8:
            digit = value / 10000000;
            value -= digit * 10000000;
            break;
        case 9:
            digit = value / 100000000;
            value -= digit * 100000000;
            break;
        default:
            digit = value / 1000000000;
            value -= digit * 1000000000;
            break;
        }
        if (digit > 9) {
            digit = 0;
        }
        text_draw_char(x, y, '0' + digit);
        x += text_size;
    }
    TEXT_CLOSE();
}

void text_print_binary(Int loc, Uint32 value, Int digits)
{
    Sint16 x;
    Sint16 y;
    int bit;

    if (frame_step != 0) {
        return;
    }
    TEXT_OPEN();
    x = (Sint16)(loc >> 16) * text_size;
    y = (Sint16)loc * text_size;
    for (bit = digits - 1; bit >= 0; bit--) {
        text_draw_char(x, y, '0' + ((value >> bit) & 1));
        x += text_size;
    }
    TEXT_CLOSE();
}

void text_print_hex(Int loc, Uint32 value, Int digits)
{
    Sint16 x;
    Sint16 y;
    int digit;
    int i;

    if (frame_step != 0) {
        return;
    }
    TEXT_OPEN();
    x = (Sint16)(loc >> 16) * text_size;
    y = (Sint16)loc * text_size;
    for (i = digits - 1; i >= 0; i--) {
        digit = (value >> (i * 4)) & 0xf;
        text_draw_char(x, y, text_hex_digits[digit]);
        x += text_size;
    }
    TEXT_CLOSE();
}

void text_print_float(Int loc, Float value, Int digits)
{
    char buffer[20];
    Sint16 x;
    Sint16 y;
    int length;
    int point;
    int pad;
    int padded;
    int i;

    if (frame_step != 0) {
        return;
    }
    TEXT_OPEN();
    x = (Sint16)(loc >> 16) * text_size;
    y = (Sint16)loc * text_size;
    if (value < 0.0F) {
        text_draw_char(x, y, '-');
        value *= -1.0F;
        x += text_size;
    }
    sprintf(buffer, "%0.*f", 5, value);
    length = strlen(buffer);
    for (point = 0; point < length; point++) {
        if (buffer[point] == '.') {
            break;
        }
    }
    pad = digits - point;
    padded = length + pad;
    /* @bug memory: a value with more than digits places before its
     * point makes pad negative, and the shift writes -pad bytes before buffer
     * (MOV.B R3,@-R7 at 0x8c043b34). each byte the shift copies then comes
     * from -pad bytes above it, so buffer fills with repeats of its last -pad
     * bytes, the NUL among them, and the print draws those */
    for (i = padded; i >= pad; i--) {
        buffer[i] = buffer[length--];
    }
    for (i = 0; i < pad; i++) {
        buffer[i] = '0';
    }
    for (i = 0; i < padded; i++) {
        text_draw_char(x, y, buffer[i]);
        x += text_size;
    }
    TEXT_CLOSE();
}

void text_printf(Int loc, const char *format, ...)
{
    char buffer[128];
    va_list args;

    if (frame_step != 0) {
        return;
    }
    va_start(args, format);
    vsprintf(buffer, format, args);
    va_end(args);
    text_print(loc, buffer);
}
