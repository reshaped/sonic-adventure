/* the ladder the object list names O LADDER, which players climb along a
 * ledge path */
#ifndef ADV02_LADDER_H
#define ADV02_LADDER_H
#include <ninja.h>

#include "system/task.h"

/* the ladder, whose model O LONG LADDER and the station's ladder draw too */
/* @hack type: ladder_object declared as an array, and read through ->. SHC
 * then loads its address and reads its model 4 bytes past it: MOV.L
 * @(4,R4),R4 at 0x0c90ce78 in ladder_draw, its address pooled at 0x0c90cebc,
 * and at 0x0c9149c8 and 0x0c9149de in station_ladder_ladder_draw. as one
 * NJS_OBJECT read through ., 2 of ladder_draw's 34 words and a pool word
 * differ, and 6 of station_ladder_ladder_draw's 43. long_ladder.c builds the
 * same either way */
extern NJS_OBJECT ladder_object[];

/* the step the object list names for O LADDER. it builds the ladder's path,
 * then hands the task to the ladder's own step */
void ladder_start(struct task *task);

#endif
