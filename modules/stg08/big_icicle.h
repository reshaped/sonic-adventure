/* O BIGTURARA: a big icicle that falls after the small icicles around it.
 * O FUTAL, O FUTAM and O FUTAM2: ice lids (futa) that break when struck */
#ifndef STG08_BIG_ICICLE_H
#define STG08_BIG_ICICLE_H

#include <sg_xpt.h>

#include "system/task.h"

/* a big icicle that starts while there is no first big icicle becomes the
 * first, until it's destroyed. TRUE once the first lands, or once the set
 * editor puts it down. FALSE while it hangs or falls, and while there is no
 * first. this reads the first's record without testing it for null */
Bool big_icicle_landed(void);

/* the object list's steps. unless its task is out of range, each takes a
 * record from the heap. outside the set editor, it also takes a world
 * object. it tests neither for null */
void big_icicle_step(struct task *task);
void futa_m_step(struct task *task);
void futa_m2_step(struct task *task);
void futa_l_step(struct task *task);

#endif
