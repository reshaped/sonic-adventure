/* C SCENECHANGER: a box that changes the stage or act once touched */
#ifndef STAGE_SCENE_CHANGER_H
#define STAGE_SCENE_CHANGER_H

#include "system/task.h"

/* go to destination: stage in bits 8-15, entrance in 4-7 and act in 0-3,
 * so STAGE_ACT(stage, act) is the destination with entrance 0. on the
 * current stage it changes act in place, destroys the streams' task and
 * places player 0 as stage_place_player does. another stage ends the act as
 * a clear, and makes the destination's stage and act the next. either way,
 * it sets the entrance first */
void scene_changer_go(int destination);
/* C SCENECHANGER's object-list entry: a box the placement's scale sizes.
 * outside the set editor, the task rises by the box's half height, so the
 * box stands on the placed position. once touched while the pads drive the
 * game, it goes to the destination its ang.z holds */
void scene_changer_init(struct task *task);

#endif
