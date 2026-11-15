/* njRotate: set m to R * m, where R rotates by ang about v, or the current
 * matrix, XMTRX, where m is null. make pc compiles this in place of
 * sdk/ninja/njRotate.c, whose body is SH-4 assembly.
 *
 * the body turns v onto z, about x by (s1, c1) and then about y by
 * (s2, c2), rotates by ang about z, and undoes both turns. (s1, c1) is v's
 * y and z made a unit vector, and (s2, c2) is v's x and the length of its
 * y and z, as v is unit length. each comes from FSRRA and FSQRT, or, where
 * a part is 0, is a unit along the other, as the body's tests give them.
 *
 * with XMTRX, each step is njrotate.h's two FTRVs. with a matrix, each step
 * turns two of its rows by scalar products, column by column, in one of
 * the body's two orders: turn_plus or turn_minus */
#include <ninja.h>

#include "xbox/include/sh4.h"
#include "xbox/sdk/ninja/njrotate.h"

/* rows r1 and r2 of m, each column's a and b: r1 = a c + b s and
 * r2 = b c - a s */
static void turn_plus(Float *m, int r1, int r2, Float c, Float s)
{
    int k;

    for (k = 0; k < 4; k++) {
        Float a = m[4 * r1 + k];
        Float b = m[4 * r2 + k];

        m[4 * r1 + k] = b * s + a * c;
        m[4 * r2 + k] = b * c - a * s;
    }
}

/* r1 = a c - b s and r2 = b c + a s */
static void turn_minus(Float *m, int r1, int r2, Float c, Float s)
{
    int k;

    for (k = 0; k < 4; k++) {
        Float a = m[4 * r1 + k];
        Float b = m[4 * r2 + k];

        m[4 * r1 + k] = a * c - b * s;
        m[4 * r2 + k] = b * c + a * s;
    }
}

/* a unit along one axis where the other part of a pair is 0: 1 where the
 * part is over 0, and -1 otherwise, as the body's FCMP/GT and FNEG give it */
static Float unit_sign(Float part)
{
    return part > 0.0F ? 1.0F : -1.0F;
}

void(njRotate)(NJS_MATRIX *m, NJS_VECTOR *v, Angle ang)
{
    Float vx = v->x;
    Float vy = v->y;
    Float vz = v->z;
    Float yz2 = sh4_fmac(vz, vz, vy * vy);
    Float s1;
    Float c1;
    Float s2;
    Float c2;
    Float length;
    Float s;
    Float c;

    if (vz == 0.0F && vy != 0.0F) {
        s1 = unit_sign(vy);
        c1 = 0.0F;
    } else if (vy == 0.0F) {
        s1 = 0.0F;
        c1 = unit_sign(vz);
    } else {
        Float scale = _builtin_fsrra(yz2);

        s1 = vy * scale;
        c1 = vz * scale;
    }
    length = __builtin_sqrtf(yz2);
    if (length == 0.0F && vx != 0.0F) {
        s2 = unit_sign(vx);
        c2 = 0.0F;
    } else if (vx == 0.0F) {
        s2 = 0.0F;
        c2 = unit_sign(length);
    } else {
        s2 = vx;
        c2 = length;
    }
    _builtin_fsca(ang, &s, &c);
    if (m == NULL) {
        {
            const Float a[4] = {0.0F, c1, -s1, 0.0F};
            const Float b[4] = {0.0F, s1, c1, 0.0F};

            nj_rotate_axes(NULL, 1, 2, a, b);
        }
        {
            const Float a[4] = {c2, 0.0F, -s2, 0.0F};
            const Float b[4] = {s2, 0.0F, c2, 0.0F};

            nj_rotate_axes(NULL, 0, 2, a, b);
        }
        {
            const Float a[4] = {c, s, 0.0F, 0.0F};
            const Float b[4] = {-s, c, 0.0F, 0.0F};

            nj_rotate_axes(NULL, 0, 1, a, b);
        }
        {
            const Float a[4] = {c2, 0.0F, s2, 0.0F};
            const Float b[4] = {-s2, 0.0F, c2, 0.0F};

            nj_rotate_axes(NULL, 0, 2, a, b);
        }
        {
            const Float a[4] = {0.0F, c1, s1, 0.0F};
            const Float b[4] = {0.0F, -s1, c1, 0.0F};

            nj_rotate_axes(NULL, 1, 2, a, b);
        }
        return;
    }
    {
        Float *rows = (Float *)m;

        turn_plus(rows, 1, 2, c1, -s1);
        turn_minus(rows, 0, 2, c2, s2);
        turn_plus(rows, 0, 1, c, s);
        turn_minus(rows, 0, 2, c2, -s2);
        turn_plus(rows, 1, 2, c1, s1);
    }
}
