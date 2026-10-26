/* the surfaces around a point, and shadows on the floor below a task */
#ifndef COLLISION_GROUND_H
#define COLLISION_GROUND_H
#include <ninja.h>
#include <sg_xpt.h>

struct angles;
struct sides_query;
struct task_work;

/* a level past any surface. a search starts from it, and a side with no
 * surface keeps it. a side below or toward minus keeps -GROUND_FAR, and a
 * side above or toward plus GROUND_FAR */
#define GROUND_FAR 1000000.0F

/* a character's shadow, and the nearest surfaces above and below it, as
 * shadow_update finds them each step */
struct shadow {
    /* the slope of the surface below, kept from an earlier step when no
     * surface is below */
    Angle ang_x;
    Angle ang_z;
    Uint32 above_flags; /* the world flags of those surfaces, or 0 for none */
    Uint32 below_flags;
    Float above; /* their heights, or GROUND_FAR and -GROUND_FAR */
    Float below;
    Uint32 last_above_flags; /* the flags as they were the step before */
    Uint32 last_below_flags;
    Float scale; /* the shadow's size on a floor right under it */
};

/* ground_below to ground_find_first_floor search the world's objects near
 * the point. each writes the found floor's slope to slope's x and z, and
 * garbage when it finds no floor. ground_below and ground_nearest take a
 * task's object only when the point is within its reach across x and z.
 * they move its surfaces by the y speed of its task's force */

/* the height of the highest floor at or below the point, of the objects
 * that aren't liquid, or -GROUND_FAR */
Float ground_below(Float x, Float y, Float z, struct angles *slope);
/* ground_below, under a second name declared to return int. a caller reads
 * that int from R0, but the height comes back in FR0, so the caller gets
 * whatever R0 holds at the return */
int ground_below_int(Float x, Float y, Float z, struct angles *slope);
/* the height of the floor over or under the point that is nearest the
 * point's height, of any object, or -GROUND_FAR */
Float ground_nearest(Float x, Float y, Float z, struct angles *slope);
/* ground_nearest, of the land's objects that aren't liquid */
Float ground_nearest_land(Float x, Float y, Float z, struct angles *slope);
/* the height of the first floor over or under the task that an object near
 * it gives, or 4000 when none does. here a land object counts only within
 * its reach across x and z. a floor gives 3000 instead when an unset stack
 * word is no more than 1.5 above it. with no object near, the height and
 * slope are garbage */
Float ground_find_first_floor(struct task_work *work, struct angles *slope);

/* the shadow functions draw only while frame_step is 0, a frame's first
 * step. those that return a floor's height return it on every step */

/* draw a shadow on a floor at height under the task, of size 1 right under
 * it, tilted by the task's x and z angles */
void shadow_draw_at_height(struct task_work *work, Float height);
/* draw the task's shadow on the floor below it, which ground_below finds
 * from 4 above the task, and return the floor's height. scale is the
 * shadow's size on a floor right under the task. the shadow tilts by the
 * floor's slope, which is garbage when there is no floor, and turns by
 * minus the task's y angle */
Float shadow_draw(struct task_work *work, Float scale);
/* how far shadow_draw, shadow_draw_slope and shadow_update draw a shadow
 * above the floor they find. shadow_draw_at adds no lift to pos */
#define SHADOW_LIFT 0.3F
/* draw a shadow at pos, tilted by ang's x and z, sized by the square of
 * scale */
void shadow_draw_at(struct angles *ang, NJS_POINT3 *pos, Float scale);
/* shadow_draw, but finding the floor from the task's own height, turning
 * the shadow by the task's y angle, and writing the floor's slope to
 * slope */
Float shadow_draw_slope(struct task_work *work, Float scale,
                        struct angles *slope);
/* run each step for a character. when the last step found a liquid's
 * surface above it and the step before didn't, it splashes. while the last
 * step found one, it ripples that surface. it then finds the nearest
 * surfaces above and below the character, of the world's objects within
 * 200 of it. a task's object that isn't a liquid counts only when the
 * character is within its reach across x and z. on a frame's first step,
 * it draws the shadow on the surface below, when that is less than 20000
 * down */
void shadow_update(struct task_work *work, struct shadow *shadow);
/* fill in result's nearest surface on each side of its point, of the
 * objects near it. ground_find_sides leaves liquids out, and returns TRUE
 * when any object has a surface on a side. ground_find_sides_water takes
 * liquids too. it returns TRUE when, at any point of the search, a
 * liquid's surface was the nearest found below */
Bool ground_find_sides(struct sides_query *result);
Bool ground_find_sides_water(struct sides_query *result);

#endif
