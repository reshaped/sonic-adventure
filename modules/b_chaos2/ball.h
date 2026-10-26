/* Chaos 2 as a ball of water. his figure rounds into the ball and back,
 * and the ball leaps at player 0. it also bursts into drops, which crawl
 * back to him while a core hovers */
#ifndef B_CHAOS2_BALL_H
#define B_CHAOS2_BALL_H
#include <ninja.h>
#include <sg_xpt.h>

#include "chaos/chaos.h"
#include "system/task.h"

/* the model chaos2_round turns from his figure into the ball and back */
/* @hack type: an array of one, whose address chaos2_round loads once, into R11,
 * for all four of its uses (MOV.L at 0x0c903f7e). as a lone NJS_OBJECT, summed
 * over chaos2_round and boss.c's chaos2_draw, 87 of 653 words differ */
extern NJS_OBJECT chaos2_round_object[1];
/* a drop's model while it is in the air. a drop that has landed draws as
 * chaos_blob */
extern NJS_OBJECT chaos2_drop_object;

/* the step of BOSS_TO_BALL and BOSS_FROM_BALL. it asks record's motion for
 * ACTION_STAND, and the rounding starts once that action plays. the
 * rounding takes 90 steps. BOSS_TO_BALL turns his figure into the ball. it
 * hides his body, his brains and the ribbon's beads, and sends emeralds 0
 * and 1 out to circle him. BOSS_FROM_BALL turns the ball back into his
 * figure. as it ends, it shows what BOSS_TO_BALL hid and calls the emeralds
 * home. either mode then asks for boss_return_mode */
void chaos2_round(struct task_work *work, struct chaos *chaos,
                  struct chaos *record);
/* the step of BOSS_BOUNCE. the ball leaps 20 high at player 0 from each
 * landing, as far as player 0 lies but at most 50. while chaos has
 * CHAOS2_STRUCK, each leap goes 50 across, within 40 degrees of the way to
 * player 0. the leaps end once 600 steps have passed, or at the hall's
 * edge. a hit in the air ends them too, and knocks the ball away from
 * player 0. the ball then bounces to rest. at rest, it destroys every child
 * of task and clears CHAOS2_STRUCK. it also sets the ball's y to 0 and
 * turns it to face player 0. it then asks for BOSS_FROM_BALL, and
 * BOSS_CHASE after that */
void chaos2_bounce(struct task *task, struct task_motion *motion,
                   struct chaos *chaos);
/* the step of BOSS_BURST. the ball bursts into 24 drops, a core, a flash
 * and a light, all children of task. a blight starts at once and every 61
 * steps after, up to five. his puddle grows as the drops crawl back to him.
 * once all are back, it destroys every child of task. it then asks for
 * BOSS_FORM, and BOSS_LEAP after that. the drops and their records go
 * unchecked, as do the core, its brains and the flash. any of them the pool
 * or the heap can't give is used through null. record is unread */
void chaos2_burst(struct task *task, struct task_work *work,
                  struct chaos *chaos, struct chaos *record);
/* draw a shadow under pos, lying flat at y 0.2. its sprite takes a scale
 * of size * 0.12 for pos at y 0, which shrinks as pos rises, to nothing at
 * y 50. a shadow it draws leaves Ninja's blending and constant material as
 * it set them */
void chaos2_shadow_draw(const NJS_POINT3 *pos, Float size);

#endif
