/* Casinopolis's pinball state, ring payouts, table countdowns and StartShot,
 * the launcher. the first table is act 2, and the second act 3 */
#ifndef STG09_PINBALL_H
#define STG09_PINBALL_H
#include <ninja.h>
#include <sg_xpt.h>

#include "system/task.h"

/* values of casino_entrance, which picks where act 0 starts player 0: at the
 * stage's start, from act 1, or from a table left with 100 rings or more */
#define CASINO_FROM_START 0xff
#define CASINO_FROM_ACT1 0
#define CASINO_FROM_TABLE 1

/* the balls a table starts player 0 with */
#define PINBALL_BALLS 3
/* the cards in the second table's hand */
#define PINBALL_CARDS 5
/* the places where the first table's second countdown puts out rings */
#define PINBALL_RING_PLACES 16
/* the second table's idea pieces, and the draws of Card's idea face that
 * earn the next piece */
#define PINBALL_IDEAS 5
#define PINBALL_IDEA_DRAWS 3

/* CC3 is the tables' camera. set in pinball_view with a view, this bit has
 * CC3 ease the camera to that view. CC3 clears it only once its six arrival
 * tests pass. a shipped bug keeps them from passing near the view */
#define PINBALL_VIEW_MOVING 0x80
/* CC3's views: the table's, which play goes back to, Roll's, each ramp's top
 * as player 0 reaches it, the three the drop targets take in turn, and each
 * Jdai's as its targets' bonus starts */
enum {
    PINBALL_VIEW_TABLE,
    PINBALL_VIEW_ROLL,
    PINBALL_VIEW_RIGHT_RAMP,
    PINBALL_VIEW_LEFT_RAMP,
    PINBALL_VIEW_TARGETS_1,
    PINBALL_VIEW_TARGETS_2,
    PINBALL_VIEW_TARGETS_3,
    PINBALL_VIEW_RIGHT_BONUS,
    PINBALL_VIEW_LEFT_BONUS
};

/* casino_flags bits. StoperC stands up while CASINO_STOPPER_C_UP is set. Roll
 * sets it with the first table's fourth countdown, which clears it as it
 * ends */
#define CASINO_STOPPER_C_UP 0x1
/* StoperL and StoperR stand up while their bits are set. JdaiL and JdaiR set
 * them. in act 3, the drop targets also set CASINO_STOPPER_L_UP once their
 * pieces are all down. the lanes, also in act 3, set CASINO_STOPPER_R_UP once
 * their lamps are all lit */
#define CASINO_STOPPER_L_UP 0x2
#define CASINO_STOPPER_R_UP 0x4
/* set while each drop target piece stands. AkaAoMido's lamps show the three
 * bits */
#define CASINO_DROP_TARGET_2_UP 0x8
#define CASINO_DROP_TARGET_1_UP 0x10
#define CASINO_DROP_TARGET_0_UP 0x20
/* set while ShoogaiL and ShoogaiR are down. a touch lowers each for 1500
 * steps. KuroKabe clears both bits as it starts. it draws one of its walls only
 * while CASINO_SHOOGAI_L_DOWN is clear, and another only while
 * CASINO_SHOOGAI_R_DOWN is */
#define CASINO_SHOOGAI_L_DOWN 0x40
#define CASINO_SHOOGAI_R_DOWN 0x80
/* StartShot clears CASINO_UNK_100 as it fires. each touch of StoperL's or
 * StoperR's switch toggles it. a touch that clears it has CC3 ease the camera
 * to view 0 */
#define CASINO_UNK_100 0x100
/* set while player 0 carries the next idea piece. once the piece is earned,
 * a touch of Path's third switch sets it. past 3600 along Path's second ride,
 * the piece is placed and the bit cleared. the stage task clears it when the
 * second table's last ball is lost */
#define CASINO_IDEA_CARRIED 0x2000
/* the first table's first countdown hides each of its two markers while that
 * marker's bit is set */
#define CASINO_COUNTDOWN0_HIDE_A 0x200
#define CASINO_COUNTDOWN0_HIDE_B 0x400
/* HoudaiL and HoudaiR each fire once their bit is set, and clear the bit */
#define CASINO_HOUDAI_L_FIRES 0x800
#define CASINO_HOUDAI_R_FIRES 0x1000
/* each of the second table's countdowns sets it as it ends. Card sets it
 * for a hand of one or two pairs. BanjuDoor sinks while it is set, and clears
 * it as it starts. Card clears it as a hand starts a countdown, and so does the
 * end of Path's second ride */
#define CASINO_BANJU_DOOR_OPEN 0x4000
/* each countdown's bit is set while it runs. ACT2 names the first table's
 * six, and ACT3 the second's four */
