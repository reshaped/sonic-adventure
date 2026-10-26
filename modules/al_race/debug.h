/* the race's debug buttons, on the second pad */
#ifndef AL_RACE_DEBUG_H
#define AL_RACE_DEBUG_H

struct race_game;

/* on the second pad, a press of A turns the crackers on or off. a press of
 * L sets the race's camera mode 10, the finish's. nothing in the image calls
 * it */
void race_debug_step(struct race_game *game);

#endif
