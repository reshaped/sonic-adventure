#include <errno.h>
#include <mathf.h>

extern float _in_pows(float x, float y);

float powf(float x, float y)
{
    float result;

    result = _in_pows(x, y);
    /* a NaN result sets errno to EDOM, and an infinite one ERANGE */
    /* @hack type: result's bits read through its address, which SHC forms from
     * R15 for each read (MOV R15,R3 at 0x8c11a656, MOV R15,R2 at 0x8c11a664);
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
