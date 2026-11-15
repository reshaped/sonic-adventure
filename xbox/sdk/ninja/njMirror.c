/* njMirror: set m to a reflection in pl followed by m, or the current
 * matrix, XMTRX, where m is null. make pc compiles this in place of
 * sdk/ninja/njMirror.c, whose body is SH-4 assembly; its comment gives the
 * turns and the plane normals they suit.
 *
 * the angle about x is asinf of the normal's y times FSRRA of y^2 + z^2,
 * and the angle about y asinf of its x, each times 10430.380859375,
 * truncated by FTRC and cut to 16 bits by EXTU.W, as the body forms them.
 * each call names its function in parentheses, so that it reaches the
 * function, as the body's JSRs do, and not a macro of its name */
#include <mathf.h>
#include <ninja.h>

#include "xbox/include/sh4.h"

/* a radian's Ninja angles, as the body's ?to_angle */
#define TO_ANGLE 10430.380859375F

void(njMirror)(NJS_MATRIX *m, NJS_PLANE *pl)
{
    Float ny = pl->vy;
    Float nz = pl->vz;
    Angle about_x;
    Angle about_y;

    about_x = (Angle)(Uint16)sh4_ftrc(
        asinf(ny * _builtin_fsrra(ny * ny + nz * nz)) * TO_ANGLE);
    about_y = (Angle)(Uint16)sh4_ftrc(asinf(pl->vx) * TO_ANGLE);
    (njTranslate)(m, pl->px, pl->py, pl->pz);
    (njRotateX)(m, -about_x);
    (njRotateY)(m, about_y);
    (njScale)(m, 1.0F, 1.0F, -1.0F);
    (njRotateY)(m, -about_y);
    (njRotateX)(m, about_x);
    (njTranslate)(m, -pl->px, -pl->py, -pl->pz);
}
