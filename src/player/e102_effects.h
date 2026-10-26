/* E-102's booster, sparks and dust, and helpers his step and other files
 * call */
#ifndef PLAYER_E102_EFFECTS_H
#define PLAYER_E102_EFFECTS_H
#include <ninja.h>
#include <sg_xpt.h>

struct e102;
struct player;
struct task;
struct task_motion;
struct task_work;

/* @hack type: the booster's flame as an array, whose one pool word, its own
 * address, both reaches its model (MOV.L @(4,R2),R3 at 0x8c0d8c78) and goes to
 * draw_tree (MOV R2,R4 at 0x8c0d8c88). as one object, it takes a second pool
 * word for its model's address, and summed over e102_draw and b_e101's draw,
 * e103_run and e104_draw, 128 of 1925 words differ */
extern NJS_OBJECT e102_flame[];

/* bob his booster, quicker at a higher speed, and tilt it with his
 * speed.x, up to 25 degrees either way */
void e102_bob_booster(struct task_work *work, struct player *record,
                      struct e102 *e102);
/* the world flags of the floor nearest work's height among world_objects,
 * or 0 for none. a task's object is tested only with work within its reach
 * across x and z */
Uint32 e102_floor_flags(struct task_work *work);
/* a spark from each of his feet, if he's on the ground */
void e102_skate_sparks(struct task_work *work, struct task_motion *motion,
                       struct player *record);
/* a puff of dust on the ground below him, whose height is ground, if he's
 * less than 30 above it */
void e102_hover_dust(struct task_work *work, Float ground);
/* a ring of dust round him, bigger and faster the faster he falls */
void e102_landing_dust(struct task_work *work, struct task_motion *motion);
/* push player's stick onto e102->sticks, dropping the oldest. with lag,
 * pad_sticks[player] takes the angle and stroke of the oldest kept, until
 * e102_restore_stick puts back the newest */
void e102_record_stick(struct e102 *e102, int player, Bool lag);
void e102_restore_stick(struct e102 *e102, int player);
/* task is E-102's player task. TRUE while he starts or holds an aim */
Bool e102_is_aiming(struct task *task);
/* put player, who must be E-102, into E102_SKATE_START, at the speed he
 * has */
void e102_set_skating(int player);

#endif
