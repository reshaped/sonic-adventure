/* O EGGWARP, the warp to the Egg Carrier's Chao Garden */
#include <ninja.h>
#include <sg_xpt.h>

#include "adv0130/dat_0ca14ff0.h"
#include "adv0130/egg_warp.h"
#include "collision/collision.h"
#include "debug/editor.h"
#include "draw/draw.h"
#include "draw/model.h"
#include "draw/tree.h"
#include "player/player.h"
#include "stage/stage.h"
#include "system/pad.h"
#include "system/task.h"
#include "system/util.h"

extern NJS_OBJECT egg_warp_object;
extern NJS_MODEL egg_warp_ring_model;

/* the warp starts once a player has stood in its middle sphere more than
 * STAND_STEPS steps in a row. it leaves WARP_PAUSE once more than PAUSE_STEPS
 * steps have passed since it started. a ring goes up each time more than
 * RING_STEPS steps have passed since the warp started or the last ring went
 * up */
#define STAND_STEPS 50
#define PAUSE_STEPS 2
#define RING_STEPS 20
/* the rings' countdown starts at RINGS_START and drops RING_DROP as each
 * ring goes up. the ring that takes it under RINGS_END, the fifth, is the
 * last */
#define RINGS_START 60
#define RING_DROP 10
#define RINGS_END 20

struct egg_warp_vars {
    Uint8 unk_00;
    Uint8 countdown; /* the rings' countdown */
};

#define VARS(work) ((struct egg_warp_vars *)&(work)->vars)

/* a ring's work->scl.x and scl.y scale its model's width and height, and
 * scl.z is its constant material. work->ang.x is the phase that sets its
 * height and its rise */
struct egg_warp_ring_vars {
    Float base; /* the height the ring rises from */
};

#define RING(work) ((struct egg_warp_ring_vars *)&(work)->vars)

enum egg_warp_mode { WARP_WAIT, WARP_PAUSE, WARP_RINGS, WARP_GO };

/* three cylinders, each narrower and taller than the last, and shape 3, the
 * sphere in their middle that a player stands in */
static const struct collision_shape egg_warp_shapes[] = {
    {SHAPE_CYLINDER,
     0,
     PUSH_ALL | PUSH_FIRM(PUSH_ALL),
     {0.0F, 0.0F, 0.0F},
     14.7F,
     1.4F,
     0.0F,
     {0, 0, 0},
     0,
     NULL},
    {SHAPE_CYLINDER,
     0,
     PUSH_ALL | PUSH_FIRM(PUSH_ALL),
     {0.0F, 0.0F, 0.0F},
     12.7F,
     2.8F,
     0.0F,
     {0, 0, 0},
     0,
     NULL},
    {SHAPE_CYLINDER,
     0,
     PUSH_ALL | PUSH_FIRM(PUSH_ALL),
     {0.0F, 0.0F, 0.0F},
     8.5F,
     4.5F,
     0.0F,
     {0, 0, 0},
     0,
     NULL},
    {SHAPE_SPHERE | SHAPE_HURTABLE | SHAPE_NO_PUSH,
     0,
     PUSH_FIRM(PUSH_ALL),
     {0.0F, 4.8F, 0.0F},
     3.7F,
     0.0F,
     0.0F,
     {0, 0, 0},
     0,
     NULL},
};

static void egg_warp_draw_work(struct task_work *work);
static void egg_warp_run(struct task *task);
static void egg_warp_destroy(struct task *task);
static void egg_warp_draw(struct task *task);
static void egg_warp_ring_init(struct task *task);
static void egg_warp_ring_draw_work(struct task_work *work);
static void egg_warp_ring_step(struct task *task);
static void egg_warp_ring_destroy(struct task *task);
static void egg_warp_ring_draw(struct task *task);

void egg_carrier_egg_warp_step(struct task *task)
{
    if (!placement_editing(task)) {
        collision_init(task, egg_warp_shapes, COLLISION_OBJECT, 1, 4);
    }
    task->step = egg_warp_run;
    task->destructor = egg_warp_destroy;
    task->draw = egg_warp_draw;
}

static void egg_warp_draw_work(struct task_work *work)
{
    Angle ang;

    if (draw_in_view(work->pos.x, work->pos.y, work->pos.z, 20.0F)) {
        njSetTexture(&ec_alife_texlist);
        njPushMatrix(NULL);
        njTranslate(NULL, work->pos.x + (njSin(work->ang.x) * 0.03F),
                    work->pos.y + (njSin(work->ang.x + work->ang.z) * 0.05F),
                    work->pos.z + (njSin(work->ang.z) * 0.08F));
        TURN_VIA(njRotateY, work->ang.y, ang);
        njScale(NULL, 0.7F, 0.7F, 0.7F);
        draw_tree(&egg_warp_object);
        njPopMatrix(1);
    }
}

