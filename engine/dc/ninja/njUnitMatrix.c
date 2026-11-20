/* njUnitMatrix on SH4ZAM. make dc compiles this in place of
 * sdk/ninja/njUnitMatrix.c */
#include <ninja.h>

/* set m to the unit matrix, _unit_matrix_. a null m selects the current
 * matrix, in XMTRX. SH4ZAM's identity sets XMTRX in the registers, with no
 * load of _unit_matrix_ */
void njUnitMatrix(NJS_MATRIX *m)
{
    if (m == NULL) {
        shz_xmtrx_init_identity();
    } else {
        zam_copy_matrix(*m, _unit_matrix_);
    }
}
