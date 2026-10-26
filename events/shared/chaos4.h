/* Chaos 4 as the events pose him. he stands, melts into Chaos 0's puddle,
 * slides as the puddle and rises from it. the calls act on the last Chaos 4
 * created. all but create, kill and the texture calls need one */
#ifndef EVENT_CHAOS4_H
#define EVENT_CHAOS4_H
#include <ninja.h>
#include <sg_xpt.h>

/* his nine textures, which each event loads from CHAOS4_COMMON */
extern NJS_TEXLIST event_chaos4_texlist;

/* load his textures and the puddle's. event_chaos4_release_textures releases
 * both */
void event_chaos4_load_textures(void);
void event_chaos4_release_textures(void);
/* create Chaos 4 standing at x, y and z, turned by the angles. a melt or a rise
 * then lasts steps + 1 steps. each step sinks or lifts his body 55 / steps. a
 * rise's last step puts him back at create's height, then lifts him once more.
 * he ends 55 / steps higher. a failed task_create or heap_alloc is unchecked */
void event_chaos4_create(Float x, Float y, Float z, Angle ang_x, Angle ang_y,
                         Angle ang_z, int steps);
/* kill him on his next step. before the first create it does nothing. his
 * task pointer stays set, so only create and the texture calls may follow */
void event_chaos4_kill(void);
/* melt into the puddle, or rise from it. either lasts steps + 1 steps, by
 * create's steps */
void event_chaos4_melt(void);
void event_chaos4_rise(void);
/* set how many frames of his action play each step as he stands, melts or
 * rises. create sets 0.1 */
void event_chaos4_set_speed(Float speed);
void event_chaos4_set_pos(Float x, Float y, Float z);
void event_chaos4_set_ang(Angle x, Angle y, Angle z);
/* slide as the puddle toward x, y and z, then stay the puddle. the slide lasts
 * steps + 1 steps, each 1 / steps of the way. it ends a step past the point */
void event_chaos4_slide(Float x, Float y, Float z, int steps);
/* turn him into the puddle at once */
void event_chaos4_puddle(void);

#endif
