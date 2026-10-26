/* SCENE CHANGE box that sends player 0 to the stage its set entry gives */
#ifndef ADV00_SCENE_CHANGE_H
#define ADV00_SCENE_CHANGE_H

#include "system/task.h"

/* the step the object list names for SCENE CHANGE and SCENE CHANGE SS. a
 * placement's angle z packs the destination's stage, entrance and act. its
 * angle y turns the box, and its scale sizes it */
void station_square_scene_change_init(struct task *task);

#endif
