/* O_Ele, O_Fiw4m, O_Fiw5m and O_Fiw6m: solid rocks */
#ifndef STG08_ROCKS_H
#define STG08_ROCKS_H

#include "system/task.h"

/* the object list's steps */
void icicle_rock_step(struct task *task);
void rock_4m_step(struct task *task);
void rock_5m_step(struct task *task);
void rock_6m_step(struct task *task);

#endif
