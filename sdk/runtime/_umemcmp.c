/* memcmp with an int count, for the number conversions */

int _umemcmp(const unsigned char *s1, const unsigned char *s2, int n)
{
    const unsigned char *p1 = s1;
    const unsigned char *p2 = s2;
    unsigned int i;

    if (n == 0) {
        return 0;
    }
    for (i = 0; i < (unsigned int)n; ++i) {
        if (*p1++ != *p2++) {
            break;
        }
    }
    return *(p1 - 1) - *(p2 - 1);
}
