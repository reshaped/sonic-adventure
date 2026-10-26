/* the coconut a palm drops as its lane's Chao swings it, an item of
 * PALM_COCONUT_KIND */
#ifndef AL_RACE_PALM_COCONUT_H
#define AL_RACE_PALM_COCONUT_H
#include <ninja.h>
#include <sg_xpt.h>

#include "system/task.h"

/* the coconut's index in item_tells */
#define PALM_COCONUT_KIND 1

/* the commands the coconut's owner sends it through item_tell, beside
 * ITEM_LET_GO */
#define PALM_COCONUT_EAT 3 /* the owner's Chao starts eating the coconut */
#define PALM_COCONUT_EATEN 4 /* the owner's Chao has eaten it */

/* drop a coconut for lane, which must be 0 to 7, at pos, turned by ang. the
 * coconut is the scenery task's child, and without the scenery task none
 * drops. owner is the task of the Chao that swung the palm, and must stay
 * alive while the coconut keeps it. at rest, the coconut asks owner to come
 * for it with ITEM_GO_TO, and stays there either way. if owner takes the
 * request, the coconut asks owner's Chao to eat it with ITEM_EAT once the
 * Chao touches it. the coconut shrinks as the Chao eats it */
void palm_coconut_create(int lane, NJS_POINT3 *pos, struct angles *ang,
                         struct task *owner);
/* the coconut's handler in item_tells, for command from other. only the
 * coconut's owner's commands take effect. ITEM_LET_GO lets go of the owner.
 * PALM_COCONUT_EAT starts the eating, once the coconut has asked the owner's
 * Chao to eat it. PALM_COCONUT_EATEN kills the coconut while it is eaten.
 * return 1 when the command takes effect, else 0 */
int palm_coconut_tell(struct task *task, struct task *other, Sint8 command);
/* store in pos where lane's last coconut came to rest, or the origin before
 * one has, and return 1. return 0, storing nothing, for a null pos or a lane
 * outside 0 to 7 */
int palm_coconut_get_rest(NJS_POINT3 *pos, int lane);

#endif
