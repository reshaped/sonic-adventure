/* C APP ZERO flags, which the robo walls and the robo shutter wait on */
#ifndef STG10_ZERO_APPEAR_H
#define STG10_ZERO_APPEAR_H
#include <sg_xpt.h>

/* how many flags there are. zero_appear_is_on and zero_appear_set require an
 * id from 0 to ZERO_APPEAR_FLAGS - 1, and don't check it */
#define ZERO_APPEAR_FLAGS 16

/* clear every flag. Final Egg's stage task calls this when it starts in
 * act 0 */
void zero_appear_reset(void);
/* TRUE once flag id has been set since the last zero_appear_reset, else
 * FALSE */
Bool zero_appear_is_on(int id);
/* set flag id. a C APP ZERO sets the flag its set entry numbers when it's
 * touched */
void zero_appear_set(int id);

#endif
