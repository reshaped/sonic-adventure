/* Knuckles' item trigger boxes, KN ITEM COLLI. the set editor draws and
 * labels each box. in the game a box does nothing: it ends for good once
 * player 0 is gone, and leaves once it is out of range */
#ifndef OBJECT_KNUCKLES_TRIGGER_H
#define OBJECT_KNUCKLES_TRIGGER_H

struct task;

/* the step a box starts with: it takes the box's settings, steps it once
 * and hands the task over to the box's own step */
void knuckles_trigger_start(struct task *task);

#endif
