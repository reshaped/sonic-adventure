/* the Perfect Chaos fight: its places, its runs, the boss's attacks and the
 * screen's fades */
#ifndef B_CHAOS7_FIGHT_H
#define B_CHAOS7_FIGHT_H

#include <sg_xpt.h>

#include "system/task.h"

/* the run's sound. the fight plays it held as a run starts, and stops it as
 * the run ends. in BOSS_RUN, the boss keeps one playing for his own work */
#define SE_RUN 0x1c1

/* the camera's height above player 0, and its distance behind him */
extern Float chaos7_camera_height;
extern Float chaos7_camera_back;

/* chaos7_part_2 is set for the fight's second part, and chaos7_won once the
 * fight is won */
extern Sint8 chaos7_part_2;
extern Sint8 chaos7_won;

/* the fight's step, for a task with a work. it makes the boss as a child, and
 * runs the fight from the opening to the clear. the task's destructor kills
 * the boss */
void chaos7_fight_step(struct task *task);
/* put the boss's task at place, 0 to 2, and give it that place's heading.
 * chaos7_boss_heading takes the heading too */
void chaos7_place_boss(struct task *task, int place);
/* cover the screen in the shade gray, from 0 for black to 255 for white. the
 * cover's alpha, 0 to 255, goes from from to to over steps steps of the
 * fight's task. the task's next delay steps then ignore any other call */
void chaos7_fade_start(int from, int to, int gray, int steps, int delay);

/* a run tells the fight as it takes player 0, and as it lets him go. both do
 * nothing without the fight's task. the end stops the run's sound, then
 * does nothing more unless the fight is in a run */
void chaos7_run_start(void);
void chaos7_run_end(void);
/* TRUE when three things hold: the fight is at place, 0 to 2; it is on or in
 * a run; and the boss is in none of BOSS_FORM, BOSS_MELT and BOSS_MELTED.
 * FALSE otherwise, and without the fight's task */
Bool chaos7_run_open(int place);

/* return 0. nothing in this image calls it */
int fun_0c9170ba(void);

#endif
