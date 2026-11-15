/* njDetMatrix: the determinant of m, or of the current matrix, XMTRX, where
 * m is null, expanded along the first row as its source says. make pc
 * compiles this in place of sdk/ninja/njDetMatrix.c, whose body is SH-4
 * assembly: two straight runs, one through XMTRX and one through m.
 *
 * tools/sh4lift.py wrote each run's C from the body, operation for
 * operation in its order, as
 *   tools/sh4lift.py sdk/ninja/njDetMatrix.c njDetMatrix - --fall-through
 *   tools/sh4lift.py sdk/ninja/njDetMatrix.c njDetMatrix ?memory --reg R4=in
 * keeping FR0, the result. edit the body, not these runs */
#include <ninja.h>

#include "xbox/include/sh4.h"

static Float det_current(const Float xf[16])
{
    Float fr_out[1];

    const Float t0 = xf[10] * xf[15];
    const Float t1 = xf[11] * xf[14];
    const Float t2 = t0 - t1;
    const Float t3 = xf[9] * xf[14];
    const Float t4 = xf[10] * xf[13];
    const Float t5 = t3 - t4;
    const Float t6 = xf[11] * xf[12];
    const Float t7 = xf[8] * xf[15];
    const Float t8 = t6 - t7;
    const Float t9 = xf[8] * xf[13];
    const Float t10 = xf[9] * xf[12];
    const Float t11 = t9 - t10;
    const Float t12 = xf[13] * xf[11];
    const Float t13 = xf[14] * xf[8];
    const Float t14 = xf[15] * xf[9];
    const Float t15 = xf[12] * xf[10];
    const Float t16 = t12 - t14;
    const Float t17 = t13 - t15;
    const Float t18 = t14 - t12;
    const Float t19 = t15 - t13;
    const Float t20 = t18 * xf[4];
    const Float t21 = xf[4] * t2;
    const Float t22 = xf[4] * t5;
    const Float t23 = t2 * xf[5];
    const Float t24 = xf[5] * t8;
    const Float t25 = xf[5] * t19;
    const Float t26 = t20 + t24;
    const Float t27 = t25 + t22;
    const Float t28 = t8 * xf[6];
    const Float t29 = t16 * xf[6];
    const Float t30 = xf[6] * t11;
    const Float t31 = t29 + t23;
    const Float t32 = t21 + t28;
    const Float t33 = t27 + t30;
    const Float t34 = t5 * xf[7];
    const Float t35 = t17 * xf[7];
    const Float t36 = xf[7] * t11;
    const Float t37 = t31 + t34;
    const Float t38 = t32 + t35;
    const Float t39 = t36 + t26;
    const Float t40 = t37 * xf[0];
    const Float t41 = t38 * xf[1];
    const Float t42 = t39 * xf[2];
    const Float t43 = t33 * xf[3];
    const Float t44 = t40 - t41;
    const Float t45 = t42 - t43;
    const Float t46 = t45 + t44;
    fr_out[0] = t46;
    return fr_out[0];
}

static Float det_memory(const Float *in)
{
    Float fr_out[1];

    const Float t0 = in[8];
    const Float t1 = in[9];
    const Float t2 = in[10];
    const Float t3 = in[11];
    const Float t4 = in[12];
    const Float t5 = in[13];
    const Float t6 = in[14];
    const Float t7 = in[15];
    const Float t8 = t2 * t7;
    const Float t9 = t3 * t6;
    const Float t10 = t8 - t9;
    const Float t11 = t1 * t6;
    const Float t12 = t2 * t5;
    const Float t13 = t11 - t12;
    const Float t14 = t3 * t4;
    const Float t15 = t0 * t7;
    const Float t16 = t14 - t15;
    const Float t17 = t0 * t5;
    const Float t18 = t1 * t4;
    const Float t19 = t17 - t18;
    const Float t20 = in[8];
    const Float t21 = in[9];
    const Float t22 = in[10];
    const Float t23 = in[11];
    const Float t24 = in[12];
    const Float t25 = in[13];
    const Float t26 = in[14];
    const Float t27 = in[15];
    const Float t28 = t25 * t23;
    const Float t29 = t26 * t20;
    const Float t30 = t27 * t21;
    const Float t31 = t24 * t22;
    const Float t32 = t28 - t30;
    const Float t33 = t29 - t31;
    const Float t34 = t30 - t28;
    const Float t35 = t31 - t29;
    const Float t36 = in[4];
    const Float t37 = in[5];
    const Float t38 = t34 * t36;
    const Float t39 = t36 * t10;
    const Float t40 = t36 * t13;
    const Float t41 = t10 * t37;
    const Float t42 = t37 * t16;
    const Float t43 = t37 * t35;
    const Float t44 = t38 + t42;
    const Float t45 = t43 + t40;
    const Float t46 = in[6];
    const Float t47 = t16 * t46;
    const Float t48 = t32 * t46;
    const Float t49 = t46 * t19;
    const Float t50 = t48 + t41;
    const Float t51 = t39 + t47;
    const Float t52 = t45 + t49;
    const Float t53 = in[7];
    const Float t54 = t13 * t53;
    const Float t55 = t33 * t53;
    const Float t56 = t53 * t19;
    const Float t57 = t50 + t54;
    const Float t58 = t51 + t55;
    const Float t59 = t56 + t44;
    const Float t60 = in[0];
    const Float t61 = in[1];
    const Float t62 = in[2];
    const Float t63 = in[3];
    const Float t64 = t57 * t60;
    const Float t65 = t58 * t61;
    const Float t66 = t59 * t62;
    const Float t67 = t52 * t63;
    const Float t68 = t64 - t65;
    const Float t69 = t66 - t67;
    const Float t70 = t69 + t68;
    fr_out[0] = t70;
    return fr_out[0];
}

Float(njDetMatrix)(NJS_MATRIX *m)
{
    if (m == NULL) {
        Float xf[16];

        sh4_xmtrx_get(xf);
        return det_current(xf);
    }
    return det_memory((const Float *)m);
}
