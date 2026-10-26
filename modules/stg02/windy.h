/* Windy Valley stage task, and the sky task that sets each act's fog and draws
 * its sky */
#ifndef STG02_WINDY_H
#define STG02_WINDY_H

#include <ninja.h>
#include <sg_xpt.h>

#include "system/task.h"

/* 1ST_READ.BIN's windy_set_fog sets act 0's fog from windy_fog_table and
 * windy_fog_shown at the vsync that ends each frame. during a demo, it sets the
 * fog only when windy_fog_shown is 0xffffffff */
extern NJS_FOG_TABLE windy_fog_table;
/* the fog's color by channel. only windy.c reads it. SHC orders bss by each
 * name's first declaration, and this one puts windy_fog_color ahead of
 * windy_fog_shown */
extern NJS_COLOR windy_fog_color;
extern Uint32 windy_fog_shown; /* windy_fog_color as one word */

/* the step of the stage's task, which 1ST_READ.BIN creates for Windy Valley.
 * task needs TASK_WORK's record. the step requests each act's music and plays
 * its wind. it changes act once act 0 or act 1 ends. a failed task_create of
 * the music's task goes unchecked */
void windy_step(struct task *task);
/* the step of the stage's sky task, which needs TASK_WORK's record. the sky
 * sets each act's fog and draws its sky. in act 0 it gathers the storm. in act
 * 2 it picks the land blocks and keeps gravity straight down. the task kills
 * itself outside Windy Valley */
void windy_sky_step(struct task *task);

#endif
