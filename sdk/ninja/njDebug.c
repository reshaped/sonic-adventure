/* debug text on the screen, from a font texture that holds 256 characters in 16
 * rows of 16 */
#include <kamui.h>
#include <machine.h>
#include <ninja.h>
#include <sg_xpt.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include "ninja/njkamui.h"

/* a store queue writes to the 64 MB area its QACR register selects, at the
 * offset within the area that the queue's own address carries. SQ_AREA is
 * ptr's area as QACR takes it: ptr's bits 28 to 26, in bits 4 to 2 */
#define SQ_AREA(ptr) (((ptr) >> 24) & 0x1c)
/* the bits of a pointer that a store queue's address carries: its offset */
#define SQ_ADDRESS 0x03ffffff
/* the store queues' own addresses start here */
#define SQ_BASE 0xe0000000
/* the bits of a list's write pointer above its offset */
#define LIST_TOP 0xfc000000

#define QACR0 (*(Uint32 *)0xff000038)
#define QACR1 (*(Uint32 *)0xff00003c)

/* the index in _nj_context_ of the print's vertex context */
#define DEBUG_CONTEXT 4

/* the side of the built-in font's texture, in texels */
#define FONT_SIZE 128

void njCharacter(Sint16 x, Sint16 y, char c);

static char print_digits[16] = {'0', '1', '2', '3', '4', '5', '6', '7',
                                '8', '9', 'A', 'B', 'C', 'D', 'E', 'F'};

static Uint32 print_color;
static Float print_size;
static NJS_TEXSURFACE font;
/* the four words of the font's polygon header, which
 * kmProcessVertexRenderState computes for the print's vertex context */
static Uint32 tex_param;
static Uint32 tsp_param;
static Uint32 global_param;
static Uint32 isp_param;
static Uint32 *print_sq;
static Uint32 list_ptr; /* the translucent list's write pointer */
static Uint32 sq_area;
static Uint32 list_top;

/* load the font texture, size texels a side in format attr, or the built-in
 * font when texture is null. set up the print's vertex context, and make the
 * print's color 0xffbfbfbf and its characters 8 pixels a side */
void njInitPrint(Uint32 *texture, Uint32 size, Uint32 attr)
{
    print_color = 0xffbfbfbf;
    print_size = 8.0F;
    if (texture == NULL) {
        kmCreateTextureSurface((PKMSURFACEDESC)&font, FONT_SIZE, FONT_SIZE,
                               NJD_TEXFMT_TWIDDLED | NJD_TEXFMT_ARGB_1555);
        kmLoadTexture((PKMSURFACEDESC)&font, (PKMDWORD)_nj_debug_char_, FALSE,
                      FALSE);
    } else {
        kmCreateTextureSurface((PKMSURFACEDESC)&font, size, size, attr);
        kmLoadTexture((PKMSURFACEDESC)&font, texture, FALSE, FALSE);
    }
    _nj_context_[DEBUG_CONTEXT].RenderState = KM_LISTTYPE | KM_COLORTYPE |
                                              KM_SHADINGMODE | KM_USESPECULAR |
                                              KM_USEALPHA;
    _nj_context_[DEBUG_CONTEXT].ParamType = KM_POLYGON;
    _nj_context_[DEBUG_CONTEXT].ListType = KM_TRANS_POLYGON;
    _nj_context_[DEBUG_CONTEXT].ColorType = KM_PACKEDCOLOR;
    _nj_context_[DEBUG_CONTEXT].ShadingMode = KM_TEXTUREFLAT;
    _nj_context_[DEBUG_CONTEXT].pTextureSurfaceDesc = (PKMSURFACEDESC)&font;
    _nj_context_[DEBUG_CONTEXT].bUseSpecular = FALSE;
    _nj_context_[DEBUG_CONTEXT].bUseAlpha = TRUE;
    kmProcessVertexRenderState(&_nj_context_[DEBUG_CONTEXT]);
    global_param = _nj_context_[DEBUG_CONTEXT].GLOBALPARAMBUFFER;
    isp_param = _nj_context_[DEBUG_CONTEXT].ISPPARAMBUFFER;
    tsp_param = _nj_context_[DEBUG_CONTEXT].TSPPARAMBUFFER;
    tex_param = _nj_context_[DEBUG_CONTEXT].TexturePARAMBUFFER;
}

void njExitPrint(void)
{
    kmFreeTexture((PKMSURFACEDESC)&font);
}

void njPrintSize(Uint16 size)
{
    print_size = size;
}

