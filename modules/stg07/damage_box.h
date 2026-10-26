/* O R DBOX and O RNDBOX, boxes that rise and sink, and UkiUki, a model */
#ifndef STG07_DAMAGE_BOX_H
#define STG07_DAMAGE_BOX_H

#include "system/task.h"

/* the steps ruin_object_list names for O R DBOX, O RNDBOX and UkiUki */
void damage_box_step(struct task *task);
void no_damage_box_step(struct task *task);
void ukiuki_step(struct task *task);

#endif
