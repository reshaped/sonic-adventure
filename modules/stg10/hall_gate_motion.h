/* the hall gate's opening motion, and the action that plays it on
 * hall_gate_object */
#ifndef STG10_HALL_GATE_MOTION_H
#define STG10_HALL_GATE_MOTION_H
#include <ninja.h>

/* @hack type: hall_gate_action declared as an array of one, and read as
 * hall_gate_action->motion. SHC then loads the action's address and reads its
 * motion 4 bytes past it (MOV.L @(4,R3),R2 at 0x0c91369c). as one NJS_ACTION
 * read as hall_gate_action.motion, 132 of hall_gate_step's 159 words and 9 of
 * hall_gate_run's 75 differ */
extern NJS_ACTION hall_gate_action[];

#endif
