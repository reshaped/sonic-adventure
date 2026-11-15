/* the x86 run's load of XMTRX, the port's (xbox/include/sh4.h; libm.c
 * defines it for the tests), from m in order, as the SH-4 run loads XF0 to
 * XF15 */
extern float sh4_xmtrx[16];

void harness_load_xmtrx(const float *m)
{
    int k;

    for (k = 0; k < 16; k++) {
        sh4_xmtrx[k] = m[k];
    }
}

void harness_store_xmtrx(float *m)
{
    int k;

    for (k = 0; k < 16; k++) {
        m[k] = sh4_xmtrx[k];
    }
}