#define CASINO_COUNTDOWN_ACT3_0 0x10000
#define CASINO_COUNTDOWN_ACT3_1 0x20000
#define CASINO_COUNTDOWN_ACT3_2 0x40000
#define CASINO_COUNTDOWN_ACT3_3 0x80000
#define CASINO_COUNTDOWN_ACT2_0 0x100000
#define CASINO_COUNTDOWN_ACT2_1 0x200000
#define CASINO_COUNTDOWN_ACT2_2 0x400000
#define CASINO_COUNTDOWN_ACT2_3 0x800000
#define CASINO_COUNTDOWN_ACT2_4 0x1000000
#define CASINO_COUNTDOWN_ACT2_5 0x2000000
/* no countdown counts while this bit is set */
#define CASINO_COUNTDOWNS_HELD 0x8000000
/* pinball_history_record records only while this bit is set */
#define CASINO_HISTORY_RECORDING 0x4000000
/* set while the stage task has OBJ_CASINO's, OBJ_CASINO2's, OBJ_CASINO9's or
 * OBJ_CASINO8's textures loaded, a bit each. it loads the last two for acts
 * 0 and 1, and the first two for the tables */
#define CASINO_HAS_CASINO 0x10000000
#define CASINO_HAS_CASINO2 0x20000000
#define CASINO_HAS_CASINO9 0x40000000
#define CASINO_HAS_CASINO8 0x80000000

extern Uint8 casino_entrance;
/* the drop targets' bonuses paid while the first table's fifth countdown or
 * the second table's third runs. each of those countdowns starts it at 0 */
extern Sint8 pinball_target_bonuses;
/* where the first table's third countdown stands: 0 to 2, or the fourth
 * place for any higher value. touches of the table's targets in turn move it
 * on */
extern Uint8 pinball_countdown2_place;
/* the second table's first countdown starts it at 1. each touch of RightLane
 * while that countdown runs adds 1 */
extern Sint8 pinball_right_lane_touches;
/* the second table's second countdown starts it at 0. each touch of Path's
 * fourth switch while that countdown runs adds 1 */
extern Sint8 pinball_ride_touches;
/* the second table's hand, and the count of its cards dealt. the count goes
 * back to 0 at the fifth */
extern Uint8 pinball_cards[PINBALL_CARDS];
extern Uint8 pinball_cards_dealt;
/* balls left on the current table */
extern Uint8 pinball_balls;
/* Card's draws of the idea face since the last idea piece was placed, held
 * at PINBALL_IDEA_DRAWS */
extern Uint8 pinball_idea_draws;
/* idea pieces in place, 1 to PINBALL_IDEAS, the first in place from the
 * start */
extern Uint8 pinball_ideas;
/* CC3's view, a PINBALL_VIEW_ number, with PINBALL_VIEW_MOVING while the
 * camera eases to it */
extern Uint8 pinball_view;
extern Uint32 casino_flags;
extern NJS_POINT3 pinball_ring_places[PINBALL_RING_PLACES];
/* a sprite that the cards and Roll's prize draw. CardDisp sets sy and p.y as
 * it starts, and the prize sets all four as it starts. each draw by the
 * cards sets sx and p.x first */
extern NJS_SPRITE card_white_sprite;

/* set player 0's speed, place, angles and aim from an entry of the history,
 * and clear CASINO_HISTORY_RECORDING. nothing in this image calls
 * pinball_history_record, so every entry holds zeros */
void pinball_history_restore(void);
/* while CASINO_HISTORY_RECORDING is set, record player 0's speed, place,
 * angles and aim. each call takes the next of the history's 30 entries,
 * round and round */
void pinball_history_record(void);
/* give player 0 count rings, each with SE_RING. the first comes on the ring
 * task's 6th step, and the rest 7 steps apart */
void pinball_give_rings(Uint8 count);
/* take count rings from player 0, one a step, stopping early once he has
 * none */
void pinball_take_rings(Uint8 count);
/* speed, capped at 8, the most a bumper or a sling launches player 0 at */
Float pinball_cap_speed(Float speed);
/* the size of player 0's forward speed, speed.x */
Float pinball_player_speed(void);
/* the steps of the tables' countdowns: six on the first table, and four on
 * the second. the task's creator sets its work->timer to the steps it runs,
 * and sets its bit. each step counts the timer down while
 * CASINO_COUNTDOWNS_HELD is clear, and draws the seconds left. a countdown
 * ends once the timer reaches 0 or its act ends, and clears its bit. the
 * first table's fourth also clears CASINO_STOPPER_C_UP, and the second
 * table's set CASINO_BANJU_DOOR_OPEN. whenever none of its rings are
 * left, the first table's second puts out rings at the ring places. it then
 * fires both Houdai and gives 20 rings */
void pinball_act2_countdown0_step(struct task *task);
void pinball_act2_countdown1_step(struct task *task);
void pinball_act2_countdown2_step(struct task *task);
void pinball_act2_countdown3_step(struct task *task);
void pinball_act2_countdown4_step(struct task *task);
void pinball_act2_countdown5_step(struct task *task);
void pinball_act3_countdown0_step(struct task *task);
void pinball_act3_countdown1_step(struct task *task);
void pinball_act3_countdown2_step(struct task *task);
void pinball_act3_countdown3_step(struct task *task);
/* StartShot's step in the object list. a touch sets player 0's angles to 0,
 * 0x8000 and 0. it sends him flying ahead at scl.z plus up to scl.y more at
 * random, and clears CASINO_UNK_100 */
void start_shot_step(struct task *task);

#endif
