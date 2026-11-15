/* the 64-bit divides that Clang calls on i386, which a freestanding build
 * finds in no library: unsigned by shift and subtract, signed by their
 * magnitudes, each rounding toward 0 as C does */
typedef unsigned long long u64;
typedef long long s64;

static u64 divide(u64 n, u64 d, u64 *rem)
{
    u64 q = 0;
    u64 r = 0;
    int bit;

    if (d == 0) {
        __builtin_trap();
    }
    if ((d >> 32) == 0 && (n >> 32) == 0) {
        *rem = (unsigned)n % (unsigned)d;
        return (unsigned)n / (unsigned)d;
    }
    for (bit = 63; bit >= 0; bit--) {
        r = (r << 1) | ((n >> bit) & 1);
        if (r >= d) {
            r -= d;
            q |= (u64)1 << bit;
        }
    }
    *rem = r;
    return q;
}

u64 __udivdi3(u64 n, u64 d)
{
    u64 r;

    return divide(n, d, &r);
}

u64 __umoddi3(u64 n, u64 d)
{
    u64 r;

    divide(n, d, &r);
    return r;
}

s64 __divdi3(s64 n, s64 d)
{
    u64 r;
    u64 q = divide(n < 0 ? -(u64)n : (u64)n, d < 0 ? -(u64)d : (u64)d, &r);

    return (n < 0) != (d < 0) ? -(s64)q : (s64)q;
}

s64 __moddi3(s64 n, s64 d)
{
    u64 r;

    divide(n < 0 ? -(u64)n : (u64)n, d < 0 ? -(u64)d : (u64)d, &r);
    return n < 0 ? -(s64)r : (s64)r;
}
