/* the O TELEPOT teleporter, and the props the object list names */
#ifndef STG09_PROPS_H
#define STG09_PROPS_H
#include <sg_xpt.h>

#include "system/task.h"

/* O TELEPOT's step in the object list. the teleporter's cylinder rises 40
 * from its position, |scl.y + 10| in radius. any collision record's touch of
 * it starts a pull of player 0. the pull moves him at 2 a step toward the
 * point 10 below the teleporter. within the square root of 10 of that point
 * in x and z, the pull becomes a hold. a step without a touch ends either
 * one. player 0 must exist through both, and neither lets him steer.
 *
 * each step of a hold stops player 0, spins him and shakes the pack in pad
 * 2's sockets. in the act the teleporter started in, a hold brightens the
 * offset colors of the players' palette. a hold sets them to 0xff000000 as
 * it ends. destroying a teleporter that holds in that act sets them to 0. on
 * a hold's 100th step, casino_request becomes scl.x, truncated */
void telepot_step(struct task *task);
/* TRUE from when a teleporter takes hold while no other holds, until that
 * one lets go or is destroyed */
Bool telepot_holding(void);

/* the props' steps in the object list. each starts its prop with one of
 * helpers.h's starts. the start says what the prop draws and how it
 * collides.
 * O FLOWERAL and O FLOWERBL draw O FLOWERA's and O FLOWERB's models unlit */
void light_a_step(struct task *task);
void light_b_step(struct task *task);
void fk_a_step(struct task *task);
void fk_b_step(struct task *task);
void slotisu_step(struct task *task);
void dento_step(struct task *task);
void flower_a_step(struct task *task);
void flower_b_step(struct task *task);
void flower_al_step(struct task *task);
void flower_bl_step(struct task *task);
void saku_step(struct task *task);
void taru_step(struct task *task);
void hako_step(struct task *task);
void gbox_step(struct task *task);
void gomi_b_step(struct task *task);
void gomi_r_step(struct task *task);
void rlt_step(struct task *task);
void yukh_step(struct task *task);
void rlti_step(struct task *task);
void kb_b_step(struct task *task);
void kb_r_step(struct task *task);
void kb_1_step(struct task *task);
void kb_2_step(struct task *task);
void kb_3_step(struct task *task);
void kb_4_step(struct task *task);
void tdenki_step(struct task *task);
void sdfuti_r_step(struct task *task);
void sdfuti_b_step(struct task *task);
void map2f_step(struct task *task);
void cfo_step(struct task *task);
void mizu_a_step(struct task *task);
void mizu_b_step(struct task *task);
void sy_a_step(struct task *task);
void sy_b_step(struct task *task);
void shwrl_step(struct task *task);
void shwhd_step(struct task *task);
void neokab_step(struct task *task);

#endif
