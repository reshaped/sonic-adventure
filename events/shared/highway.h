/* the stretch of highway where the last story's last events play, which a
 * task of its own draws */
#ifndef EVENT_HIGHWAY_H
#define EVENT_HIGHWAY_H
#include <ninja.h>

/* the highway's 12 textures, which EV00FD and EV00FF load from
 * LAST1A_HIGHWAY_A */
extern NJS_TEXLIST event_highway_texlist;

/* create the task that draws the highway, unless there is one. nothing
 * happens when task_create fails */
void event_highway_create(void);
/* kill the highway's task, if there is one */
void event_highway_kill(void);

#endif
