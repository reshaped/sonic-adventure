/* EV0050.PRS's parked cars: ADV00.PRS's parked car, drawn with its lights
 * on */
#ifndef EV0050_CAR_H
#define EV0050_CAR_H

#include "system/task.h"

/* create a car with an event record, or return NULL if task_create fails.
 * the car draws ADV00.PRS's parked car, so ADV00.PRS must hold the stage
 * slot while the car lives. it collides with nothing until event_set_radius
 * gives it a radius */
struct task *ev0050_car_create(void);

#endif
