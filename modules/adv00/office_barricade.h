/* O OFFICE BARRICADE barricade before the office, which Knuckles knocks down
 * the road */
#ifndef ADV00_OFFICE_BARRICADE_H
#define ADV00_OFFICE_BARRICADE_H

#include "system/task.h"

/* the step the object list names for O OFFICE BARRICADE. outside the set
 * editor, it kills its placement unless player 0 is Knuckles and
 * FLAG_KNUCKLES_SS_BARRICADE is clear. a barricade it keeps clears
 * FLAG_OFFICE_BARRICADE, which the thrown barricade sets as it lands */
void office_barricade_init(struct task *task);

#endif
