/* store *half in the two bytes at bytes, most significant first. bytes must
 * not overlap *half */
void _mvsto2b(unsigned char *bytes, const unsigned short *half)
{
    int i;

    for (i = 0; i < 2; i++) {
        bytes[i] = *half >> ((1 - i) * 8);
    }
}
