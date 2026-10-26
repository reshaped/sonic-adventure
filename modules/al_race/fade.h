/* the Chao race's full-screen gray fade */
#ifndef AL_RACE_FADE_H
#define AL_RACE_FADE_H

/* race_fade_in clears the gray, and race_fade_out makes it opaque, from the
 * gray as it is. each spreads the change evenly over that many of the fade
 * task's steps. where steps is 0 or less, the change comes at its next
 * step. without the fade's task, neither does anything */
void race_fade_in(int steps);
void race_fade_out(int steps);
/* create the fade's task, with the gray clear, unless it exists already */
void race_fade_create(void);

#endif
