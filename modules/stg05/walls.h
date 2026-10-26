/* tests of the surfaces near a point, for the E-100 series robots and their
 * missiles */
#ifndef STG05_WALLS_H
#define STG05_WALLS_H
#include <ninja.h>
#include <sg_xpt.h>

/* each test lists in world_objects the objects near pos, as world_list_near
 * does. it lists only objects the world's last collect kept. an object is
 * near when its sphere comes within the test's reach of pos. a task's object
 * is near within 30 more. polygon_find_sides then gives each listed object's
 * nearest surface on each side of pos. a surface touches pos when its level
 * is less than margin from pos along its side's axis */

/* TRUE when a surface of a listed object touches pos. the reach is 1 */
Bool e100_near_surface(NJS_POINT3 *pos, Float margin);
/* TRUE when a wall of a listed object touches pos. the reach is 2. a wall is a
 * surface on a side of pos along x or z. pos + push lies margin from the first
 * wall that touches pos, along the wall's axis. push is 0 on the other axes,
 * and on all three for FALSE. unless floor is NULL, *floor gets the level of
 * the highest floor below pos, or -GROUND_FAR for none */
Bool e100_push_from_walls(NJS_POINT3 *pos, Float margin, Float *floor,
                          NJS_VECTOR *push);

#endif
