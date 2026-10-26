/* the race: the lights, the laps the zones count, and the clock */
#ifndef MINICART_RACE_H
#define MINICART_RACE_H
#include <sg_xpt.h>

#include "system/task.h"
#include "system/task_desc.h"

enum race_mode {
    RACE_NONE,
    RACE_READY, /* waiting for the lights */
    RACE_COUNTDOWN,
    RACE_GO,
    RACE_LAP, /* a lap's split blinks as the race goes on */
    RACE_FINISH, /* the run's time shows, and its gap to the best blinks */
    RACE_OVER /* the run's time shows */
};

/* the zones race_enter_zone takes, in the order a lap passes them. the home
 * stretch runs from ZONE_NEXT to ZONE_GOAL */
enum race_zone { ZONE_GOAL, ZONE_PREVIOUS, ZONE_NEXT };

/* what the dial shows as the lights go green, and on the last lap's home
 * stretch */
#define DIAL_GO 4
#define DIAL_HOME_STRETCH 7

/* create the race's task at level 2, and return it, or null when it can't
 * be created. the task resets the race as it starts. cart_race_kill ends
 * the task on its next step, and cart_race_destroy ends it now. either
 * leaves no race for the functions below */
struct task *cart_race_create(void);
void cart_race_kill(void);
void cart_race_destroy(void);
/* create desc's task with arg as the race's first child, and return it. the
 * race's step steps it. null without the race, or when the child can't be
 * created */
struct task *cart_race_create_child(const struct task_desc *desc, void *arg);
/* set the race waiting for the lights, with its clocks at 0 and the save's
 * best times. TRUE, or FALSE without the race */
Bool cart_race_reset(void);
/* start the lights' countdown if the race is RACE_READY. TRUE if it starts,
 * else FALSE */
Bool cart_race_start_lights(void);
/* the race's mode, or RACE_NONE without the race */
Sint8 cart_race_mode(void);
/* what the race's dial shows. it is 0 without the race or before the first
 * light. it counts 1 to 3 as the lights beep, and is DIAL_GO as they go
 * green. on the furthest lap begun, entering ZONE_NEXT from ZONE_PREVIOUS
 * adds 1, up to DIAL_HOME_STRETCH. going back takes 1 away, down to DIAL_GO */
Sint8 cart_race_dial(void);
/* tell the race the player has entered zone, a race_zone. entering
 * ZONE_GOAL from ZONE_NEXT begins lap 0, and later finishes the lap the
 * player is on. the first time a lap is finished, the race keeps its split.
 * it then blinks the split, or after the last lap ends the race. entering
 * ZONE_NEXT from ZONE_GOAL takes a lap back. without the race nothing
 * changes */
void race_enter_zone(Sint8 zone);

#endif
