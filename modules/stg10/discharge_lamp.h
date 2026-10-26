/* O DSCHG_LAMP bobbing lamp whose beam goes on and off for the times its set
 * entry gives. an attack bursts it into four pieces */
#ifndef STG10_DISCHARGE_LAMP_H
#define STG10_DISCHARGE_LAMP_H

#include "system/task.h"

/* the object list's step */
void discharge_lamp_step(struct task *task);

#endif
