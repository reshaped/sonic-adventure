/* the homing missiles E-101 mkII fires */
#ifndef B_E101_R_MISSILE_H
#define B_E101_R_MISSILE_H
#include <ninja.h>
#include <sg_xpt.h>

/* a count of the missiles in flight, which the boss reads to widen the
 * camera. each missile adds 1 as it fires. as it is destroyed, it takes 1
 * off a count above 0. while the count is negative, each missile bursts on
 * its next step */
extern Sint32 e101_mk2_missiles_flying;

/* set the count to 0 */
void e101_mk2_missiles_reset(void);
/* set the count to -1, so every missile in flight bursts */
void e101_mk2_missiles_explode(void);
/* fire a missile from pos at speed, facing the way the boss faces. ang is
 * the boss's y angle, which the boss faces back along. the missile's y angle
 * runs the other way round, so it starts at 0x8000 - ang. pos and speed are
 * copied.
 *
 * when no task can be made, no missile fires. when the heap can't hold the
 * missile's record, e101_mk2_missile_create writes through null. without its
 * shapes' copy, the missile reads its collision record through null each
 * step. without its lock-on record, enemy_alloc_record kills its task, which
 * ends on its next step */
void e101_mk2_missile_create(NJS_POINT3 *pos, NJS_VECTOR *speed, Angle ang);

#endif
