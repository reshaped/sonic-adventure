/* land textures and night lights for the story's time of day */
#ifndef ADV00_LAND_TIME_H
#define ADV00_LAND_TIME_H

/* set the act's land textures and night lights for the story's time of day.
 * a time other than TIME_DAY, TIME_EVENING or TIME_NIGHT changes nothing.
 * station_square_act_start calls it as each act starts */
void land_time_set(void);

#endif
