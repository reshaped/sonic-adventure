/* Twinkle Park's scenery. O FENCE 1, O FENCE 2 and O PLANT 1 to O PLANT 6 are
 * solid models. O BOW WINDOW and O GIRDER only draw. O LAMP is a lamp post
 * with two glows. O CANDLE 1 and O CANDLE 2 are candle stands with flames.
 * O FLAG 1, O FLAG 2 and O FLAG W LAMP are flags that wave near player 0 */
#ifndef STG03_SCENERY_H
#define STG03_SCENERY_H

#include "system/task.h"

/* task steps for each object above, named in the object list. a fence or
 * plant turns by its set entry's ang.y and ang.z, and ignores ang.x and the
 * scale */
void twinkle_fence_1_step(struct task *task);
void twinkle_fence_2_step(struct task *task);
void twinkle_plant_1_step(struct task *task);
void twinkle_plant_2_step(struct task *task);
void twinkle_plant_3_step(struct task *task);
void twinkle_plant_4_step(struct task *task);
void twinkle_plant_5_step(struct task *task);
void twinkle_plant_6_step(struct task *task);
void twinkle_bow_window_step(struct task *task);
void twinkle_girder_step(struct task *task);
void twinkle_lamp_step(struct task *task);
/* the candle stands draw only in act 2, STAGE_TWINKLE_MIRRORS */
void twinkle_candle_1_step(struct task *task);
void twinkle_candle_2_step(struct task *task);
/* a flag waves while player 0 is within its set entry's scale y plus its own
 * distance. O FLAG 1 adds 300, O FLAG 2 400, and O FLAG W LAMP 200. its motion
 * advances scale x plus 0.4 frames each frame */
void twinkle_flag_1_step(struct task *task);
void twinkle_flag_2_step(struct task *task);
void twinkle_flag_lamp_step(struct task *task);

#endif
