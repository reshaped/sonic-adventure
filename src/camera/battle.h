/* the camera in a battle with a rival */
#ifndef CAMERA_BATTLE_H
#define CAMERA_BATTLE_H
#include <ninja.h>
#include <sg_xpt.h>

struct task;

/* run the battle camera through camera_override. it looks past the rival,
 * player 1, from behind player 0. task's extra must be the struct rival,
 * whose kind sets how far back the camera keeps. camera_start_battle sets
 * the camera up afresh. camera_keep_battle runs it through camera_override
 * again as it was */
void camera_start_battle(struct task *task);
void camera_keep_battle(void);
/* copy camera_work's position into *pos. camera_work must not be null */
void camera_get_pos(NJS_POINT3 *pos);
/* set the most the battle camera swings round player 0 in a step.
 * camera_start_battle sets it back to 0x100, a 256th of a turn */
void camera_set_battle_turn(Angle turn);

#endif
