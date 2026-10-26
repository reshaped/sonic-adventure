/* two-argument arc tangent without errno reporting */

extern float _in_atans(float x);
extern const float _PAIS;
extern const float _NPAIS;

float _in_atan2s(float y, float x)
{
    float angle;

    angle = _in_atans(y / x);
    /* the sign bits select the half turn, so -0 counts as negative */
    /* @hack type: x's and y's sign bits read through their addresses, which SHC
     * forms from R15 for each read (MOV R15,R2 at 0x8c11bf2c, MOV R15,R1 at
     * 0x8c11bf38); as union copies, 34 of 35 words differ and the function is 8
     * bytes longer */
    if (*(unsigned long *)&x & 0x80000000) {
        if (*(unsigned long *)&y & 0x80000000) {
            angle = angle + _NPAIS;
        } else {
            angle = angle + _PAIS;
        }
    }
    return angle;
}
