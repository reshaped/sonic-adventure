/* debris pieces' falling motion, and the chips they draw. Final Egg and Hot
 * Shelter each link a copy */
#ifndef STG10_DEBRIS_H
#define STG10_DEBRIS_H
#include <ninja.h>
#include <sg_xpt.h>

#include "system/task.h"

struct debris_body {
    Float drag; /* share of horizontal speed lost each step */
    Float bounce; /* share of vertical speed that debris_bounce keeps */
};

/* a piece's motion, in place of the task's struct task_motion */
struct debris_motion {
    struct debris_body *body;
    NJS_VECTOR speed;
    NJS_VECTOR accel; /* added to the speed and then cleared by debris_move */
    struct angles aim;
    struct angles spin; /* the turn about x and about z each step */
    Uint8 unk_34[4];
    Float ground; /* height of the ground the piece lands on */
    Float weight;
};

/* in work->flags: debris_land put the piece on its ground this step */
#define DEBRIS_LANDED 0x1

/* add a downward pull of 0.08 to the accel */
void debris_fall(struct task_work *work, struct debris_motion *motion);
/* add the accel to the speed. the speed's x and z also lose the body's drag
 * share of what they were. then move the piece by the speed, and clear the
 * accel */
void debris_move(struct task_work *work, struct debris_motion *motion);
/* lift a piece below its ground to 0.1 units above it. DEBRIS_LANDED is set
 * only then */
void debris_land(struct task_work *work, struct debris_motion *motion);
/* reverse the piece's vertical speed, scaled by its body's bounce */
void debris_bounce(struct task_work *work, struct debris_motion *motion);
/* add the speed's x and z, times a drag no path sets, to the accel's x and z.
 * those then hold undefined values */
void debris_drag(struct task_work *work, struct debris_motion *motion);
/* set the accel to force over the piece's weight */
void debris_push(struct debris_motion *motion, NJS_VECTOR *force);
/* pick a chip model, one of four, and a material of model's, each at random.
 * both go in work->vars. a null model leaves work->vars unchanged */
void debris_pick_chip(struct task_work *work, NJS_MODEL *model);
/* draw the chip debris_pick_chip picked, in the current matrix. without a
 * chip model and material, it draws nothing. the chip model's materials are
 * left pointing at the picked one */
void debris_draw_chip(struct task_work *work);
/* set the body of the task's debris motion. a null body sets the default,
 * with drag 0.02 and bounce 0.6 */
void debris_set_body(struct task *task, struct debris_body *body);
/* the body of the task's debris motion, or null for the default body */
struct debris_body *debris_get_body(struct task *task);
/* set the piece's weight, which debris_push divides by */
void debris_set_weight(struct debris_motion *motion, Float weight);

#endif
