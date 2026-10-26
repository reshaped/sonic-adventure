/* the race's splits and lap times, the save's best, and the start boost */
#ifndef MINICART_TIMES_H
#define MINICART_TIMES_H
#include <sg_xpt.h>

#include "save/minigame.h"

/* the run's time, its split as lap ended, and lap's own time. each comes
 * as minicart/cart_time.h's cart_time_of makes it from hundredths. lap must
 * be 0 to 2, and lap 2's split is the run's time */
struct cart_time cart_get_time(void);
struct cart_time cart_get_split(Sint8 lap);
struct cart_time cart_get_lap(Sint8 lap);
/* make the task that gives the start boost. a boost needs A's last press
 * before the race goes to come at most 2 steps early. a press on the step
 * the race goes is 0 steps early */
void boost_start(void);
/* the save's best time for the character and act playing, and the best
 * run's split as lap ended. each is in hundredths. lap must be 0 to 2, and
 * lap 2's split is the best time */
int cart_best_time(void);
int cart_best_split(Sint8 lap);
/* keep the run's time as lap ended, or lap's own time, in hundredths. lap
 * must be 0 to 2 */
void cart_set_split(Sint8 lap, int time);
void cart_set_lap(Sint8 lap, int time);

#endif
