/* the confetti the crackers fire: pieces that rise, then drift down to land */
#ifndef AL_RACE_CONFETTI_H
#define AL_RACE_CONFETTI_H
#include <ninja.h>

/* the most pieces in flight at once */
#define CONFETTI_COUNT 1024

/* fire about count pieces from pos at speed, each axis of each piece's
 * speed varied by up to 0.2 either way. some pieces are slowed: the first
 * rarely but hard, the last often but gently. without the confetti task, it
 * fires none */
void confetti_create(NJS_POINT3 *pos, NJS_VECTOR *speed, int count);
/* create the confetti task, unless there is one, with room for
 * CONFETTI_COUNT pieces, and set up the streamers and the butterflies.
 * after a failed allocation, it frees what it took and creates no task, and
 * the streamers and the butterflies stay as they were */
void confetti_task_create(void);
/* add a piece at pos moving at speed, unless there is no confetti task or
 * CONFETTI_COUNT pieces are in flight */
void confetti_add(NJS_POINT3 *pos, NJS_VECTOR *speed);

#endif
