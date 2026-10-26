/* Chaos 6's bombs, the satellites that spray frost on the deck */
#ifndef B_CHAOS6_BOMB_H
#define B_CHAOS6_BOMB_H
#include <sg_xpt.h>

#include "system/task.h"

struct bomb_vars {
    Uint8 unk_00[4];
    /* the steps left in a walk before the bomb aims again, or the steps a
     * waiting bomb has waited */
    Sint32 steps;
    /* the bomb's slot in the ring round the boss, or -1 for one Eggman
     * drops */
    /* @hack type: slot as an array of one, which forms its address before
     * each read and write (MOV R4,R3 and ADD #16,R3 at 0x0c9042e4); as a
     * Sint8, 468 of 1300 words differ */
    Sint8 slot[1];
};

#define BOMB_VARS(work) ((struct bomb_vars *)&(work)->vars)

/* the bombs that have taken their first step and not yet been destroyed */
extern Sint16 chaos6_bomb_count;

/* a bomb's first step, for a task with a work and a motion. its maker sets
 * work->pos and BOMB_VARS(work)->slot before the step runs. the bomb starts
 * at pos moving up, and falls to the deck */
void chaos6_bomb_step(struct task *task);
/* the step of the ring of six bombs that circles the boss, for a task with
 * a work. the bombs are children of its task, which steps them. it makes a
 * bomb in each empty slot on each step the boss is in BOSS_START,
 * BOSS_CHASE, BOSS_STRIKE or BOSS_WEAVE */
void chaos6_bomb_ring_step(struct task *task);

#endif