void njPrintColor(Uint32 c)
{
    print_color = c;
}

/* step print_sq back a word and store value there */
#define SQ_PUT(type, value) (*(type *)--print_sq = (value))

/* the characters in each row of the font, which has as many rows */
#define FONT_COLUMNS 16
/* a character's side in texture coordinates */
#define FONT_CELL (1.0F / FONT_COLUMNS)

/* send character c, its top left corner at x, y in pixels, as a strip of four
 * 32-byte vertices. each vertex is written from its color back to its control
 * word, then sent by the prefetch of its first word */
void njCharacter(Sint16 x, Sint16 y, char c)
{
    Uint8 ch;
    Sint16 col;
    Sint16 row;
    Float left;
    Float top;
    Float right;
    Float bottom;

    /* unsigned, so that codes 128 to 255 take rows 8 to 15, the texture's
     * bottom half */
    ch = c;
    col = ch % FONT_COLUMNS;
    row = ch / FONT_COLUMNS;
    left = col * FONT_CELL;
    top = row * FONT_CELL;
    right = left + FONT_CELL;
    bottom = top + FONT_CELL;
    print_sq += 7;
    SQ_PUT(Uint32, print_color);
    SQ_PUT(Float, top);
    SQ_PUT(Float, left);
    SQ_PUT(Float, 1.0F);
    SQ_PUT(Float, y);
    SQ_PUT(Float, x);
    SQ_PUT(Uint32, KM_VERTEXPARAM_NORMAL);
    prefetch(print_sq);
    print_sq += 8 + 7;
    SQ_PUT(Uint32, print_color);
    SQ_PUT(Float, bottom);
    SQ_PUT(Float, left);
    SQ_PUT(Float, 1.0F);
    SQ_PUT(Float, y + print_size);
    SQ_PUT(Float, x);
    SQ_PUT(Uint32, KM_VERTEXPARAM_NORMAL);
    prefetch(print_sq);
    print_sq += 8 + 7;
    SQ_PUT(Uint32, print_color);
    SQ_PUT(Float, top);
    SQ_PUT(Float, right);
    SQ_PUT(Float, 1.0F);
    SQ_PUT(Float, y);
    SQ_PUT(Float, x + print_size);
    SQ_PUT(Uint32, KM_VERTEXPARAM_NORMAL);
    prefetch(print_sq);
    print_sq += 8 + 7;
    SQ_PUT(Uint32, print_color);
    SQ_PUT(Float, bottom);
    SQ_PUT(Float, right);
    SQ_PUT(Float, 1.0F);
    SQ_PUT(Float, y + print_size);
    SQ_PUT(Float, x + print_size);
    SQ_PUT(Uint32, KM_VERTEXPARAM_ENDOFSTRIP);
    prefetch(print_sq);
    print_sq += 8;
}

/* point the store queues at the translucent list and send the font's polygon
 * header. _nj_current_texture becomes -2, so that Ninja's next textured draw
 * sets its texture again */
/* @hack spelling: _nj_vertex_buffer_ through its address, here and in
 * PRINT_CLOSE, which keeps each load of the address beside its use (0x8c622c1c
 * in njPrintD); as _nj_vertex_buffer_ itself, njPrintD loads it among its
 * register saves, the section runs 16 bytes short, and in the five prints
 * that use it, 610 of 857 words differ */
#define PRINT_OPEN()                                                           \
    _nj_current_texture = -2;                                                  \
    list_ptr = (Uint32)(&_nj_vertex_buffer_)->pCurrentPtr[KM_TRANS_POLYGON];   \
    list_top = list_ptr & LIST_TOP;                                            \
    sq_area = SQ_AREA(list_ptr);                                               \
    QACR0 = sq_area;                                                           \
    QACR1 = sq_area;                                                           \
    print_sq = (Uint32 *)((list_ptr & SQ_ADDRESS) | SQ_BASE);                  \
    print_sq += 4;                                                             \
    SQ_PUT(Uint32, tex_param);                                                 \
    SQ_PUT(Uint32, tsp_param);                                                 \
    SQ_PUT(Uint32, isp_param);                                                 \
    SQ_PUT(Uint32, global_param);                                              \
    prefetch(print_sq);                                                        \
    print_sq += 8;

/* move the translucent list's write pointer to the end of what the print sent,
 * in the list's own area */
#define PRINT_CLOSE()                                                          \
    ((&_nj_vertex_buffer_)->pCurrentPtr[KM_TRANS_POLYGON] =                    \
         (Uint32 *)(((Uint32)print_sq & SQ_ADDRESS) | list_top))

