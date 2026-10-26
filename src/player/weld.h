/* a player's model posed by a motion the caller gives, with the player's
 * welds closing its seams. events pose players so, in place of their own
 * actions */
#ifndef PLAYER_WELD_H
#define PLAYER_WELD_H
#include <ninja.h>
#include <sg_xpt.h>

/* pose object, the player's model, as motion has it at frame. the pose takes as
 * many nodes as the player's first action has. then apply the player's welds
 * whose base is object. the player's weld_mode must be PLAYER_WELDING, which
 * player_update_welds sets once it has made the welds. with any other mode,
 * player_weld loops forever */
void player_weld(int player, NJS_OBJECT *object, NJS_MOTION *motion,
                 Float frame);

#endif
