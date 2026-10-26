/* shift n bytes, most significant first, right one bit, and return the bit
 * shifted out */
int _rsft(unsigned char *bytes, int n)
{
    int i;
    int carry;
    int low;

    carry = 0;
    for (i = 0; i < n; i++) {
        low = bytes[i] & 1;
        bytes[i] >>= 1;
        if (carry) {
            bytes[i] |= 0x80;
        }
        carry = low;
    }
    return carry;
}
