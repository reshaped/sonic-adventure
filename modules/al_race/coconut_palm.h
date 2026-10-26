/* the palms of a race of kind 4, one to a lane, which each lane's Chao
 * sways until its coconut drops */
#ifndef AL_RACE_COCONUT_PALM_H
#define AL_RACE_COCONUT_PALM_H
#include <ninja.h>
#include <sg_xpt.h>

#include "system/task.h"

/* the palm's item kind, coconut_palm_tell's index in item_tells */
#define COCONUT_PALM_KIND 0

/* the commands a Chao sends the palm it claimed, as it starts to sway the
 * palm and as it stops */
#define COCONUT_PALM_SWAY 1
#define COCONUT_PALM_STOP 2

/* create a palm for each lane, where coconut_palm_get_place says, as a child
 * of the scenery task. without the scenery task, it creates none */
void coconut_palm_create(void);
/* the palm's handler. a command takes effect only while other is the Chao
 * that claimed the palm. ITEM_LET_GO drops the claim. COCONUT_PALM_SWAY
 * starts the sway the claim waits for. COCONUT_PALM_STOP drops the claim of
 * a swaying palm, which plays its sway to the end and stands still. returns
 * 1 when the command takes effect, else 0 */
int coconut_palm_tell(struct task *task, struct task *other, Sint8 command);
/* store in pos where lane's palm stands, and return 1. return 0, storing
 * nothing, for a null pos or a lane outside 0 to RACE_LANES - 1 */
int coconut_palm_get_place(NJS_POINT3 *pos, int lane);

#endif
