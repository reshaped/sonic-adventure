/* Station Square acts, and the field and sky tasks */
#ifndef ADV00_STATION_SQUARE_H
#define ADV00_STATION_SQUARE_H

#include "system/task.h"

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

/* the field's and the sky's task steps, which 1ST_READ.BIN's stage_steps
 * and sky_steps name by their addresses at the module's start */
void station_square_step(struct task *task);
void station_square_sky_step(struct task *task);

#endif
