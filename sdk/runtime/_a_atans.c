/* arc tangent for small arguments, as a series in x through x^11 */

/* the series' coefficients, by powers of x squared from the highest */
static const float coefficients[6] = {
    -0.09090787917375565F, 0.111111108F,         -0.1428571492433548F,
    0.20000000298023224F,  -0.3333333432674408F, 1.0F,
};

#pragma inline(series)

static float series(float square)
{
    const float *coefficient = coefficients;
    float sum = *coefficient++;
    int i = 5;

    do {
        sum = (sum * square) + *coefficient++;
    } while (--i > 0);
    return sum;
}

float _app_atans(float x)
{
    return x * series(x * x);
}
