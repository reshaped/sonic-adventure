#include <errno.h>
#include <mathf.h>

extern float _in_modfs(float value, float *iptr);

float modff(float value, float *iptr)
{
    float fraction;
    float whole;

    fraction = _in_modfs(value, &whole);
    *iptr = whole;
    /* a NaN fraction sets errno to EDOM, and an infinite one ERANGE */
    /* @hack type: fraction's bits read through its address, which SHC forms
     * from R15 for each read (MOV R15,R3 at 0x8c123cc6, MOV R15,R2 at
     * 0x8c123cd0); as a union local, 35 of 35 words differ and the function is
     * 4 bytes shorter */
    if ((*(unsigned long *)&fraction & 0x7f800000) == 0x7f800000) {
        if (*(unsigned long *)&fraction & 0x007fffff) {
            errno = EDOM;
        } else {
            errno = ERANGE;
        }
    }
    return fraction;
}
