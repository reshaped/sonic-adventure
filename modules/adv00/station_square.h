/* Station Square acts, and the field and sky tasks */
#ifndef ADV00_STATION_SQUARE_H
#define ADV00_STATION_SQUARE_H

#include "system/task.h"

struct death_zone;

/* the zones a player goes down in, a list for each act, ended by a null
 * object. 1ST_READ.BIN's death_zones holds the array's address for
 * Station Square. the stage's generated data holds them */
extern struct death_zone *station_square_death_zones[];

/* the field's acts, as the stage select names them */
#define STATION_SQUARE_OFFICE 0
#define STATION_SQUARE_CASINO 1
#define STATION_SQUARE_GESUI 2 /* the sewers */
#define STATION_SQUARE_STATION 3
#define STATION_SQUARE_HOTEL 4
#define STATION_SQUARE_TWINKLE 5

/* cleared as the office's barricade and doors start, and set as the
 * barricade Knuckles throws lands. the locked office doors open once it is
 * set */
#define FLAG_OFFICE_BARRICADE 4

/* the field's and the sky's task steps, at the module's start, which
 * 1ST_READ.BIN's stage_steps and sky_steps name */
void station_square_step(struct task *task);
void station_square_sky_step(struct task *task);

#endif
