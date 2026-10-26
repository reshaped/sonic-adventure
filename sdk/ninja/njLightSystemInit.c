#include <ninja.h>
#include <sg_xpt.h>

void njLightSystemInit(void)
{
    Int i;

    _nj_light_control_.count = 0;
    _nj_light_material_.diffuse.a = 1.0F;
    _nj_light_material_.diffuse.r = 0.5F;
    _nj_light_material_.diffuse.g = 0.5F;
    _nj_light_material_.diffuse.b = 0.5F;
    _nj_light_material_.specular.a = 1.0F;
    _nj_light_material_.specular.r = 0.5F;
    _nj_light_material_.specular.g = 0.5F;
    _nj_light_material_.specular.b = 0.5F;
    _nj_light_material_.ambient.a = 1.0F;
    _nj_light_material_.ambient.r = 0.0F;
    _nj_light_material_.ambient.g = 0.0F;
    _nj_light_material_.ambient.b = 0.0F;
    _nj_light_material_.exponent = 6.0F;
    _nj_light_queue_.count = 0;
    for (i = 0; i < NJD_LIGHT_MAX; i++) {
        _nj_light_queue_.light[i] = 0;
    }
}
