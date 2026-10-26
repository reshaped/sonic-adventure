/* Chaos 2's flash and ball sprites, and the fade of the land's objects
 * that stand in the camera's way */
#ifndef B_CHAOS2_EFFECTS_H
#define B_CHAOS2_EFFECTS_H
#include <ninja.h>
#include <sg_xpt.h>

#include "system/task.h"

/* a task's step that plays a flash at the work's pos. the flash plays its
 * sprite's 8 frames over 30 steps, and destroys its task on the 31st */
void chaos2_flash_step(struct task *task);
/* draw the ball sprite in the current matrix, added to what lies behind
 * it, at the frame in the Float that starts work's vars. drop_draw draws
 * it on each drop until the drop lands */
void chaos2_ball_sprite_draw(struct task_work *work);

/* while this is nonzero, each of the land's fading objects fades where
 * effects.c finds it in the camera's way. the boss sets it as the fight
 * begins */
extern Sint8 chaos2_land_fade_on;
/* the first step of the task that fades the land's objects. it finds
 * their entries in chaos2_land, makes them opaque and clears
 * chaos2_land_fade_on. from then on the task fades and draws them on each
 * frame's first step, and in its draw, which task_draw_all runs under the
 * pause menu */
void chaos2_land_fade_step(struct task *task);

#endif
