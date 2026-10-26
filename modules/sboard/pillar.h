/* PILLAR S, M and L, stones that slide along their own y as a player comes
 * near */
#ifndef SBOARD_PILLAR_H
#define SBOARD_PILLAR_H

#include "system/task.h"

/* task steps for PILLAR S, M and L, named in the object list */
void pillar_s_step(struct task *task);
void pillar_m_step(struct task *task);
void pillar_l_step(struct task *task);

#endif
