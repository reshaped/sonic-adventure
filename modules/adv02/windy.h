/* the Mystic Ruins' winds. O CHG WINDY is the whirlwind into Windy Valley, and
 * O CHG CAVE and O CHG CAVE2 are the cave's two updrafts. O W LEAF1, O W LEAF2
 * and O W BOARD are leaves and a board that drift on the wind */
#ifndef ADV02_WINDY_H
#define ADV02_WINDY_H

#include "system/task.h"

/* the step the object list names for O CHG WINDY. one whirlwind runs at a
 * time */
void windy_entrance_step(struct task *task);
/* the steps the object list names for O CHG CAVE and O CHG CAVE2. the first is
 * the updraft up to the island act, and the second the updraft down to the
 * station act */
void windy_cave_up_step(struct task *task);
void windy_cave_down_step(struct task *task);
/* the step the object list names for O W LEAF2, which each leaf the whirlwind
 * throws up runs too */
void windy_leaf_2_step(struct task *task);
/* the steps the object list names for O W BOARD and O W LEAF1 */
void windy_board_step(struct task *task);
void windy_leaf_1_step(struct task *task);

#endif
