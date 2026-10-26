/* the water splashes that O ICEBAR1 and O ICEBAR2 raise as they rise */
#ifndef STG08_SPLASH_H
#define STG08_SPLASH_H
#include <ninja.h>

/* create a splash at pos, size times its sprite's size. it faces the
 * camera, turned ang about the line of sight. it plays its images once, then
 * fades and goes. when task_create fails, there is no splash */
void icecap_splash_create(NJS_POINT3 *pos, Angle ang, Float size);

#endif
