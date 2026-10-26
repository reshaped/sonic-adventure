/* an event's props: models in slots, each drawn still or playing one of its
 * actions */
#ifndef EVENT_PROP_H
#define EVENT_PROP_H
#include <ninja.h>
#include <sg_xpt.h>

#include "system/task.h"

/* an event's prop slots, 0 to EVENT_PROPS - 1 */
#define EVENT_PROPS 10
/* a prop's actions, 0 to EVENT_PROP_ACTIONS - 1 */
#define EVENT_PROP_ACTIONS 10

/* make a prop in slot: object, drawn with texlist at x, y and z, turned
 * and scaled. it draws its object until an action plays. its step runs the
 * paths queued on its task, and draws it as well as its draw. a prop already
 * in the slot runs on, and no call by slot reaches it again. task_create's
 * and heap_alloc's failures are unchecked */
void event_prop_create(Float x, Float y, Float z, Angle ang_x, Angle ang_y,
                       Angle ang_z, Float scl_x, Float scl_y, Float scl_z,
                       NJS_OBJECT *object, NJS_TEXLIST *texlist, int slot);

/* each call below that takes a slot but event_prop_get and event_prop_kill
 * needs a prop in it */

/* hold action as the prop's action number */
void event_prop_set_action(int slot, NJS_ACTION *action, int number);
/* the count at which event_prop_play plays an action without end */
#define EVENT_PROP_FOREVER (-1)
/* play the action held as number from its frame 0, advancing speed of its
 * frames each step. number must hold an action from event_prop_set_action,
 * since the step and the draw read it. after count plays, the prop holds the
 * action's frame 0. a count of 0 or below, as EVENT_PROP_FOREVER, plays it
 * without end */
void event_prop_play(int slot, int number, Float speed, int count);
/* stop the action, and draw the prop's object still */
void event_prop_stop(int slot);
/* the prop's task, or null for an empty slot */
struct task *event_prop_get(int slot);
/* kill the prop's task, if the slot holds one, and empty the slot */
void event_prop_kill(int slot);

#endif
