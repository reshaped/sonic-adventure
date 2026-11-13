/* njRotate on SH4ZAM. make dc compiles this in place of
 * sdk/ninja/njRotate.c */
#include <ninja.h>

/* set XMTRX's columns first and second to XMTRX times a and times b. both
 * products are taken before either write */
static inline __attribute__((always_inline)) void
turn_columns(unsigned first, shz_vec4_t a, unsigned second, shz_vec4_t b)
{
    shz_vec4_t to_first = shz_xmtrx_transform_vec4(a);
    shz_vec4_t to_second = shz_xmtrx_transform_vec4(b);

    shz_xmtrx_write_col(first, to_first);
    shz_xmtrx_write_col(second, to_second);
}

/* the shipped body's turns differ in which products they add and subtract.
 * this one form serves them all, with the sine negated where needed. x - y
 * gives the bits of x + -y */
static inline __attribute__((always_inline)) void
turn_rows(NJS_MATRIX *m, unsigned first, unsigned second, Float cosine,
          Float sine)
{
    int i;

    for (i = 0; i < 4; i++) {
        Float a = (*m)[4 * first + i];
        Float b = (*m)[4 * second + i];

        (*m)[4 * first + i] = a * cosine + b * sine;
        (*m)[4 * second + i] = b * cosine - a * sine;
    }
}

/* set m to R * m, where R rotates by ang about v: it turns v onto z, about x
 * and then y, rotates by ang about z, and undoes both turns. v must be unit
 * length. a null m selects the current matrix, in XMTRX.
 *
 * the turns about x and y take their sines and cosines by the shipped
 * body's arithmetic and branches.
 *
 * a null m doesn't take SH4ZAM's shz_xmtrx_rotate_dir. that entry takes
 * radians, where ang is in FSCA's own unit. it also differs from this body
 * where v isn't unit length. about a zero v, this body turns by ang about z.
 * that entry scales XMTRX's first three columns by ang's cosine */
void njRotate(NJS_MATRIX *m, NJS_VECTOR *v, Angle ang)
{
    Float x = v->x;
    Float y = v->y;
    Float z = v->z;
    Float across = __builtin_fmaf(z, z, y * y);
    Float length;
    Float sin_x;
    Float cos_x;
    Float sin_y;
    Float cos_y;
    shz_sincos_t turn;

    if (z == 0.0F && y != 0.0F) {
        sin_x = y > 0.0F ? 1.0F : -1.0F;
        cos_x = 0.0F;
    } else if (y == 0.0F) {
        sin_x = 0.0F;
        cos_x = z > 0.0F ? 1.0F : -1.0F;
    } else {
        Float scale = _builtin_fsrra(across);

        sin_x = y * scale;
        cos_x = z * scale;
    }
    length = __builtin_sqrtf(across);
    if (length == 0.0F && x != 0.0F) {
        sin_y = x > 0.0F ? 1.0F : -1.0F;
        cos_y = 0.0F;
    } else if (x == 0.0F) {
        sin_y = 0.0F;
        cos_y = length > 0.0F ? 1.0F : -1.0F;
    } else {
        sin_y = x;
        cos_y = length;
    }
    turn = zam_fsca(ang);
    if (m == NULL) {
        turn_columns(1, shz_vec4_init(0.0F, cos_x, -sin_x, 0.0F), 2,
                     shz_vec4_init(0.0F, sin_x, cos_x, 0.0F));
        turn_columns(0, shz_vec4_init(cos_y, 0.0F, -sin_y, 0.0F), 2,
                     shz_vec4_init(sin_y, 0.0F, cos_y, 0.0F));
        turn_columns(0, shz_vec4_init(turn.cos, turn.sin, 0.0F, 0.0F), 1,
                     shz_vec4_init(-turn.sin, turn.cos, 0.0F, 0.0F));
        turn_columns(0, shz_vec4_init(cos_y, 0.0F, sin_y, 0.0F), 2,
                     shz_vec4_init(-sin_y, 0.0F, cos_y, 0.0F));
        turn_columns(1, shz_vec4_init(0.0F, cos_x, sin_x, 0.0F), 2,
                     shz_vec4_init(0.0F, -sin_x, cos_x, 0.0F));
    } else {
        turn_rows(m, 1, 2, cos_x, -sin_x);
        turn_rows(m, 0, 2, cos_y, -sin_y);
        turn_rows(m, 0, 1, turn.cos, turn.sin);
        turn_rows(m, 0, 2, cos_y, sin_y);
        turn_rows(m, 1, 2, cos_x, sin_x);
    }
}
