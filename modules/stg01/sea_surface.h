/* sea surface of acts 1 and 2, which follows player 0 */
#ifndef STG01_SEA_SURFACE_H
#define STG01_SEA_SURFACE_H
#include <ninja.h>

/* the surface and its textures, which act 0's sea draws too */
extern NJS_OBJECT beach_sea_surface_object;
extern NJS_TEXLIST beach_sea_surface_texlist;
/* the plane that makes the sea a place to swim, in acts 0 and 1 */
extern NJS_OBJECT beach_sea_surface_plane_object;

/* create the surface for mode, BEACH_ACT1 or BEACH_ACT2, and keep mode in
 * the surface's work. any other mode creates nothing. the task pool and the
 * heap must have room, since task_create's result goes unchecked.
 * beach_act_start calls this */
void beach_sea_surface_create(int mode);
/* kill the surface, which beach_sea_surface_create must have made: the task
 * pointer is not checked. the sky's destructor calls this for acts 1 and
 * 2 */
void beach_sea_surface_remove(void);

#endif
