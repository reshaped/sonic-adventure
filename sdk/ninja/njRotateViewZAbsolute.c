#include <ninja.h> /* NOLINT(misc-include-cleaner) */

/* clang-format off */
#ifndef __clang__
/* set v's roll to its absolute roll, aroll, plus ang. a null v selects the
 * current view */
/* @asm epilogue: dead-return, SHC's unreachable RTS at 0x8c65b350 follows the
 * body's own return at 0x8c65b348 */
#pragma inline_asm(njRotateViewZAbsolute)
void njRotateViewZAbsolute(NJS_VIEW *v, Angle ang)
{
        .IMPORT     __nj_view_
        TST         R4,R4
        MOV         #116,R0
        BF          ?have_view
        MOV.L       #__nj_view_,R2
        MOV.L       @R2,R4
?have_view:
        MOV.L       @(R0,R4),R3
        ADD         R5,R3
        MOV         #88,R0
        RTS
        MOV.L       R3,@(R0,R4)
}
#endif
/* clang-format on */
