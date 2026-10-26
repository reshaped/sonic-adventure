/* welds for the events: the seams of a model that an event poses itself,
 * closed as the model is posed */
#ifndef EVENT_WELD_H
#define EVENT_WELD_H
#include <ninja.h>
#include <sg_xpt.h>

#include "player/player.h"

/* run welds, a table ended by a null base, by mode. welds may be null only
 * for PLAYER_WELD_MAKE and PLAYER_WELDING.
 *
 * PLAYER_WELD_MAKE finds the nodes of each weld's two objects in base. it
 * copies both objects' points and normals at the shared vertices into a
 * block from the heap.
 *
 * PLAYER_WELDING poses action at frame, as far as node, counted from 0. node
 * must be below 128 and in action's tree. for a weld whose base is action's
 * object, WELD_BOTH and WELD_TO_A move b's vertices onto a's kept ones.
 * WELD_TO_B moves a's onto b's.
 *
 * any other mode, PLAYER_WELD_FREE among them, frees the blocks without
 * putting the vertices back */
void event_weld(struct weld *welds, NJS_ACTION *action, Float frame, int node,
                int mode);

#endif
