/* arc cosine without errno reporting */

extern float _in_asins(float x);
extern const float _HLFPAIS;

float _in_acoss(float x)
{
    return _HLFPAIS - _in_asins(x);
}
