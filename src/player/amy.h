/* Amy as a player: what she's doing and carrying, her hammer, and the calls
 * other tasks make to her */
#ifndef PLAYER_AMY_H
#define PLAYER_AMY_H
#include <sg_xpt.h>

struct task;

/* what amy_get_action gives: what Amy is doing, or 0 */
enum amy_action {
    AMY_TAKING_DRUM = 1,
    AMY_SETTING_DRUM,
    AMY_RECOILING, /* from a hit */
    AMY_HOLDING_HANDLE,
    AMY_SPINNING, /* round, with her hammer */
    AMY_BALANCING,
    AMY_HAMMER_JUMPING, /* leaping off her hammer */
    AMY_AIR_HAMMERING,
    AMY_DIZZY /* once the spin is over */
};

/* the step of a player's task when the player is Amy */
void amy_step(struct task *task);
/* TRUE while the player is Amy with a drum, from when she finishes taking
 * it until she starts to set it down. FALSE otherwise */
Bool amy_carries_drum(int player);
/* what the player is doing, as an amy_action, or 0. it reads the player's
 * own mode but player 0's record, whichever player is asked, so AMY_DIZZY
 * comes from player 0's action. 0 when the player has no task or isn't Amy,
 * when player 0 has no record, and when no amy_action names her action or
 * mode */
int amy_get_action(int player);
/* amy_set_hammer_scale shows the player's hammer at scale at once, and
 * amy_grow_hammer eases it to scale. each holds it there until either is
 * called with a scale of 0 or less. swinging, spinning or leaping off her
 * hammer still changes the scale. the player must be Amy: each writes Amy's
 * fields into whatever character's record the player has. FALSE for a
 * player with no record, else TRUE */
Bool amy_set_hammer_scale(Uint8 player, Float scale);
Bool amy_grow_hammer(Uint8 player, Float scale);
/* call player 0 to the handle for this step: at a press of the action
 * button she takes hold, and on a step without the call she lets go.
 * player 0 must be Amy: it sets Amy's call in whatever character's record
 * player 0 has. FALSE with no record, else TRUE */
Bool amy_call_to_handle(void);
/* tell player 0 she's hidden for this step, as Zero's trigger boxes do
 * where it loses sight of her. her step clears the call unread. player 0
 * must be Amy: it sets Amy's call in whatever character's record player 0
 * has. FALSE with no record, else TRUE */
Bool amy_call_hidden(void);

#endif
