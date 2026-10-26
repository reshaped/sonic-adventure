/* Froggy in the events. he draws the model each event defines, and blends
 * three of its nodes through shape scripts */
#ifndef EVENT_FROG_H
#define EVENT_FROG_H
#include <ninja.h>
#include <sg_xpt.h>

#include "system/task.h"

/* the script event_frog_set_shape takes to put his nodes back as made */
#define FROG_SCRIPT_RESTORE 7

/* his 8 textures, which each event that links frog.c defines and loads from
 * SHAPE_FROG */
extern NJS_TEXLIST event_frog_texlist;

/* create Froggy, or return null when task_create fails. his first step
 * gives him his records: an event record, a collision sphere and the shape
 * record. wait a step before the calls that need them, event_frog_set_shape
 * among them */
struct task *event_frog_create(void);
/* kill the last Froggy created, which puts his nodes back as made. a second
 * call does nothing */
void event_frog_kill(void);
/* start shape script 0 to 6 on the last Froggy created, or put his nodes
 * back with FROG_SCRIPT_RESTORE. scripts 2 and 4 each run paired with the
 * script after it. nothing happens before the first create or after
 * event_frog_kill */
void event_frog_set_shape(Sint8 script);

#endif
