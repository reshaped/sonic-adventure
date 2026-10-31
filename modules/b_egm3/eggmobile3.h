/* the Egg Viper's stage: its stage and sky steps, and the boss's tasks the
 * stage starts */
#ifndef B_EGM3_EGGMOBILE3_H
#define B_EGM3_EGGMOBILE3_H
#include <ninja.h>
#include <sg_xpt.h>

#include "system/task.h"

/* the boss's segments and its needles. a segment's work->tick is its
 * number, from 0, ORed with its kind: 0, EGG_VIPER_JOINT or EGG_VIPER_TAIL */
#define EGG_VIPER_SEGMENTS 13
#define EGG_VIPER_NEEDLES 2
#define EGG_VIPER_JOINT 0x10
#define EGG_VIPER_TAIL 0x20

/* each segment's work pos and ang. segment n + 1 trails segment n */
extern NJS_POINT3 *egg_viper_segment_pos[EGG_VIPER_SEGMENTS];
extern struct angles *egg_viper_segment_ang[EGG_VIPER_SEGMENTS];
extern struct task *egg_viper_task;
extern struct task *egg_viper_needles[EGG_VIPER_NEEDLES];
extern struct task *egg_viper_segments[EGG_VIPER_SEGMENTS];

/* the stage's step and its sky's, which 1ST_READ.BIN's stage_steps and
 * sky_steps name */
void eggmobile3_step(struct task *task);
void eggmobile3_sky_step(struct task *task);

#endif
