/* sine for small arguments, as a series in x through x^13 */

/* the series' coefficients, by powers of x squared from the highest */
static const float coefficients[7] = {
    1.6058363e-10F, -2.50521044e-08F, 2.7557319e-06F, -1.98412702e-04F,
    8.333334e-03F,  -0.166666672F,    1.0F,
};

#pragma inline(series)

static float series(float square)
{
    const float *coefficient = coefficients;
    float sum = *coefficient++;
    int i = 6;

    do {
        sum = (sum * square) + *coefficient++;
    } while (--i > 0);
    return sum;
}

float _app_sins(float x)
{
    return x * series(x * x);
}
