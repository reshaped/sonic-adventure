#include <ninja.h> /* NOLINT(misc-include-cleaner) */

#ifdef GCC
#include <kamui.h>
#include <machine.h>

/* make gcc builds njDrawPolygon, njDrawTexture and njDrawTextureMemList from
 * the C below, for the 2D draws' squeeze of x that port.h describes, and for
 * the split frame, which keeps each draw for a picture to come. each sends
 * the vertices the assembly under the #else sends, but for that x */

Sint32 njSetkmTextureNumG(Uint32 globalIndex);
Sint32 njSetkmTextureMemList(Uint32 n);

/* step sq back a word and store value there */
#define SQ_PUT(type, value) (*(type *)--sq = (value))

enum {
    POLYGON_WORDS = sizeof(NJS_POLYGON_VTX) / sizeof(Float),
    TEXTURE_WORDS = sizeof(NJS_TEXTURE_VTX) / sizeof(Float),
};

/* send count vertices, count at least 1, from vertex as one strip from sq
 * on, and return the address past the last. words is a vertex's length,
 * POLYGON_WORDS or TEXTURE_WORDS. a vertex goes into the queue from its
 * color, word 6, down to its control word, as the assembly writes it. the
 * PREF at word 0 then sends the queue. a polygon's vertex leaves the queue's
 * words 4 and 5, where a texture's u and v go, as the queue held them. x
 * takes the 2D draws' squeeze where squeeze is TRUE. send_strip is
 * noinline, so that the three draws share one copy */
static __attribute__((noinline)) Uint32 *
send_strip(Uint32 *sq, const Float *vertex, Int count, Int words, Int squeeze)
{
    Int i;

    for (i = 0; i < count; i++, vertex += words) {
        sq += 7;
        SQ_PUT(Uint32, *(const Uint32 *)&vertex[words - 1]);
        if (words == TEXTURE_WORDS) {
            SQ_PUT(Float, vertex[4]);
            SQ_PUT(Float, vertex[3]);
        } else {
            sq -= 2;
        }
        SQ_PUT(Float, vertex[2]);
        SQ_PUT(Float, vertex[1]);
        SQ_PUT(Float, squeeze ? port_wide_x(vertex[0]) : vertex[0]);
        SQ_PUT(Uint32, i == count - 1 ? KM_VERTEXPARAM_ENDOFSTRIP
                                      : KM_VERTEXPARAM_NORMAL);
        prefetch(sq);
        sq += 8;
    }
    return sq;
}

void njDrawPolygon(NJS_POLYGON_VTX *polygon, Int count, Int trans)
{
    /* a polygon that covers the screen keeps x */
    Int squeeze = port_wide_squeezes() &&
                  !port_wide_covers(&polygon->x, count, POLYGON_WORDS);
    Uint32 *sq;

    present_record(PRESENT_POLYGON, polygon, count, 0, 0.0F, (Uint32)trans);
    njGetkmPointer();
    if (trans) {
        njSetkmColorMode(COLOR_MODE_TRANS);
        sq = njStartVertexNonTexTrans();
    } else {
        njSetkmColorMode(COLOR_MODE_OPAQUE);
        sq = njStartVertexNonTexOpaque();
    }
    njEndVertex(send_strip(sq, &polygon->x, count, POLYGON_WORDS, squeeze));
    njSetkmPointer();
}

/* draw polygon's count vertices as njDrawTexture and njDrawTextureMemList
 * do. the two differ only in set_texture, the call that makes texture tex
 * current */
static void texture_draw(NJS_TEXTURE_VTX *polygon, Int count, Int tex, Int flag,
                         Sint32 (*set_texture)(Uint32 tex))
{
    Uint32 *sq;

    njGetkmPointer();
    njSetkmColorMode((flag ? COLOR_MODE_TRANS : COLOR_MODE_OPAQUE) |
                     COLOR_MODE_TEXTURE);
    set_texture(tex);
    if (flag) {
        sq = njStartVertexTexTrans();
    } else {
        sq = njStartVertexTexOpaque();
    }
    njEndVertex(send_strip(sq, &polygon->x, count, TEXTURE_WORDS,
                           port_wide_squeezes()));
    njSetkmPointer();
}

void njDrawTexture(NJS_TEXTURE_VTX *polygon, Int count, Int tex, Int flag)
{
    present_record(PRESENT_TEXTURE, polygon, count, tex, 0.0F, (Uint32)flag);
    texture_draw(polygon, count, tex, flag, njSetkmTextureNumG);
}

