/* Chaos 1 as the events pose him: he stands, melts into Chaos 0's puddle and
 * rises from it. the calls act on the last Chaos 1 created. all but create,
 * kill and the texture calls need one */
#ifndef EVENT_CHAOS1_H
#define EVENT_CHAOS1_H
#include <ninja.h>
#include <sg_xpt.h>

/* load his textures and the puddle's. event_chaos1_release_textures releases
 * both */
void event_chaos1_load_textures(void);
void event_chaos1_release_textures(void);
/* create Chaos 1 standing at x, y and z, turned by the angles. a melt or a rise
 * then lasts steps + 1 steps. each step sinks or lifts his body 30 / steps. a
 * rise's last step puts him back at create's height, then lifts him once more.
 * he ends 30 / steps higher. a failed task_create or heap_alloc is unchecked */
void event_chaos1_create(Float x, Float y, Float z, Angle ang_x, Angle ang_y,
                         Angle ang_z, int steps);
/* kill him on his next step. before the first create it does nothing. his task
 * pointer stays set, so only create and the texture calls may follow */
void event_chaos1_kill(void);
/* melt into the puddle, or rise from it, over the steps create took */
void event_chaos1_melt(void);
void event_chaos1_rise(void);
/* set how many frames of his action play each step. create sets 0.1 */
void event_chaos1_set_speed(Float speed);
void event_chaos1_set_pos(Float x, Float y, Float z);
void event_chaos1_set_ang(Angle x, Angle y, Angle z);
/* turn him into the puddle at once */
void event_chaos1_puddle(void);

#endif
