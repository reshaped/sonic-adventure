/* O TENSS and O TENSC, gates under a slot sign and a card sign. each sign
 * steps through its faces, and each gate's lights run while a teleporter
 * holds player 0 */
#ifndef STG09_TRANSFER_GATE_H
#define STG09_TRANSFER_GATE_H

#include "system/task.h"

/* the steps the object list names for O TENSS and O TENSC. the sign shows each
 * face for scl.z + 2 steps, scl.z truncated, and 1 at least. each step the
 * lights run, their scroll phase moves on by twice the truncated scl.y */
void transfer_gate_slot_step(struct task *task);
void transfer_gate_card_step(struct task *task);

#endif
