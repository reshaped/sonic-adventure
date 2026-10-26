/* Zero's trigger boxes, which the set editor names EROB COLLI. Amy or Zero
 * coming into one sets it off. a box ends Zero or sends Zero off, brings
 * Zero in, blinds Zero to Amy, or pushes Zero back out */
#ifndef OBJECT_ZERO_TRIGGER_H
#define OBJECT_ZERO_TRIGGER_H

struct task;

/* the boxes' step in the object lists. a box takes its type from the low
 * byte of its ang.x. this runs the box's own step at once, and hands the
 * task on to it */
void zero_trigger_start(struct task *task);

#endif
