/* O Dd, the snow that falls around player 0 on the board run */
#ifndef STG08_SNOWFALL_H
#define STG08_SNOWFALL_H
#include <ninja.h>

#include "system/task.h"

/* the flakes' sprite, which the snow slides' powder draws too */
extern NJS_SPRITE snowfall_sprite;

/* the step icecap_object_list names for O Dd. the snow waits until player 0
 * comes down to a y of -6650, snowfall.c's SNOW_START */
void snowfall_step(struct task *task);

#endif
