/* store to minus from at to. each is n bytes, most significant first. in mode
 * 0, a borrow out of the top returns -1. otherwise it returns 0 for a zero
 * difference and 1 for any other */
int _sub(int mode, unsigned char *to, const unsigned char *from, int n)
{
    int difference;
    int borrow = 0;
    int zero = 1;

    for (--n; n >= 0; --n) {
        difference = (int)to[n] - (int)from[n] - borrow;
        /* @hack spelling: the borrow and the wrap in one select, which keeps
         * difference in a frame word (MOV.L R2,@R15 at 0x8c127a80) and sets
         * the borrow in R14 on each arm (0x8c127a8a, 0x8c127a8c); as an if
         * and an else, SHC saves R8, R10 and R11 for R12, the function is 2
         * bytes longer, and 55 of 60 words differ */
        borrow = difference < 0 ? (difference = difference + 256, 1) : 0;
        if (difference != 0) {
            zero = 0;
        }
        to[n] = (unsigned char)difference;
    }
    if (borrow == 1 && mode == 0) {
        return -1;
    }
    if (zero == 1) {
        return 0;
    }
    return 1;
}
