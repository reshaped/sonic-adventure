/* Chaos 0 as the events pose him. he stands, walks and hops, and melts into his
 * puddle and rises from it. the calls act on the last Chaos 0 created. all but
 * create, kill, event_chaos0_get_task and the texture calls need one */
#ifndef EVENT_CHAOS0_H
#define EVENT_CHAOS0_H
#include <ninja.h>
#include <sg_xpt.h>

#include "system/task.h"

/* his textures, which event_chaos0_load_textures loads from CHAOS0 */
extern NJS_TEXLIST event_chaos0_texlist;

/* load his textures and his puddle's. event_chaos0_release_textures releases
 * both */
void event_chaos0_load_textures(void);
void event_chaos0_release_textures(void);
/* create Chaos 0 standing at x, y and z, turned by the angles. a melt or a rise
 * then lasts steps + 1 steps. each step sinks or lifts his body 25 / steps. a
 * rise's last step puts him back at create's height, then lifts him once more.
 * he ends 25 / steps higher. a failed task_create or heap_alloc is unchecked */
void event_chaos0_create(Float x, Float y, Float z, Angle ang_x, Angle ang_y,
                         Angle ang_z, int steps);
/* kill him on his next step. before the first create it does nothing. his task
 * pointer stays set, so only create and the texture calls may follow */
void event_chaos0_kill(void);
/* hop toward x, y and z in an arc as high as height, then stand. he faces the
 * way he hops. the hop lasts steps + 1 steps, each 1 / steps of the way. it
 * ends a step past the point. each of the four hops plays its own action. each
 * step advances it by the first hop action's frame count over steps */
void event_chaos0_hop0(Float x, Float y, Float z, Float height, int steps);
void event_chaos0_hop1(Float x, Float y, Float z, Float height, int steps);
void event_chaos0_hop2(Float x, Float y, Float z, Float height, int steps);
void event_chaos0_hop3(Float x, Float y, Float z, Float height, int steps);
/* walk toward x, y and z, facing that way, then stand. the walk lasts steps + 1
 * steps, each 1 / steps of the way. it ends a step past the point */
void event_chaos0_walk(Float x, Float y, Float z, int steps);
/* slide as his puddle toward x, y and z, then stay a puddle. the slide lasts
 * steps + 1 steps, each 1 / steps of the way. it ends a step past the point */
void event_chaos0_slide(Float x, Float y, Float z, int steps);
/* the last Chaos 0's task, or null before the first create */
struct task *event_chaos0_get_task(void);
/* set how many frames of his actions play each step, but for a hop's.
 * create sets 1 */
void event_chaos0_set_speed(Float speed);
/* melt into his puddle, or rise from it, over the steps create took */
void event_chaos0_melt(void);
void event_chaos0_rise(void);
void event_chaos0_set_pos(Float x, Float y, Float z);
void event_chaos0_set_ang(Angle x, Angle y, Angle z);
/* turn him into his puddle at once */
void event_chaos0_puddle(void);
/* set the scale he draws at as he stands or loops one of the four actions
 * below. the other modes draw him unscaled */
void event_chaos0_set_scale(Float x, Float y, Float z);
/* stand, as create leaves him, or loop one of four more actions */
void event_chaos0_stand(void);
void event_chaos0_play7(void);
void event_chaos0_play8(void);
void event_chaos0_play9(void);
void event_chaos0_play10(void);

#endif
