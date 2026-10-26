/* the fence of eight posts around the deck, joined by chains of light */
#ifndef B_ROBO_FENCE_H
#define B_ROBO_FENCE_H

/* create the fence, whose task idles until erobo_fence_start. when
 * task_create or heap_alloc fails, there is no fence */
void erobo_fence_create(void);
/* on the fence task's next step, raise the posts 250 to 350 above the
 * deck's floor. they then fall onto it. the fence's shapes take part in the
 * checks from the step after. with no fence, it does nothing */
void erobo_fence_start(void);
/* take the fence's shapes out of the checks. each post then waits 1 to 29
 * steps, counted while it is down. its chains narrow away over at most 90
 * steps as it crackles. it then smokes for 240 steps, and is no longer
 * drawn. with no fence, it does nothing */
void erobo_fence_stop(void);

#endif
