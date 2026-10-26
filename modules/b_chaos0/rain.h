/* Chaos 0's rain: drops that fall ahead of the camera and splash */
#ifndef B_CHAOS0_RAIN_H
#define B_CHAOS0_RAIN_H

/* start the rain's task, which makes the drops. call it only while the rain
 * is stopped: a second start loses the first task, which chaos0_rain_stop
 * then never destroys */
void chaos0_rain_start(void);
/* destroy the rain's task, unless the rain has stopped since the last
 * chaos0_rain_start. the drops already falling live on */
void chaos0_rain_stop(void);

#endif
