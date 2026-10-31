/* Super Sonic's aura, wake, spin balls and dash in the Perfect Chaos fight */
#ifndef B_CHAOS7_SUPER_H
#define B_CHAOS7_SUPER_H
#include <ninja.h>

#include "system/task.h"

/* Super Sonic's textures, which chaos7_act_pvms loads from the SUPERSONIC pvm.
 * his body's 16 come first, then the aura's, the wake's, the spin balls' and
 * the dash's. sonic.c draws him with them during his super actions */
extern NJS_TEXLIST super_sonic_texlist;

/* Super Sonic's model, which sonic.c draws, and the nodes of it that
 * 1ST_READ.BIN's sonic_welds joins, two at each seam */
extern NJS_OBJECT super_sonic_object;
extern NJS_OBJECT dat_0c902334;
extern NJS_OBJECT dat_0c9024bc;
extern NJS_OBJECT dat_0c902838;
extern NJS_OBJECT dat_0c902e34;
extern NJS_OBJECT dat_0c903278;
extern NJS_OBJECT dat_0c903504;
extern NJS_OBJECT dat_0c90368c;
extern NJS_OBJECT dat_0c9039fc;
extern NJS_OBJECT dat_0c903fec;
extern NJS_OBJECT dat_0c904430;
extern NJS_OBJECT dat_0c9046e4;
extern NJS_OBJECT dat_0c90486c;
extern NJS_OBJECT dat_0c904bac;
extern NJS_OBJECT dat_0c905020;
extern NJS_OBJECT dat_0c905464;
extern NJS_OBJECT dat_0c905708;
extern NJS_OBJECT dat_0c905890;
extern NJS_OBJECT dat_0c905bd0;
extern NJS_OBJECT dat_0c906044;
extern NJS_OBJECT dat_0c90647c;

/* Super Sonic's actions, which 1ST_READ.BIN's sonic_actions names in rows
 * ACTION_SUPER_STAND to ACTION_SUPER_BALL. a comment gives an action's rows
 * where its name doesn't give them all */
extern NJS_ACTION super_sonic_walk_action;
extern NJS_ACTION super_sonic_trot_action;
/* ACTION_SUPER_RUN, ACTION_SUPER_SPRINT */
extern NJS_ACTION super_sonic_run_action;
extern NJS_ACTION super_sonic_stand_action;
extern NJS_ACTION super_sonic_ball_action;
extern NJS_ACTION super_sonic_spring_action;
extern NJS_ACTION super_sonic_fall_start_action;
extern NJS_ACTION super_sonic_fall_action;
extern NJS_ACTION super_sonic_hard_landing_action; /* row 138 */
extern NJS_ACTION super_sonic_clear_action;
extern NJS_ACTION dat_0c914dc0; /* row 140 */

/* the motions of Sonic's spring, fall start, fall, hard landing and ball
 * actions, which 1ST_READ.BIN holds. actions in the generated data play
 * them: the first four on super_sonic_object, the ball's on dat_0c90bd80 */
extern NJS_MOTION sonic_spring_motion;
extern NJS_MOTION sonic_fall_start_motion;
extern NJS_MOTION sonic_fall_motion;
extern NJS_MOTION sonic_hard_landing_motion;
extern NJS_MOTION sonic_ball_motion;

/* the first steps of the tasks sonic.c creates for Super Sonic, each with the
 * player's number in PLAYER_VARS(work)->player. the balls and the dash take
 * the place of effects.h's ball_follow_start, ball_shrink_start and
 * dash_start */
void super_aura_start(struct task *task); /* his aura, and his wake */
void super_ball_follow_start(struct task *task); /* a step behind the player */
void super_ball_shrink_start(struct task *task); /* left where the player was */
/* the dash is a streak from where the player's middle was to where it is.
 * without room on the heap for its copy of the streak, it ends at once */
void super_dash_start(struct task *task);
/* Super Sonic's ball: the object of super_sonic_ball_action */
extern NJS_OBJECT dat_0c90bd80;

#endif
