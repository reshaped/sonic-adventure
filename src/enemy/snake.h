/* the snake, a line of segments behind a lead that circles where the
 * snake was placed. they burst once one is hit */
#ifndef ENEMY_SNAKE_H
#define ENEMY_SNAKE_H
#include <ninja.h>

struct task;

/* E_SNAKE, the snake's PVM */
extern NJS_TEXLIST e_snake_texlist;

/* the step Windy Valley's, Lost World's and Ice Cap's object lists name
 * E HEVY. scl.z is the radius of the lead's circle, held to 60 to 150.
 * ang.z's low byte is the master emerald piece the snake carries, which
 * Knuckles takes once every segment has burst */
void snake_step(struct task *task);

#endif
