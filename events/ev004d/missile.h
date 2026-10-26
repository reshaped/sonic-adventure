/* Eggman's missile, in EV004D.PRS. as the MISSILE_ bits ask, its lamps
 * glow, it trails smoke and its band's texture rolls */
#ifndef EV004D_MISSILE_H
#define EV004D_MISSILE_H
#include <ninja.h>
#include <sg_xpt.h>

#include "system/task.h"

/* what the missile shows, as bits of its work's tick */
#define MISSILE_LAMPS 0x1 /* its four lamps glow */
#define MISSILE_EXHAUST 0x2 /* smoke from its nozzle, a puff every 4 steps */
#define MISSILE_SCROLL 0x4 /* the texture of its band rolls */

/* EGG_MISSILE, loaded through ev004d_textures */
extern NJS_TEXLIST ev004d_missile_texlist;

/* a new missile's task, or null when task_create fails. its first step
 * gives it an event record. that step also sets its scale to 1 and its bits
 * to MISSILE_LAMPS | MISSILE_SCROLL */
struct task *ev004d_missile_create(void);
/* set the missile's scale on every axis, or the MISSILE_ bits it shows.
 * call them after its first step, which sets both. task must not be null:
 * each reads task->work before it tests task */
void ev004d_missile_set_scale(struct task *task, Float scale);
void ev004d_missile_set_effects(struct task *task, int effects);

#endif