/* in the set editor the warp only draws. otherwise it waits for a player to
 * stand in its middle sphere, shape 3, for more than STAND_STEPS steps. it
 * then stops player 0's pad and sends up five rings, shaking as they go. each
 * ring turns player 0 and lifts it a little. the stage then fades to the Chao
 * Garden */
static void egg_warp_run(struct task *task)
{
    struct task_work *work = task->work;
    struct collision_work *hit;
    struct task_work *ring_work;
    struct task *ring;

    if (edit_mode) {
        egg_warp_draw_work(work);
    } else {
        hit = work->collision->shapes[3].hit;
        switch (work->mode) {
        case WARP_WAIT:
            if (hit != NULL) {
                if (hit->kind == COLLISION_PLAYER) {
                    if (TASK_TIMER(work) > STAND_STEPS) {
                        work->timer = 0;
                        work->mode++;
                        VARS(work)->countdown = RINGS_START;
                        pad_disable_player(0);
                    }
                } else {
                    work->timer = 0;
                }
            } else {
                work->timer = 0;
            }
            break;
        case WARP_PAUSE:
            if (TASK_TIMER(work) > PAUSE_STEPS) {
                work->mode++;
            }
            break;
        case WARP_RINGS:
            if (TASK_TIMER(work) > RING_STEPS) {
                ring =
                    task_create(TASK_MOTION | TASK_WORK, 4, egg_warp_ring_init);
                ring_work = ring->work;
                ring_work->pos = work->pos;
                RING(ring_work)->base = work->pos.y + 2.0F;
                work->timer = 0;
                VARS(work)->countdown -= RING_DROP;
                if (VARS(work)->countdown < RINGS_END) {
                    work->mode++;
                    work->timer = 0;
                }
                player_work[0]->ang.y += 0x100;
                player_work[0]->pos.y += 0.1F;
            }
            work->ang.x += 0x2000;
            work->ang.z += 0x1200;
            break;
        case WARP_GO:
            stage_set_entrance(0);
            stage_fade_to(STAGE_ALIFE_EG, 0);
            break;
        }
        egg_warp_draw_work(work);
        collision_register(work);
        work->tick++;
        work->timer++;
    }
}

static void egg_warp_destroy(struct task *task)
{
}

static void egg_warp_draw(struct task *task)
{
    struct task_work *work = task->work;

    egg_warp_draw_work(work);
}

static void egg_warp_ring_init(struct task *task)
{
    struct task_work *work = task->work;

    work->scl.x = 1.0F;
    work->scl.y = 0.0F;
    work->scl.z = 0.0F;
    work->ang.x = 0;
    work->ang.z = 0;
    task->step = egg_warp_ring_step;
    task->destructor = egg_warp_ring_destroy;
    task->draw = egg_warp_ring_draw;
}

static void egg_warp_ring_draw_work(struct task_work *work)
{
    Float width = work->scl.x * 0.7F;
    Float height = work->scl.y * 4.5F;
    Float shade = work->scl.z;
    Angle ang;

    njSetTexture(&ec_alife_texlist);
    draw_attr_set(0, NJD_FLAG_USE_ALPHA);
    njPushMatrix(NULL);
    draw_set_material(shade, shade, shade, shade);
    njTranslateV(NULL, &work->pos);
    TURN_VIA(njRotateY, work->ang.y, ang);
    njScale(NULL, width, height, width);
    draw_model(&egg_warp_ring_model);
    njPopMatrix(1);
    draw_attr_clear(0, NJD_FLAG_USE_ALPHA);
    draw_set_material(0.0F, 0.0F, 0.0F, 0.0F);
}

/* each step, the ring turns, narrows and fades, and its phase moves on. its
 * height follows the phase's sine as it rises 18 above its base. the task
 * dies once the phase passes half a turn */
static void egg_warp_ring_step(struct task *task)
{
    struct task_work *work = task->work;

    /* @hack spelling: the mode tested by a switch of one case, which tests it
     * with CMP/EQ #0,R0 (0x0c92b50e); as an if, SHC tests it with TST R3,R3 and
     * 7 of 80 words differ */
    switch (work->mode) {
    case 0:
        work->ang.y += 0x180;
        work->scl.x -= 0.002F;
        work->scl.y = njSin(work->ang.x);
        work->ang.x += 0x222;
        work->scl.z -= 1.0F / 55.0F;
        if (work->ang.x < 0x4000) {
            work->pos.y = RING(work)->base + (work->scl.y * 9.0F);
        } else {
            work->pos.y = RING(work)->base + ((2.0F - work->scl.y) * 9.0F);
        }
        egg_warp_ring_draw_work(work);
        break;
    }
    if (work->ang.x > 0x8000) {
        task_kill(task);
    }
}

static void egg_warp_ring_destroy(struct task *task)
{
}

static void egg_warp_ring_draw(struct task *task)
{
    struct task_work *work = task->work;

    egg_warp_ring_draw_work(work);
}
