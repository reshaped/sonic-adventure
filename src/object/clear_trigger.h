/* the clear trigger, O CLROBJ in the object lists. player 0's touch clears
 * the stage. in STAGE_MOUNTAIN, STAGE_RUIN and STAGE_FINAL, once Sonic has
 * cleared the stage, a capsule stands in the trigger's place instead */
#ifndef OBJECT_CLEAR_TRIGGER_H
#define OBJECT_CLEAR_TRIGGER_H

struct task;

/* the object lists' step for O CLROBJ */
void clear_trigger_step(struct task *task);

#endif
