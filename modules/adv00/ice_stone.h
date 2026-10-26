/* O KEYICE the Ice Stone, which Sonic, Tails and Big carry from Station Square
 * to the Mystic Ruins */
#ifndef ADV00_ICE_STONE_H
#define ADV00_ICE_STONE_H

#include "system/task.h"

/* the step the object list names for O KEYICE. the stone lies for Sonic and
 * Tails in the casino act, and for Big in the station act. it lies there from
 * the character's SS_ICE_STONE flag until its MR_ICE_STONE flag is set. while
 * the stone doesn't lie, init stays the task's step and tests again each step.
 * picking the stone up shows a message, and a dropped stone goes once it has
 * lain 1801 steps */
void ice_stone_init(struct task *task);

#endif
