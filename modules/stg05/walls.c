/* tests of the surfaces near a point, for the E-100 series robots and their
 * missiles */
#include <ninja.h>
#include <sg_xpt.h>

#include "collision/ground.h"
#include "collision/polygon.h"
#include "collision/world.h"
#include "stg05/walls.h"

Bool e100_near_surface(NJS_POINT3 *pos, Float margin)
{
    struct sides_query query;
    struct world_object *objects;
    Uint16 i;
    Float distance;

    query.pos = *pos;
    objects = world_objects;
    world_list_near(pos->x, pos->y, pos->z, 1.0F);
    for (i = 0; i < world_count; i++) {
        if (polygon_find_sides(&query, objects[i].object, objects[i].flags)) {
            if (query.sides[SIDE_MINUS_X].found) {
                distance = njAbs(query.sides[SIDE_MINUS_X].level - pos->x);
                if (distance < margin) {
                    return TRUE;
                }
            }
            if (query.sides[SIDE_PLUS_X].found) {
                distance = njAbs(query.sides[SIDE_PLUS_X].level - pos->x);
                if (distance < margin) {
                    return TRUE;
                }
            }
            if (query.sides[SIDE_BELOW].found) {
                distance = njAbs(query.sides[SIDE_BELOW].level - pos->y);
                if (distance < margin) {
                    return TRUE;
                }
            }
            if (query.sides[SIDE_ABOVE].found) {
                distance = njAbs(query.sides[SIDE_ABOVE].level - pos->y);
                if (distance < margin) {
                    return TRUE;
                }
            }
            if (query.sides[SIDE_MINUS_Z].found) {
                distance = njAbs(query.sides[SIDE_MINUS_Z].level - pos->z);
                if (distance < margin) {
                    return TRUE;
                }
            }
            if (query.sides[SIDE_PLUS_Z].found) {
                distance = njAbs(query.sides[SIDE_PLUS_Z].level - pos->z);
                if (distance < margin) {
                    return TRUE;
                }
            }
        }
    }
    return FALSE;
}

Bool e100_push_from_walls(NJS_POINT3 *pos, Float margin, Float *floor,
                          NJS_VECTOR *push)
{
    struct sides_query query;
    struct world_object *objects;
    Uint16 i;
    Float distance;
    Float highest;
    Bool touched = FALSE;

    query.pos = *pos;
    objects = world_objects;
    highest = -GROUND_FAR;
    world_list_near(pos->x, pos->y, pos->z, 2.0F);
    push->x = push->y = push->z = 0.0F;
    for (i = 0; i < world_count; i++) {
        if (polygon_find_sides(&query, objects[i].object, objects[i].flags)) {
            if (query.sides[SIDE_MINUS_X].found) {
                distance = njAbs(query.sides[SIDE_MINUS_X].level - pos->x);
                if (distance < margin) {
                    push->x = margin - distance;
                    touched = TRUE;
                    break;
                }
            }
            if (query.sides[SIDE_PLUS_X].found) {
                distance = njAbs(query.sides[SIDE_PLUS_X].level - pos->x);
                if (distance < margin) {
                    push->x = distance - margin;
                    touched = TRUE;
                    break;
                }
            }
            if (query.sides[SIDE_MINUS_Z].found) {
                distance = njAbs(query.sides[SIDE_MINUS_Z].level - pos->z);
                if (distance < margin) {
                    push->z = margin - distance;
                    touched = TRUE;
                    break;
                }
            }
            if (query.sides[SIDE_PLUS_Z].found) {
                distance = njAbs(query.sides[SIDE_PLUS_Z].level - pos->z);
                if (distance < margin) {
                    push->z = distance - margin;
                    touched = TRUE;
                    break;
                }
            }
            if (query.sides[SIDE_BELOW].found) {
                if (highest < query.sides[SIDE_BELOW].level) {
                    highest = query.sides[SIDE_BELOW].level;
                }
            }
        }
    }
    /* a wall that touches pos ends the scan for walls at object i, before
     * i's floor is taken. so this loop takes the floor of i and of each object
     * after it */
    if (floor != NULL) {
        for (; i < world_count; i++) {
            if (polygon_find_sides(&query, objects[i].object,
                                   objects[i].flags)) {
                struct surface *below = &query.sides[SIDE_BELOW];

                if (below->found && highest < below->level) {
                    highest = below->level;
                }
            }
        }
        *floor = highest;
    }
    return touched;
}
