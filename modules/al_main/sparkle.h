/* the Chao's sparkles. a glint is one sparkle. spirals, trails and bursts
 * leave glints behind them, and flecks of glitter drift down */
#ifndef AL_MAIN_SPARKLE_H
#define AL_MAIN_SPARKLE_H
#include <ninja.h>

/* create a glint of the given scale at pos. it plays a third of a sprite
 * frame each frame, and ends after its fourth. it sinks faster each frame,
 * and draws added to what is behind it */
void alife_sparkle_create(NJS_POINT3 *pos, Float scale);
/* create a spiral of glints round pos, three turns of 16 steps. each step
 * leaves a glint 3 out from pos, a sixteenth of a turn on from the last. the
 * glints rise 1 a turn, each within 0.5 of that at random */
void alife_sparkle_spiral_create(NJS_POINT3 *pos);
/* create ten trails that start 5 from pos, each a random way, and close in
 * on it. each lives 30 steps, and each step takes it 0.8 / 30 of the way in.
 * each leaves a glint every 16 steps */
void alife_sparkle_gather_create(NJS_POINT3 *pos);
/* create ten trails thrown out from pos, each a random way up. each lives 15
 * steps, and its speed up y falls 0.1 a step, to -1. each leaves a glint
 * every 8 steps */
void alife_sparkle_burst_create(NJS_POINT3 *pos);
/* create a fleck of glitter near pos, at a random offset within 0.5 on each
 * axis. it plays its eight sprite frames as it drifts, slowing on x and z
 * and falling faster. alife_glitter_create_slow's fleck starts at a tenth of
 * its offset a step, and plays 8 / 90 of a frame a step.
 * alife_glitter_create's starts at its offset a step, and plays 8 / 30 */
void alife_glitter_create_slow(NJS_POINT3 *pos);
void alife_glitter_create(NJS_POINT3 *pos);

#endif
