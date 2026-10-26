#include <ninja.h> /* NOLINT(misc-include-cleaner) */

/* clang-format off */
#ifndef __clang__
/* set _nj_screen_.dist to _nj_screen_.w / (2 tan(ang / 2)), the distance at
 * which the screen's width spans an angle of view of ang. unlike njSetCamera,
 * it leaves out _nj_scr_aspect_x_ */
/* @asm epilogue: dead-return, SHC's unreachable RTS at 0x8c65b394 follows the
 * body's own return at 0x8c65b386 */
#pragma inline_asm(njSetPerspective)
void njSetPerspective(Angle ang)
{
        .IMPORT     __nj_screen_
        .IMPORT     _njTan
        STS.L       PR,@-R15
        MOV.L       #_njTan,R3
        JSR         @R3
        SHAR        R4
        MOV.L       #__nj_screen_+4,R3
        FMOV        FR0,FR3
        FMOV.S      @R3,FR2
        FADD        FR3,FR0
        FDIV        FR0,FR2
        ADD         #-4,R3
        LDS.L       @R15+,PR
        RTS
        FMOV.S      FR2,@R3
}
#endif
/* clang-format on */
