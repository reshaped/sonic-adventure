#include <ninja.h> /* NOLINT(misc-include-cleaner) */

/* clang-format off */
#ifndef __clang__
/* set c's angle of view to ang */
/* @asm epilogue: dead-return, SHC's unreachable RTS at 0x8c66874c follows the
 * body's own return at 0x8c668748 */
#pragma inline_asm(njSetCameraAngle)
void njSetCameraAngle(NJS_CAMERA *c, Angle ang)
{
        RTS
        MOV.L       R5,@(28,R4)
}
#endif
/* clang-format on */
