/* cosine for small arguments, as a series in x through x^12 */

/* the series' coefficients, by powers of x squared from the highest */
static const float coefficients[7] = {
    2.0875608e-09F, -2.7557315e-07F, 2.4801588e-05F, -0.00138888893F,
    0.041666668F,   -0.5F,           1.0F,
};

float _app_coss(float x)
{
    float square = x * x;
    const float *coefficient = coefficients;
    int i = 6;

    /* @hack spelling: the sum kept in x, which leaves x's FR4 to the sum and
     * FR5 to square (FMAC FR0,FR4,FR3 at 0x8c12754e); in a local of its own,
     * the two trade and 2 of 14 words differ */
    x = *coefficient++;
    do {
        x = (x * square) + *coefficient++;
    } while (--i > 0);
    return x;
}
