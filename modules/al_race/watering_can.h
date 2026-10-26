/* WATERING CAN, a can that a racing Chao picks up and waters with */
#ifndef AL_RACE_WATERING_CAN_H
#define AL_RACE_WATERING_CAN_H
#include <sg_xpt.h>

#include "system/task.h"

/* the can's item kind, watering_can_tell's index in item_tells */
#define WATERING_CAN_KIND 3

/* the commands a Chao sends the can it holds, as it starts to water with the
 * can and as it stops */
#define WATERING_CAN_WATER 7
#define WATERING_CAN_DONE 8

/* the can's step in the object list, which makes the task a watering can */
void watering_can_init(struct task *task);
/* take command from other, and return 1 when it takes effect, else 0. only a
 * command from the Chao that holds the can takes effect. ITEM_LET_GO lets go
 * of the can. WATERING_CAN_WATER starts watering with a held can, and
 * WATERING_CAN_DONE stops the watering, lets go and leaves the can used */
int watering_can_tell(struct task *task, struct task *other, Sint8 command);

#endif
