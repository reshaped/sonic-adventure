int _allzero(const unsigned char *bytes, int n)
{
    int i;

    for (i = 0; i < n; i++, bytes++) {
        if (*bytes != 0) {
            return 0;
        }
    }
    return 1;
}
