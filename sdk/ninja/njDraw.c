#include <ninja.h> /* NOLINT(misc-include-cleaner) */

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