void njDrawTextureMemList(NJS_TEXTURE_VTX *polygon, Int count, Int tex,
                          Int flag)
{
    present_record(PRESENT_TEXTURE_MEMLIST, polygon, count, tex, 0.0F,
                   (Uint32)flag);
    texture_draw(polygon, count, tex, flag, njSetkmTextureMemList);
}
#else
/* clang-format off */
#ifndef __clang__
/* @asm open: its dead RTS at 0x8c623504 follows a tail jump at 0x8c6234f4,
 * which plain C leaves too */
#pragma inline_asm(njDrawPolygon)
void njDrawPolygon(NJS_POLYGON_VTX *polygon, Int count, Int trans)
{
        .IMPORT     _njEndVertex
        .IMPORT     _njGetkmPointer
        .IMPORT     _njSetkmColorMode
        .IMPORT     _njSetkmPointer
        .IMPORT     _njStartVertexNonTexOpaque
        .IMPORT     _njStartVertexNonTexTrans
        STS.L       PR,@-R15
        MOV.L       R4,@-R15
        MOV.L       R5,@-R15
        MOV.L       #_njGetkmPointer,R3
        TST         R6,R6
        BT          ?opaque
        JSR         @R3
        NOP
        MOV.L       #_njSetkmColorMode,R3
        MOV.L       #H'02100002,R4
        JSR         @R3
        NOP
        MOV.L       #_njStartVertexNonTexTrans,R3
        JSR         @R3
        NOP
        BRA         ?start
        NOP
?opaque:
        JSR         @R3
        NOP
        MOV.L       #_njSetkmColorMode,R3
        MOV.L       #H'00080002,R4
        JSR         @R3
        NOP
        MOV.L       #_njStartVertexNonTexOpaque,R3
        JSR         @R3
        NOP
?start:
        MOV.L       @R15+,R1
        MOV.L       @R15+,R2
        MOV.L       #H'E0000000,R3
        ADD         #28,R0
?next:
        MOV.L       @R2+,R4
        MOV.L       @R2+,R5
        MOV.L       @R2+,R6
        MOV.L       @R2+,R7
        PREF        @R2
        DT          R1
        BT          ?last
        MOV.L       R7,@-R0
        ADD         #-8,R0
        MOV.L       R6,@-R0
        MOV.L       R5,@-R0
        MOV.L       R4,@-R0
        MOV.L       R3,@-R0
        PREF        @R0
        BRA         ?next
        ADD         #60,R0
?last:
        MOV.L       #H'F0000000,R3
        MOV.L       R7,@-R0
        ADD         #-8,R0
        MOV.L       R6,@-R0
        MOV.L       R5,@-R0
        MOV.L       R4,@-R0
        MOV.L       R3,@-R0
        PREF        @R0
        MOV.L       #_njEndVertex,R3
        ADD         #32,R0
        JSR         @R3
        MOV         R0,R4
        MOV.L       #_njSetkmPointer,R3
        LDS.L       @R15+,PR
        JMP         @R3
        NOP
}

/* @asm open: its dead RTS at 0x8c6235d8 follows a tail jump at 0x8c6235c8,
 * which plain C leaves too */
