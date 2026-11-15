/* njInitView: set v's matrix to the unit matrix, its position and absolute
 * position to the origin, its direction and absolute direction to
 * (0, 0, -1), and its roll and absolute roll to 0. make pc compiles this in
 * place of sdk/ninja/njInitView.c, whose body is SH-4 assembly */
#include <ninja.h>

void(njInitView)(NJS_VIEW *v)
{
    const Float *unit = (const Float *)&_unit_matrix_;
    Float *m = (Float *)&v->m;
    int k;

    v->aroll = 0;
    v->avz = -1.0F;
    v->avy = 0.0F;
    v->avx = 0.0F;
    v->apz = 0.0F;
    v->apy = 0.0F;
    v->apx = 0.0F;
    v->roll = 0;
    v->vz = -1.0F;
    v->vy = 0.0F;
    v->vx = 0.0F;
    v->pz = 0.0F;
    v->py = 0.0F;
    v->px = 0.0F;
    for (k = 0; k < 16; k++) {
        m[k] = unit[k];
    }
}
