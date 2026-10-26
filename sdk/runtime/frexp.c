#include <errno.h>
#include <math.h>

/* a double is a float under -fpu=single */
union number {
    double value;
    unsigned long bits;
};

/* an infinity sets errno to ERANGE, and a NaN to EDOM; either is returned
 * with *exp unset */
double frexp(double value, int *exp)
{
    union number number;
    long exponent;

    number.value = value;
    exponent = number.bits & 0x7f800000;
    if (value == 0.0) {
        *exp = 0;
        return value;
    }
    if (exponent == 0x7f800000) {
        if ((number.bits & 0x007fffff) == 0) {
            errno = ERANGE;
        } else {
            errno = EDOM;
        }
        return value;
    }
    *exp = (exponent >> 23) - 126;
    number.bits =
        (number.bits & 0x80000000) | (number.bits & 0x007fffff) | 0x3f000000;
    return number.value;
}
