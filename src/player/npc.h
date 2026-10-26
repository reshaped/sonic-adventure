/* the npc: Tails as player 1, following player 0, and the title card */
#ifndef PLAYER_NPC_H
#define PLAYER_NPC_H
#include <ninja.h>
#include <sg_xpt.h>

struct task;

/* start the npc, and Tails as player 1, 10 behind leader. leader must not
 * be null. it returns the npc's task, new or already running, even when
 * Tails' own task can't be created. it returns null when the npc's task
 * can't be created, or where Tails may not follow Sonic. in Casinopolis,
 * STAGE_E101_R and STAGE_ALIFE_MR it starts the npc without asking whether
 * he may. a new npc clears npc_disabled */
struct task *npc_create(struct task *leader);
/* kill the npc's task, if it runs */
void npc_kill(void);
/* set the npc's mode. npc_follow puts Tails behind player 0 to follow it.
 * npc_wait keeps him out of sight until port B presses Start. npc_drop_in
 * drops him in from above the camera. each returns FALSE with no npc, and
 * TRUE otherwise */
Bool npc_follow(void);
Bool npc_wait(void);
Bool npc_drop_in(void);
/* kill player 1's task if it is Tails'. it returns TRUE once it has, or
 * with no npc, and FALSE when player 1 is missing or isn't Tails */
Bool npc_remove_tails(void);
/* bring Tails back as player 1. it returns his new task, or null with no
 * npc or when task_create fails */
struct task *npc_create_tails(void);
/* the y angle from one point to another. *distance gets the distance
 * between them, *across the distance in x and z; either can be null */
Angle npc_angle_to(NJS_POINT3 *from, NJS_POINT3 *to, Float *distance,
                   Float *across);
/* the stick that brakes against speed. while speed has x or z, *stroke
 * gets 1 and *ang the y angle opposite it. otherwise both get 0. either can
 * be null */
void npc_stick_against(NJS_VECTOR *speed, Float *stroke, Angle *ang);
/* TRUE when one task's work has PLAYER_GUARD and COLLISION_TOUCHING and
 * its collision hit the other task, whose work has PLAYER_GUARD too. while
 * task's work has both flags, only task's own hit is tested. neither may
 * be null */
Bool npc_players_clash(struct task *task, struct task *other);
/* set npc_disabled, which keeps Tails out of sight while it is TRUE */
void npc_set_disabled(Bool disabled);

/* stop the streams, black the screen and load the stage's title card, to
 * show for frames, or 180 if frames is 0. frames must be under 255, or the
 * card never ends. it returns 0 once the card is loaded, and -1 when the
 * stage has no card or the card won't load. the streams stop and the
 * screen blacks either way */
Sint16 title_card_begin(int frames);
/* draw the card. once its frames are up and card_saving is 0, it releases
 * the card, sets draw_off and returns 1. until then it returns 0 */
Sint16 title_card_step(void);

#endif
