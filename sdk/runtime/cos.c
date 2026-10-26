#include <errno.h>
#include <math.h>

extern float _in_coss(float x);

double cos(double x)
{
    double result;

    result = _in_coss(x);
    /* under -fpu=single a double is a float, one unsigned long wide. a NaN
     * result sets errno to EDOM, and an infinite one ERANGE */
    /* @hack type: result's bits read through its address, which SHC forms from
     * R15 for each read (MOV R15,R3 at 0x8c126cbe, MOV R15,R2 at 0x8c126ccc);
     * as a union local, 27 of 29 words differ and the function is 4 bytes
     * shorter */
    if ((*(unsigned long *)&result & 0x7f800000) == 0x7f800000) {
        if (*(unsigned long *)&result & 0x007fffff) {
            errno = EDOM;
        } else {
            errno = ERANGE;
        }
    }
    return result;
}
