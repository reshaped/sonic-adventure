#include <multiprec.h>

int _lsft(unsigned char *bytes, int n)
{
    int carry = 0;
    int high;
    int i;

    for (i = n - 1; i >= 0; --i) {
        high = bytes[i] & 0x80;
        bytes[i] <<= 1;
        if (carry) {
            bytes[i] |= 1;
        }
        carry = high;
    }
    if (carry) {
        return 1;
    }
    return 0;
}
