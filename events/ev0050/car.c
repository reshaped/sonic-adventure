/* EV0050.PRS's parked cars: ADV00.PRS's parked car, drawn with its lights
 * on */
#include <ninja.h>
#include <sg_xpt.h>

#include "adv00/texlists.h"
#include "collision/collision.h"
#include "collision/ground.h"
#include "draw/tree.h"
#include "ev0050/car.h"
#include "event/event.h"
#include "system/task.h"
#include "system/util.h"

#define CAR_LIGHTS_MATERIAL 2 /* the model's material for the lights */
#define CAR_LIGHTS_ON_TEXTURE 12 /* the lights lit, in sscar_texlist */

struct car_vars {
    Uint8 unk_00[8];
    /* the lights' texture when the car was created, which the draw and the
     * destructor put back */
    Uint32 lights_texture;
};

#define VARS(work) ((struct car_vars *)&(work)->vars)

/* in ADV00.PRS */
extern NJS_OBJECT parked_car_object;

/* a sphere of radius 0. the step registers it only once event_set_radius
 * gives it a radius */
/* clang-format off */
static struct collision_shape ev0050_car_collision[1] = {
    {SHAPE_SPHERE, 0, PUSH_FIRM(PUSH_ALL) | PUSH_PLAYERS, {0.0F, 0.0F, 0.0F},
     0.0F, 0.0F, 0.0F, {0, 0, 0}, 0, NULL},
};
/* clang-format on */

/* put the lights' texture back: the model is ADV00.PRS's, not the car's */
static void ev0050_car_destroy(struct task *task)
{
    struct task_work *work = task->work;

    /* @hack spelling: the model as (&parked_car_object)->model. SHC then loads
     * the object's address and reads the model 4 bytes past it, as the image
     * does (MOV.L @(4,R3),R2 at 0x0cb83838). as parked_car_object.model, SHC
     * loads the field's own address from the pool, and 5 of 8 words differ */
    (&parked_car_object)->model->mats[CAR_LIGHTS_MATERIAL].attr_texId =
        VARS(work)->lights_texture;
}

static void ev0050_car_draw(struct task *task)
{
    /* @hack redundant: register, which gives work R14 and car R13, as the
     * image does (0x0cb8384e, 0x0cb8384c). without it, the two trade, and 8
     * of 47 words differ */
    register struct task_work *work = task->work;
    NJS_OBJECT *car = &parked_car_object;
    Angle ang;

    car->model->mats[CAR_LIGHTS_MATERIAL].attr_texId = CAR_LIGHTS_ON_TEXTURE;
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    TURN_VIA(njRotateY, work->ang.y, ang);
    njSetTexture(&sscar_texlist);
    draw_tree(car);
    njPopMatrix(1);
    car->model->mats[CAR_LIGHTS_MATERIAL].attr_texId =
        VARS(work)->lights_texture;
}

static void ev0050_car_step(struct task *task)
{
    struct task_work *work = task->work;
    struct event_work *event = work->event;
    /* @hack type: flags as an int, which SHC widens once and keeps in R12
     * (EXTU.B R12,R12 at 0x0cb838c2), as the image does. as a Uint8, SHC
     * keeps it on the stack and widens it at each test, and 93 of 97 words
     * differ */
    int flags = event->flags;
    NJS_POINT3 under;
    struct angles slope;
    Float x;
    Float y;
    Float z;

    if (event->paths != NULL) {
        event_paths_step(work);
    }
    x = work->pos.x;
    y = work->pos.y;
    z = work->pos.z;
    /* @bug mismatched-call: the step reads ground_below's height from R0 as an
     * int (LDS R0,FPUL and FLOAT at 0x0cb838e2 and 0x0cb838ea), though
     * ground_below returns it in FR0. the car's height under EVENT_GROUND,
     * plus 3, and its shadow's less SHADOW_LIFT, are whatever R0 holds */
#ifdef GCC
    /* repairs the height the tag names: GCC reads the Float ground_below
     * returns */
    under.y = ground_below(x, y, z, &slope);
#else
    under.y = ground_below_int(x, y, z, &slope);
#endif
    if (flags & EVENT_GROUND) {
        work->pos.y = under.y - 3.0F;
    }
    if (flags & EVENT_SLOPE) {
        work->ang.x = slope.x;
        work->ang.z = slope.z;
    }
    if (event->shadow > 0.0F) {
        under.x = x;
        under.y += SHADOW_LIFT;
        under.z = z;
        shadow_draw_at(&slope, &under, event->shadow);
    }
    ev0050_car_draw(task);
    if (work->collision->shapes->radius > 0.0F) {
        collision_register(work);
    }
}

struct task *ev0050_car_create(void)
{
    struct task *task;

    task = task_create(TASK_WORK, 3, ev0050_car_step);
    if (task != NULL) {
        event_alloc(task->work);
        collision_init(task, ev0050_car_collision, 4, 1, 1);
        /* @hack spelling: the model as (&parked_car_object)->model. SHC then
         * loads the object's address and reads the model 4 bytes past it, as
         * the image does (MOV.L @(4,R3),R1 at 0x0cb83994). as
         * parked_car_object.model, SHC loads the field's own address from the
         * pool, and 8 of 37 words differ */
        VARS(task->work)->lights_texture =
            (&parked_car_object)->model->mats[CAR_LIGHTS_MATERIAL].attr_texId;
        task->draw = ev0050_car_draw;
        task->destructor = ev0050_car_destroy;
    }
    return task;
}
