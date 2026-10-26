/* an event's clouds: two streams of sprites that drift along z past the
 * camera */
#ifndef EVENT_CLOUDS_H
#define EVENT_CLOUDS_H
#include <ninja.h>
#include <sg_xpt.h>

/* the clouds' textures. each event that links clouds.c defines them, and
 * each that starts the clouds loads them from EV_ECCLOUD */
extern NJS_TEXLIST event_clouds_texlist;

/* start the clouds, count in each of the two streams. each cloud takes a
 * random scale from four fifths of scale to scale. a start while they're
 * out does nothing. a failed task_create or heap_alloc is unchecked */
void event_clouds_start(Float scale, int count);
/* kill the clouds on their next step, and free their records now, if they're
 * out. a draw of the clouds before that step reads the freed records */
void event_clouds_stop(void);
/* set how far the clouds drift along z each step: 5 until it's set */
void event_clouds_set_speed(Float speed);

#endif
