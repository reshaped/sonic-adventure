/* put a player back where the stage places it, or hold it still with its
 * pad off */
#ifndef PLAYER_RESET_H
#define PLAYER_RESET_H
#include <sg_xpt.h>

/* place the player with stage_place_player, at the first place it lists:
 * the act's restart point, through to the character's start. for a player
 * with no work, it does nothing */
void player_place_start(Sint8 player);
/* hold the player still: its pad stops driving it, and its speed and its
 * motion's go to 0 */
void player_hold(Sint8 player);

#endif
