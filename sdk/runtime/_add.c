#include <multiprec.h>

void _add(unsigned char *to, unsigned char *from, int n)
{
    int sum;
    int carry = 0;

    --n;
    from += n;
    to += n;
    for (; n >= 0; --n, --from, --to) {
        /* @hack spelling: the store and the sum in one statement, which keeps
         * the sum in R14 and carry in R7 (MOV #0,R7 at 0x8c1281c4); as two
         * statements, the two trade and 8 of 31 words differ in each image */
        *to = sum = *to + *from + carry;
        carry = sum / 256;
    }
}
