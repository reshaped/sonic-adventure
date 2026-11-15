/* the SH-4 harness's model of the four instructions qemu-sh4 can't run, as
 * xbox/test/emulate.py rewrites them: each by the model the x86 run takes
 * too (libm.c), so that the two runs agree where the port does */
struct sh4_sincos {
    float sin, cos;
};
struct sh4_sincos test_sincos16(unsigned short angle);
float test_fsrra(float x);

struct harness_cpu {
    unsigned r[8], pr, t, fpul, macl, mach;
    float fr[16], xf[16];
} harness_cpu;

void harness_emulate_c(int code, struct harness_cpu *cpu)
{
    int m = code >> 4 & 15;
    int n = code & 15;
    float *fr = cpu->fr;
    int i;

    switch (code >> 8) {
    case 1: { /* FIPR FVm,FVn: FRn+3 is FVm . FVn */
        float sum = fr[m] * fr[n];

        for (i = 1; i < 4; i++) {
            sum += fr[m + i] * fr[n + i];
        }
        fr[n + 3] = sum;
        break;
    }
    case 2: { /* FTRV XMTRX,FVn: XMTRX, XF0 to XF15 by columns, times FVn */
        float v[4];

        for (i = 0; i < 4; i++) {
            v[i] = fr[n + i];
        }
        for (i = 0; i < 4; i++) {
            float sum = cpu->xf[i] * v[0];
            int j;

            for (j = 1; j < 4; j++) {
                sum += cpu->xf[i + 4 * j] * v[j];
            }
            fr[n + i] = sum;
        }
        break;
    }
    case 3: { /* FSCA FPUL,DRn */
        struct sh4_sincos pair = test_sincos16((unsigned short)cpu->fpul);

        fr[n] = pair.sin;
        fr[n + 1] = pair.cos;
        break;
    }
    case 4: /* FSRRA FRn */
        fr[n] = test_fsrra(fr[n]);
        break;
    }
}
