/* drawing helpers: a model tree with its own draws for some nodes, the
 * hud's big digits and a line, and measures of points, spheres and angles */
#ifndef DRAW_HELPERS_H
#define DRAW_HELPERS_H
#include <ninja.h>
#include <sg_xpt.h>

#include "system/task.h"

/* where hud_draw_time puts its digits */
struct hud_layout {
    NJS_POINT2 pos; /* of the first digit */
    NJS_POINT2 scale;
    NJS_POINT2 spacing; /* from each digit to the next */
};

/* draw_object draws this object with draw instead. a table of them ends
 * at a null object */
struct draw_override {
    NJS_OBJECT *object;
    void (*draw)(NJS_OBJECT *object);
};

/* draw object, its children and its siblings in the current matrix. each
 * node translates the matrix by its pos, turns it by its ang and scales it
 * by its scl. it turns by njRotateZ, njRotateX and njRotateY in that order,
 * whatever its evalflags. its model, if any, is then drawn with draw_model.
 * a node with a model that overrides names is neither moved nor drawn here:
 * the override's draw takes it, and a null draw skips it. that node's
 * children draw in the matrix the override's draw leaves */
void draw_object(NJS_OBJECT *object, const struct draw_override *overrides);
/* set out to the point distance along the way from start toward target, or
 * to start itself where target is the same point */
void point_toward(NJS_POINT3 *start, NJS_POINT3 *target, Float distance,
                  NJS_POINT3 *out);
/* TRUE when spheres of these radii at these centers overlap, else FALSE */
Bool spheres_overlap(NJS_POINT3 *center1, NJS_POINT3 *center2, Float radius1,
                     Float radius2);
/* how far target lies ahead of angle, counting up: 0 to just under a full
 * turn, 0x10000 */
Angle angle_forward_distance(Angle angle, Angle target);
/* how far target lies from angle the short way round: 0 to a half turn */
Angle angle_shortest_distance(Angle angle, Angle target);
/* angle turned toward target by delta the short way, and no further than
 * target. from exactly a half turn away, it adds delta. the result is from 0
 * to 0xffff */
Angle angle_turn_toward(Angle angle, Angle target, Uint32 delta);
/* draw minutes and seconds in the hud's big digits, two each, with a colon
 * between. each draws its last two digits, as 00 below 0 and as 99 past
 * 999 */
void hud_draw_time(struct hud_layout *layout, int minutes, int seconds);
/* draw a line length long from pos, along the z axis turned by ang in
 * njRotateXYZ's order */
void draw_line_facing(struct angles *ang, NJS_POINT3 *pos, Float length);

#endif