#pragma inline_asm(njDrawTexture)
void njDrawTexture(NJS_TEXTURE_VTX *polygon, Int count, Int tex, Int flag)
{
        .IMPORT     _njSetkmTextureNumG
        .IMPORT     _njStartVertexTexOpaque
        .IMPORT     _njStartVertexTexTrans
        MOV.L       R9,@-R15
        MOV.L       R8,@-R15
        STS.L       PR,@-R15
        MOV.L       R4,@-R15
        MOV.L       R5,@-R15
        MOV.L       R6,@-R15
        MOV.L       #_njGetkmPointer,R3
        TST         R7,R7
        BT          ?opaque
        JSR         @R3
        NOP
        MOV.L       #_njSetkmColorMode,R3
        MOV.L       #H'0210000A,R4
        JSR         @R3
        NOP
        MOV.L       #_njSetkmTextureNumG,R3
        JSR         @R3
        MOV.L       @R15+,R4
        MOV.L       #_njStartVertexTexTrans,R3
        JSR         @R3
        NOP
        BRA         ?start
        NOP
?opaque:
        JSR         @R3
        NOP
        MOV.L       #_njSetkmColorMode,R3
        MOV.L       #H'0008000A,R4
        JSR         @R3
        NOP
        MOV.L       #_njSetkmTextureNumG,R3
        JSR         @R3
        MOV.L       @R15+,R4
        MOV.L       #_njStartVertexTexOpaque,R3
        JSR         @R3
        NOP
?start:
        MOV.L       @R15+,R1
        MOV.L       @R15+,R2
        MOV.L       #H'E0000000,R3
        ADD         #28,R0
?next:
        MOV.L       @R2+,R4
        MOV.L       @R2+,R5
        MOV.L       @R2+,R6
        MOV.L       @R2+,R7
        MOV.L       @R2+,R8
        MOV.L       @R2+,R9
        PREF        @R2
        DT          R1
        BT          ?last
        MOV.L       R9,@-R0
        MOV.L       R8,@-R0
        MOV.L       R7,@-R0
        MOV.L       R6,@-R0
        MOV.L       R5,@-R0
        MOV.L       R4,@-R0
        MOV.L       R3,@-R0
        PREF        @R0
        BRA         ?next
        ADD         #60,R0
?last:
        MOV.L       #H'F0000000,R3
        MOV.L       R9,@-R0
        MOV.L       R8,@-R0
        MOV.L       R7,@-R0
        MOV.L       R6,@-R0
        MOV.L       R5,@-R0
        MOV.L       R4,@-R0
        MOV.L       R3,@-R0
        PREF        @R0
        MOV.L       #_njEndVertex,R3
        ADD         #32,R0
        JSR         @R3
        MOV         R0,R4
        MOV.L       #_njSetkmPointer,R3
        LDS.L       @R15+,PR
        MOV.L       @R15+,R8
        JMP         @R3
        MOV.L       @R15+,R9
}

/* @asm open: its dead RTS at 0x8c6236ac follows a tail jump at 0x8c62369c,
 * which plain C leaves too */
#pragma inline_asm(njDrawTextureMemList)
void njDrawTextureMemList(NJS_TEXTURE_VTX *polygon, Int count, Int tex,
                          Int flag)
{
        .IMPORT     _njSetkmTextureMemList
        MOV.L       R9,@-R15
        MOV.L       R8,@-R15
        STS.L       PR,@-R15
        MOV.L       R4,@-R15
        MOV.L       R5,@-R15
        MOV.L       R6,@-R15
        MOV.L       #_njGetkmPointer,R3
        TST         R7,R7
        BT          ?opaque
        JSR         @R3
        NOP
        MOV.L       #_njSetkmColorMode,R3
        MOV.L       #H'0210000A,R4
        JSR         @R3
        NOP
        MOV.L       #_njSetkmTextureMemList,R3
        JSR         @R3
        MOV.L       @R15+,R4
        MOV.L       #_njStartVertexTexTrans,R3
        JSR         @R3
        NOP
        BRA         ?start
        NOP
?opaque:
        JSR         @R3
        NOP
        MOV.L       #_njSetkmColorMode,R3
        MOV.L       #H'0008000A,R4
        JSR         @R3
        NOP
        MOV.L       #_njSetkmTextureMemList,R3
        JSR         @R3
        MOV.L       @R15+,R4
        MOV.L       #_njStartVertexTexOpaque,R3
        JSR         @R3
        NOP
?start:
        MOV.L       @R15+,R1
        MOV.L       @R15+,R2
        MOV.L       #H'E0000000,R3
        ADD         #28,R0
?next:
        MOV.L       @R2+,R4
        MOV.L       @R2+,R5
        MOV.L       @R2+,R6
        MOV.L       @R2+,R7
        MOV.L       @R2+,R8
        MOV.L       @R2+,R9
        PREF        @R2
        DT          R1
        BT          ?last
        MOV.L       R9,@-R0
        MOV.L       R8,@-R0
        MOV.L       R7,@-R0
        MOV.L       R6,@-R0
        MOV.L       R5,@-R0
        MOV.L       R4,@-R0
        MOV.L       R3,@-R0
        PREF        @R0
        BRA         ?next
        ADD         #60,R0
?last:
        MOV.L       #H'F0000000,R3
        MOV.L       R9,@-R0
        MOV.L       R8,@-R0
        MOV.L       R7,@-R0
        MOV.L       R6,@-R0
        MOV.L       R5,@-R0
        MOV.L       R4,@-R0
        MOV.L       R3,@-R0
        PREF        @R0
        MOV.L       #_njEndVertex,R3
        ADD         #32,R0
        JSR         @R3
        MOV         R0,R4
        MOV.L       #_njSetkmPointer,R3
        LDS.L       @R15+,PR
        MOV.L       @R15+,R8
        JMP         @R3
        MOV.L       @R15+,R9
}
#endif
/* clang-format on */
#endif
