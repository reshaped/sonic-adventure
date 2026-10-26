/* O Hyo1NoExp to O Hyo3Exp: ice blocks in three sizes, standing or
 * breakable */
#ifndef STG08_ICE_H
#define STG08_ICE_H

#include "system/task.h"

/* the object list's steps for O Hyo1NoExp, O Hyo1Exp, O Hyo2NoExp, O Hyo2Exp,
 * O Hyo3NoExp and O Hyo3Exp */
void icecap_ice_1_step(struct task *task);
void icecap_ice_1_breakable_step(struct task *task);
void icecap_ice_2_step(struct task *task);
void icecap_ice_2_breakable_step(struct task *task);
void icecap_ice_3_step(struct task *task);
void icecap_ice_3_breakable_step(struct task *task);

#endif
