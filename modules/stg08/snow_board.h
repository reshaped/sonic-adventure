/* O SnowBoard, the stand player 0 takes the snowboard from. a player's touch
 * takes the board, and the stand's pieces then spin, sink and shrink away */
#ifndef STG08_SNOW_BOARD_H
#define STG08_SNOW_BOARD_H

#include "system/task.h"

/* the step icecap_object_list names for O SnowBoard */
void snow_board_step(struct task *task);

#endif
