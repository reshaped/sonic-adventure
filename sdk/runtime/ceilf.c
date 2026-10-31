#include <errno.h>
#include <mathf.h>

extern float _in_ceils(float x);

#ifdef GCC
/* each C call inlines mathf.h's zam_ceilf, so nothing calls this
 * definition */
#undef ceilf
#endif
float ceilf(float x)
{
    float result;

    result = _in_ceils(x);
    /* a NaN result sets errno to EDOM, and an infinite one ERANGE */
    /* @hack type: result's bits read through its address, which SHC forms from
     * R15 for each read (MOV R15,R3 at 0x8c11c192, MOV R15,R2 at 0x8c11c1a0);
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
