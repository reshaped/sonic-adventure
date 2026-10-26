/* traffic on the roads of the office and station acts */
#ifndef ADV00_TRAFFIC_H
#define ADV00_TRAFFIC_H

#include "system/task.h"

/* event flag 1, which the traffic task sets as it starts. while it is
 * clear, the task steps no car, so no car moves. a car then draws only while
 * the game is paused, when task_draw_all draws every task. clearing the flag
 * also stops the traffic's noise, which setting it again doesn't restart */
#define FLAG_TRAFFIC 1

/* the roads, the office act's and the station act's. only the station road
 * has the light */
enum traffic_road { TRAFFIC_OFFICE, TRAFFIC_STATION };

/* create and return the traffic task for road, a traffic_road taken mod 2. it
 * returns NULL when task_create fails, and while player 0 is Sonic before
 * FLAG_SONIC_SS_BARRICADE is set or Tails before FLAG_TAILS_SS_BARRICADE is.
 * whatever it returns, it clears NJD_FLAG_IGNORE_LIGHT on the police car's
 * material 5 */
struct task *traffic_start(int road);

#endif
