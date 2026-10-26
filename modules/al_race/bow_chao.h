/* BOW CHAO, a Chao in the race hall that bows as the hall's door starts to
 * open from shut, and as something touches it */
#ifndef AL_RACE_BOW_CHAO_H
#define AL_RACE_BOW_CHAO_H

#include "system/task.h"

/* BOW CHAO's step in the race's object kinds. it gives the task the Chao's
 * record and callbacks, and starts it */
void bow_chao_init(struct task *task);

#endif
