/* act 2's traffic: the path task that drives nine cars around traffic_route */
#ifndef STG04_TRAFFIC_H
#define STG04_TRAFFIC_H

#include <ninja.h>
#include <sg_xpt.h>

#include "system/task.h"

/* the point of a route nearest player 0, which the traffic's path task finds
 * each step */
struct route_nearest {
    /* on highway_act2_route, the row of police_stretches whose span holds
     * along, or -1 */
    int stretch;
    Float distance; /* player 0's distance from pos */
    Float along; /* pos's distance along the route */
    NJS_POINT3 pos;
};

/* the cars' textures, which act 2 loads */
extern NJS_TEXLIST highway_car_texlist;
/* the point of highway_act2_route nearest player 0 */
extern struct route_nearest traffic_act2_nearest;

/* the step traffic_route names for its path task. it makes the cars, and gives
 * the task the step that runs them */
void traffic_step(struct task *task);

#endif
