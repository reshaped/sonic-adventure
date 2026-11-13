/* njMultiMatrix on SH4ZAM. make dc compiles this in place of
 * sdk/ninja/njMultiMatrix.c */
#include <ninja.h>

/* md = ms * md in memory, by FTRV in XMTRX. the current matrix waits on the
 * stack meanwhile. the shipped body runs sixteen FIPRs where this runs FTRV,
 * and FTRV's last bits needn't match theirs. it is out of line so that the
 * null md's path makes no frame for the current matrix */
static void __attribute__((noinline)) multiply_into(NJS_MATRIX *md,
                                                    NJS_MATRIX *ms)
{
    shz_mat4x4_t current;

    shz_xmtrx_store_4x4(&current);
    shz_xmtrx_load_unaligned_4x4(*md);
    shz_xmtrx_apply_unaligned_4x4(*ms);
    shz_xmtrx_store_unaligned_4x4(*md);
    shz_xmtrx_load_4x4(&current);
}

/* md = ms * md, for row-major matrices. a null md selects the current matrix,
 * in XMTRX. ms is read whole before md is written, so the two may overlap */
void njMultiMatrix(NJS_MATRIX *md, NJS_MATRIX *ms)
{
    if (md != NULL) {
        multiply_into(md, ms);
        return;
    }
    shz_xmtrx_apply_unaligned_4x4(*ms);
}

/* the shipped body only returns, so the image can't show what parameters the
 * SDK gave this function. this definition takes none */
void njMultiMatrix_old(void)
{
}
