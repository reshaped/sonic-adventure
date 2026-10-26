#include <ninja.h> /* NOLINT(misc-include-cleaner) */

/* clang-format off */
#ifndef __clang__
/* set the near and far clip planes to z = n and z = f, each clamped to
 * [-65535, -1]. _nj_near_clip_ and _nj_far_clip_ take them negated, as
 * distances */
/* @asm epilogue: dead-return, SHC's unreachable RTS at 0x8c65b21c follows the
 * body's own return at 0x8c65b206 */
#pragma inline_asm(njClipZ)
void njClipZ(Float n, Float f)
{
        .IMPORT     __nj_far_clip_
        .IMPORT     __nj_near_clip_
        MOVA        ?near_limit,R0
        FMOV.S      @R0,FR0
        MOVA        ?far_limit,R0
        FMOV.S      @R0,FR1
        FCMP/GT     FR0,FR4
        BF/S        ?near_low
        FCMP/GT     FR4,FR1
        FMOV        FR0,FR4
?near_low:
        BF/S        ?far_high
        FCMP/GT     FR0,FR5
        FMOV        FR1,FR4
?far_high:
        BF/S        ?far_low
        FCMP/GT     FR5,FR1
        FMOV        FR0,FR5
?far_low:
        BF/S        ?store
        FNEG        FR4
        FMOV        FR1,FR5
?store:
        FNEG        FR5
        MOV.L       ?near_clip,R2
        MOV.L       ?far_clip,R3
        FMOV.S      FR4,@R2
        RTS
        FMOV.S      FR5,@R3
        .RES.W      1                       ; pads with 0, not .ALIGN's NOP
?near_limit:
        .FDATA.S    F'-1.0
?far_limit:
        .FDATA.S    F'-65535.0
?near_clip:
        .DATA.L     __nj_near_clip_
?far_clip:
        .DATA.L     __nj_far_clip_
}
#endif
/* clang-format on */
