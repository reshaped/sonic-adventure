#include <ninja.h> /* NOLINT(misc-include-cleaner) */

#ifdef DREAMCAST
#ifdef NONMATCHING
#include <kamui.h>

#include "sa1/polygon.h"

/* make dc builds njDrawPolygon, njDrawTexture and njDrawTextureMemList from
 * the C below. that C is SA1's layer for Ninja's screen polygons on the
 * Dreamcast. port.h's present_record keeps each draw for a picture to come,
 * or refuses it. sa1/polygon.h's send writes the vertices the assembly under
 * the #else sends. only x differs, by the 2D draws' squeeze (dreamcast.h) */

Sint32 njSetkmTextureNumG(Uint32 globalIndex);
Sint32 njSetkmTextureMemList(Uint32 n);

/* hand count vertices from vertex to sa_present_live in format. it writes
 * them into the strip from sq on, with x squeezed where squeeze is TRUE. the
 * result is the address past the last */
ALWAYS_INLINE Uint32 *polygon_live(const void *vertex, Int count, Uint32 format,
                                   Int squeeze, Uint32 *sq)
{
    struct sa1_live_polygon state = {count, squeeze};
    sa_draw_live draw = {vertex, &state, format, 0};

    sa1_live_sq = sq;
    sa_present_live(SA_DC_ENGINE, &draw);
    return sa1_live_sq;
}

void njDrawPolygon(NJS_POLYGON_VTX *polygon, Int count, Int trans)
{
    /* a polygon that covers the screen keeps x */
    Int squeeze =
        port_wide_squeezes() &&
        !port_wide_covers(&polygon->x, count, sizeof(*polygon) / sizeof(Float));
    Uint32 *sq;

    if (!present_record(PRESENT_POLYGON, polygon, count, 0, 0.0F,
                        (Uint32)trans)) {
        return;
    }
    njGetkmPointer();
    if (trans) {
        njSetkmColorMode(COLOR_MODE_TRANS);
        sq = njStartVertexNonTexTrans();
    } else {
        njSetkmColorMode(COLOR_MODE_OPAQUE);
        sq = njStartVertexNonTexOpaque();
    }
    njEndVertex(polygon_live(polygon, count, SA_SA1_POLYGON, squeeze, sq));
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
    njEndVertex(
        polygon_live(polygon, count, SA_SA1_TEXTURE, port_wide_squeezes(), sq));
    njSetkmPointer();
}

void njDrawTexture(NJS_TEXTURE_VTX *polygon, Int count, Int tex, Int flag)
{
    if (present_record(PRESENT_TEXTURE, polygon, count, tex, 0.0F,
                       (Uint32)flag)) {
        texture_draw(polygon, count, tex, flag, njSetkmTextureNumG);
    }
}

void njDrawTextureMemList(NJS_TEXTURE_VTX *polygon, Int count, Int tex,
                          Int flag)
{
    if (present_record(PRESENT_TEXTURE_MEMLIST, polygon, count, tex, 0.0F,
                       (Uint32)flag)) {
        texture_draw(polygon, count, tex, flag, njSetkmTextureMemList);
    }
}
#else
/* clang-format off */
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
/* clang-format on */
#endif
#endif
