/* Chao race hall gates, eight in an arc, one for each racing Chao */
#ifndef AL_RACE_GATE_H
#define AL_RACE_GATE_H
#include <ninja.h>
#include <sg_xpt.h>

#define GATE_COUNT 8

/* add the hall's eight gates to the scenery, and reset the count of Chao
 * through them */
void gates_create(void);
/* store in pos where a Chao waits before gate index, and return TRUE.
 * FALSE, storing nothing, for a null pos or an index outside 0 to 7 */
Bool gate_get_spot(NJS_POINT3 *pos, int index);
/* store in pos the point 10 units beyond gate index, on its far side, and
 * return TRUE. FALSE, storing nothing, for a null pos or an index outside 0
 * to 7 */
Bool gate_get_far_side(NJS_POINT3 *pos, int index);
/* TRUE once every gate has let its Chao through */
Bool gates_all_passed(void);

#endif
