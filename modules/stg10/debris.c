/* debris pieces' falling motion, and the chips they draw. Final Egg and Hot
 * Shelter each link a copy */
#include <ninja.h>
#include <sg_xpt.h>
#include <stdlib.h>

#include "draw/model.h"
#include "stg10/debris.h"
#include "system/task.h"
#include "system/util.h"

extern NJS_MODEL debris_chip_0_model;
extern NJS_MODEL debris_chip_1_model;
extern NJS_MODEL debris_chip_2_model;
extern NJS_MODEL debris_chip_3_model;

#define CHIPS 4

/* what debris_pick_chip keeps in work->vars */
struct debris_chip {
    NJS_MATERIAL *material;
    Uint8 unk_04[4];
    NJS_MODEL *model;
};

#define CHIP(work) ((struct debris_chip *)&(work)->vars)

static NJS_MODEL *debris_chips[CHIPS] = {
    &debris_chip_0_model,
    &debris_chip_1_model,
    &debris_chip_2_model,
    &debris_chip_3_model,
};

/* the body debris_set_body gives a piece without one */
static struct debris_body debris_default_body = {0.02F, 0.6F};

void debris_pick_chip(struct task_work *work, NJS_MODEL *model)
{
    int i;

    if (model != NULL) {
        i = rand() * (CHIPS * RAND_SCALE);
        CHIP(work)->model = debris_chips[i];
        i = (Float)rand() * model->nbMat * RAND_SCALE;
        CHIP(work)->material = &model->mats[i];
    }
}

void debris_draw_chip(struct task_work *work)
{
    if (CHIP(work)->model != NULL && CHIP(work)->material != NULL) {
        NJS_MODEL *model = CHIP(work)->model;

        model->mats = CHIP(work)->material;
        draw_model(model);
    }
}

void debris_fall(struct task_work *work, struct debris_motion *motion)
{
    motion->accel.y += -0.08F;
}

void debris_drag(struct task_work *work, struct debris_motion *motion)
{
    Float drag;
    Float *speed;
    Float *accel;
    Float x;
    Float z;
    Float ax;
    Float ay;
    Float az;

    /* @hack spelling: cursors over speed's and accel's floats, for the image's
     * post-increment loads and pre-decrement stores (0x0c909bfa to 0x0c909c12
     * in STG10); through the members, 18 of debris_drag's 19 words differ */
    speed = &motion->speed.x;
    x = *speed++;
    speed++;
    z = *speed;
    accel = &motion->accel.x;
    ax = *accel++;
    ay = *accel++;
    az = *accel++;
    /* @bug uninitialized: no path sets drag, and the image reads it from the
     * FR15 the caller left (FMOV FR15,FR0 at 0x0c909bf6 in STG10). no code
     * calls debris_drag, so the read never runs */
    /* NOLINTNEXTLINE(clang-diagnostic-uninitialized) */
    ax += drag * x;
    az += drag * z;
    *--accel = az;
    *--accel = ay;
    *--accel = ax;
}

void debris_move(struct task_work *work, struct debris_motion *motion)
{
    struct debris_body *body = motion->body;
    Float *accel = &motion->accel.x;
    Float *speed = &motion->speed.x;
    Float *pos = &work->pos.x;
    Float drag;
    Float x;
    Float y;
    Float z;
    Float ax;
    Float ay;
    Float az;
    Float px;
    Float py;
    Float pz;

    /* @hack spelling: cursors over speed's, accel's and pos's floats, for the
     * image's post-increment loads and pre-decrement stores (0x0c909c20 to
     * 0x0c909c5e in STG10); through the members, 36 of debris_move's 36 words
     * differ */
    x = *speed++;
    ax = *accel++;
    ay = *accel++;
    y = *speed++;
    z = *speed++;
    az = *accel++;
    drag = -body->drag;
    /* @hack redundant: a self-assignment of drag that never runs, which keeps
     * drag in FR6 and x in FR7 (0x0c909c26, 0x0c909c28 in STG10); without it,
     * the two trade and 14 of 36 words differ */
    if (0) {
        drag = drag; /* NOLINT(clang-diagnostic-self-assign) */
    }
    x += ax + (drag * x);
    y += ay;
    z += az + (drag * z);
    *--accel = 0.0F;
    *--accel = 0.0F;
    *--accel = 0.0F;
    *--speed = z;
    *--speed = y;
    *--speed = x;
    px = *pos++;
    py = *pos++;
    pz = *pos++;
    px += x;
    py += y;
    pz += z;
    *--pos = pz;
    *--pos = py;
    *--pos = px;
}

void debris_bounce(struct task_work *work, struct debris_motion *motion)
{
    struct debris_body *body = motion->body;

    motion->speed.y *= -body->bounce;
}

void debris_push(struct debris_motion *motion, NJS_VECTOR *force)
{
    Float *accel;
    Float *from = &force->x;
    Float x = *from++;
    Float y = *from++;
    Float z = *from;
    Float scale = 1.0F / motion->weight;

    /* @hack spelling: a cursor from the end of motion->accel, for the image's
     * pre-decrement stores (0x0c909c8a to 0x0c909c90 in STG10); through the
     * members, 9 of debris_push's 15 words differ */
    accel = VECTOR_FLOATS(&motion->accel) + 3;
    x *= scale;
    y *= scale;
    z *= scale;
    *--accel = z;
    *--accel = y;
    *--accel = x;
}

void debris_land(struct task_work *work, struct debris_motion *motion)
{
    work->flags &= ~DEBRIS_LANDED;
    if (work->pos.y < motion->ground) {
        work->pos.y = motion->ground + 0.1F;
        work->flags |= DEBRIS_LANDED;
    }
}

void debris_set_weight(struct debris_motion *motion, Float weight)
{
    motion->weight = weight;
}

void debris_set_body(struct task *task, struct debris_body *body)
{
    struct debris_motion *motion = (struct debris_motion *)task->motion;

    if (body == NULL) {
        motion->body = &debris_default_body;
    } else {
        motion->body = body;
    }
}

struct debris_body *debris_get_body(struct task *task)
{
    struct debris_body *body = ((struct debris_motion *)task->motion)->body;

    if (body == &debris_default_body) {
        return NULL;
    }
    return body;
}