void njPrintC(Int loc, const char *s)
{
    Sint16 x;
    Sint16 y;
    Sint16 length;
    Sint16 i;

    PRINT_OPEN();
    x = (Sint16)(loc >> 16) * print_size;
    y = (Sint16)loc * print_size;
    length = strlen(s);
    for (i = 0; i < length; i++) {
        njCharacter(x, y, s[i]);
        x += print_size;
    }
    PRINT_CLOSE();
}

void njPrintD(Int loc, Int val, Int digit)
{
    Int d = 0;
    Sint16 x;
    Sint16 y;

    PRINT_OPEN();
    x = (Sint16)(loc >> 16) * print_size;
    y = (Sint16)loc * print_size;
    if (val < 0) {
        njCharacter(x, y, '-');
        val = -val;
        x += print_size;
    }
    for (; digit > 0; digit--) {
        switch (digit) {
        case 1:
            d = val % 10;
            break;
        case 2:
            d = val / 10;
            val -= d * 10;
            break;
        case 3:
            d = val / 100;
            val -= d * 100;
            break;
        case 4:
            d = val / 1000;
            val -= d * 1000;
            break;
        case 5:
            d = val / 10000;
            val -= d * 10000;
            break;
        case 6:
            d = val / 100000;
            val -= d * 100000;
            break;
        case 7:
            d = val / 1000000;
            val -= d * 1000000;
            break;
        case 8:
            d = val / 10000000;
            val -= d * 10000000;
            break;
        case 9:
            d = val / 100000000;
            val -= d * 100000000;
            break;
        default:
            d = val / 1000000000;
            val -= d * 1000000000;
            break;
        }
        if (d > 9) {
            d = 0;
        }
        njCharacter(x, y, '0' + d);
        x += print_size;
    }
    PRINT_CLOSE();
}

void njPrintB(Int loc, Uint32 val, Int digit)
{
    Sint16 x;
    Sint16 y;
    Int i;

    PRINT_OPEN();
    x = (Sint16)(loc >> 16) * print_size;
    y = (Sint16)loc * print_size;
    for (i = digit - 1; i >= 0; i--) {
        njCharacter(x, y, '0' + ((val >> i) & 1));
        x += print_size;
    }
    PRINT_CLOSE();
}

void njPrintH(Int loc, Uint32 val, Int digit)
{
    Sint16 x;
    Sint16 y;
    Int d;
    Int i;

    PRINT_OPEN();
    x = (Sint16)(loc >> 16) * print_size;
    y = (Sint16)loc * print_size;
    for (i = digit - 1; i >= 0; i--) {
        d = (val >> (i * 4)) & 0xf;
        njCharacter(x, y, print_digits[d]);
        x += print_size;
    }
    PRINT_CLOSE();
}

/* print val with digit places before the point, padded with zeros, and five
 * after it */
void njPrintF(Int loc, Float val, Int digit)
{
    char buf[20];
    Sint16 x;
    Sint16 y;
    Int length;
    Int point;
    Int pad;
    Int padded;
    Int i;

    PRINT_OPEN();
    x = (Sint16)(loc >> 16) * print_size;
    y = (Sint16)loc * print_size;
    if (val < 0.0F) {
        njCharacter(x, y, '-');
        val *= -1.0F;
        x += print_size;
    }
    sprintf(buf, "%0.*f", 5, val);
    length = strlen(buf);
    for (point = 0; point < length; point++) {
        if (buf[point] == '.') {
            break;
        }
    }
    pad = digit - point;
    padded = length + pad;
    /* @bug memory: a val with more than digit places before its point
     * makes pad negative, and the shift writes -pad bytes before buf (MOV.B
     * R3,@-R7 at 0x8c6231fe). each byte the shift copies then comes from -pad
     * bytes above it, so buf fills with repeats of its last -pad bytes, the
     * NUL among them, and the print draws those */
    for (i = padded; i >= pad; i--) {
        buf[i] = buf[length--];
    }
    for (i = 0; i < pad; i++) {
        buf[i] = '0';
    }
    for (i = 0; i < padded; i++) {
        njCharacter(x, y, buf[i]);
        x += print_size;
    }
    PRINT_CLOSE();
}

void njPrint(Int loc, const char *fmt, ...)
{
    char buf[128];
    va_list args;

    va_start(args, fmt);
    vsprintf(buf, fmt, args);
    va_end(args);
    njPrintC(loc, buf);
}
