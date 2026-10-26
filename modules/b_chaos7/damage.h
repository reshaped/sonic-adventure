/* Perfect Chaos's damage effects: bursts, drops, mist and ripples */
#ifndef B_CHAOS7_DAMAGE_H
#define B_CHAOS7_DAMAGE_H

#include <ninja.h>
#include <sg_xpt.h>

#include "system/task.h"

/* the texlist chaos7_act_pvms loads CHAOS7_0DAMAGE into */
extern NJS_TEXLIST chaos7_damage_texlist;
/* 1 to show the mist, and 0 to hide it. chaos7_mist_fade moves the mist's
 * strength toward it */
extern Sint8 chaos7_mist_on;

/* a drop's first step. the drop falls from the task's position at a size of
 * 0.5 to 1.5. its life gains 1/180 a step, and it ends once that reaches 1. it
 * leaves a splash every four steps. the task needs a motion, whose speed the
 * drop starts at */
void chaos7_damage_drop_start(struct task *task);
/* a step that makes bursts round the task's position, then ends the task. its
 * creator sets the count of bursts in vars.b[4] and the steps before the
 * first in vars.b[5]. it sets their spread in vars.f[2]. the bursts come nine
 * steps apart. each lands from half the spread out to all of it */
void chaos7_damage_bursts_step(struct task *task);
/* a mist puff's first step. the puff appears 500 to 650 from Chaos toward
 * player 0, and 400 to 600 to one side. it drifts while it comes and goes, then
 * appears again elsewhere. the task needs a motion */
void chaos7_mist_start(struct task *task);
/* a step that moves the mist's strength toward chaos7_mist_on. task is
 * unread */
void chaos7_mist_fade(struct task *task);
/* a ripple's first step. the ripple plays three times, 150 from the task's
 * position along Chaos's facing */
void chaos7_damage_ripple_start(struct task *task);

#endif
