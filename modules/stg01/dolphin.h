/* O DOLPHIN, a dolphin that leaps from the sea, and O DOLSW, a switch that
 * calls dolphins */
#ifndef STG01_DOLPHIN_H
#define STG01_DOLPHIN_H

#include "system/task.h"

/* the steps the object list names for O DOLPHIN and O DOLSW. each task
 * needs a work. a dolphin's needs an extra too, where it keeps its home and
 * the steps since its last spray. a switch's set entry gives its id in scale
 * y and a count in scale z. a hit calls each dolphin with the id in scale y
 * and a scale x below the count. a called dolphin leaps once. its scale z
 * picks leap a when below 0, and leap b otherwise */
void dolphin_step(struct task *task);
void dolphin_switch_step(struct task *task);

#endif
