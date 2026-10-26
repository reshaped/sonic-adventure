#include <ninja.h> /* NOLINT(misc-include-cleaner) */

/* clang-format off */
#ifndef __clang__
/* set v's matrix to the unit matrix, its position and absolute position to the
 * origin, its direction and absolute direction to (0, 0, -1), and its roll and
 * absolute roll to 0 */
/* @asm epilogue: dead-return, SHC's unreachable RTS at 0x8c65b288 follows the
 * body's own return at 0x8c65b280 */
#pragma inline_asm(njInitView)
void njInitView(NJS_VIEW *v)
{
        .IMPORT     __unit_matrix_
        MOV.L       #__unit_matrix_,R5
        ADD         #120,R4
        FLDI1       FR1
        FLDI0       FR0
        FNEG        FR1
        FMOV.S      FR0,@-R4
        FMOV.S      FR1,@-R4
        FMOV.S      FR0,@-R4
        FMOV.S      FR0,@-R4
        FMOV.S      FR0,@-R4
        FMOV.S      FR0,@-R4
        FMOV.S      FR0,@-R4
        FMOV.S      FR0,@-R4
        FMOV.S      FR1,@-R4
        FMOV.S      FR0,@-R4
        FMOV.S      FR0,@-R4
        FMOV.S      FR0,@-R4
        MOV         R5,R6
        FMOV.S      FR0,@-R4
        ADD         #32,R6
        FMOV.S      FR0,@-R4
        FSCHG
        FMOV        @R6+,DR0
        FMOV        @R6+,DR2
        FMOV        @R6+,DR4
        FMOV        @R6+,DR6
        FMOV        @R5+,DR8
        FMOV        @R5+,DR10
        FSCHG
        FMOV.S      FR7,@-R4
        FMOV.S      FR6,@-R4
        FMOV.S      FR5,@-R4
        FMOV.S      FR4,@-R4
        FMOV.S      FR3,@-R4
        FMOV.S      FR2,@-R4
        FMOV.S      FR1,@-R4
        FMOV.S      FR0,@-R4
        FSCHG
        FMOV        @R5+,DR6
        FMOV        @R5+,DR4
        FSCHG
        FMOV.S      FR5,@-R4
        FMOV.S      FR4,@-R4
        FMOV.S      FR7,@-R4
        FMOV.S      FR6,@-R4
        FMOV.S      FR11,@-R4
        FMOV.S      FR10,@-R4
        FMOV.S      FR9,@-R4
        RTS
        FMOV.S      FR8,@-R4
}
#endif
/* clang-format on */
