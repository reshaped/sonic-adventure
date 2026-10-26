/* Perfect Chaos's ball of light, which falls short of player 0 and raises a
 * column of water */
#ifndef B_CHAOS7_BALL_H
#define B_CHAOS7_BALL_H

#include <ninja.h>

/* drop a ball of light from boss_work's x and z at a height of 600. it falls
 * at 30 a step to the sea, 150 short of player 0 on the line from the boss.
 * it drops none when the task can't be made. nothing in the image calls
 * it */
void chaos7_ball_create(void);

/* the splash's and the sprite's texlists, which chaos7_fight_pvms fills */
extern NJS_TEXLIST ball_splash_texlist;
extern NJS_TEXLIST ball_sprite_texlist;

#endif
