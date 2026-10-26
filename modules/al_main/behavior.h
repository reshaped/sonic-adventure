/* a Chao's modes, which pick the behavior it runs, and the calls that run
 * and change them */
#ifndef AL_MAIN_BEHAVIOR_H
#define AL_MAIN_BEHAVIOR_H
#include <sg_xpt.h>

#include "al_main/chao.h"
#include "system/task.h"

/* a Chao's work->mode: the index of the behavior alife_behave runs, from
 * 0 to 42 */
#define MODE_IDLE 0
#define MODE_GO 1 /* to the Chao's target */
#define MODE_NEWBORN 2
#define MODE_SLEEP 3
#define MODE_HURT 4
#define MODE_TAKE_ANIMAL 5
#define MODE_HOLD 6
#define MODE_GROW_UP 7
#define MODE_LIFE_OVER 8
#define MODE_GO_TO_PLAYER 9
#define MODE_HUNGRY 14
#define MODE_SLEEPY 23
#define MODE_TRUMPET 25
#define MODE_DRUM 27
#define MODE_SING 28
#define MODE_GO_2 30 /* as MODE_GO */
#define MODE_COURT 32 /* calling for a partner */
#define MODE_PARTNER 33
#define MODE_BREED 34
#define MODE_JOIN_COURT 35 /* making for a Chao that calls */
#define MODE_MARK_FAVORITE 38
/* an idle Chao's when it neither plays nor goes to a place */
#define MODE_NO_PLAY 40

/* set CHAO_NEAR_PLAYER in the Chao's player_flags while player 0 is within
 * 100 of it, and clear it otherwise. then run the behavior work->mode
 * picks, and alife_apply_speed, which moves the Chao by its speed. work->mode
 * must be 0 to 42 */
void alife_behave(struct task_work *work, struct alife_chao *chao);
/* try fun_0c915b00, alife_take_hit, alife_try_take_animal and alife_try_hold
 * in turn, each of which may take the Chao from its behavior. returns TRUE at
 * the first that returns TRUE, and FALSE when none does. in MODE_GROW_UP and
 * MODE_LIFE_OVER it tries none, and returns FALSE. during its cooldown
 * alife_take_hit returns no value. the 122 its image leaves in R0 then reads
 * as TRUE, so the tries stop there and this returns TRUE */
Bool alife_interrupt(struct task_work *work, struct alife_chao *chao);
/* send the Chao in MODE_GO to one of the four places nearest it, picked at
 * random. rand() scaled to 3.99 gives the fourth a share of 0.99 to each
 * other's 1. its target lies on the circle of the place's radius about the
 * place, in a random direction. a Chao whose hunger is above 150 then takes
 * each tree or fruit among the four at 4 in 10. the last it takes wins, and
 * each take sets the Chao's icon to ICON_GLAD_1, the glad symbol. with fewer
 * than four places in the garden, the pick can find an empty slot, and a Chao
 * left with none goes back to MODE_IDLE. in such a garden, a hungry Chao also
 * reads the kind through each empty slot's null entry */
void alife_go_to_place(struct task_work *work, struct alife_chao *chao);

#endif
