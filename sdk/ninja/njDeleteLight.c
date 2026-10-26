#include <ninja.h>
#include <sg_xpt.h>

void njDeleteLight(NJS_LIGHT *l)
{
    Int i;
    Int found = -1;
    Int count;

    if (_nj_light_control_.count > 0) {
        count = _nj_light_control_.count;
        for (i = 0; i < count; i++) {
            if (l == _nj_light_control_.light[i]) {
                found = i;
                i = count;
            }
        }
        if (found >= 0) {
            found++;
            for (i = found; i < count; i++) {
                _nj_light_control_.light[i - 1] = _nj_light_control_.light[i];
            }
            _nj_light_control_.count--;
        }
    }
}
