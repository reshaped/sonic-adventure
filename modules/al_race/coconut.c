/* COCONUT, a coconut that falls and bounces to rest */
#include <ninja.h>
#include <sg_xpt.h>
#include <stdlib.h>

#include "al_race/coconut.h"
#include "al_race/item.h"
#include "collision/collision.h"
#include "debug/editor.h"
#include "draw/model.h"
#include "object/placement.h"
#include "object/simple.h"
#include "system/frame.h"
#include "system/task.h"
#include "system/task_desc.h"
#include "system/texture.h"
#include "system/util.h"

/* the coconut's object */
extern NJS_OBJECT dat_0cbd12e8;

/* the coconut's state, in its task's motion */
struct coconut {
    Sint8 mode; /* an enum coconut_mode */
    Uint8 unk_01[3];
    NJS_MODEL *model;
    struct collision_list *list;
    NJS_VECTOR speed;
    struct task *knocker; /* the task of the Chao that last hit it, or null */
    Angle phase; /* a random angle each hit sets, which nothing reads */
};

#define COCONUT(task) ((struct coconut *)(task)->motion)

/* coconut_start's arg */
struct coconut_start {
    NJS_OBJECT *object;
    struct collision_list list;
};

enum coconut_mode { COCONUT_BOUNCE, COCONUT_REST };

/* the radius of the coconut's sphere, and the height object_bounce keeps
 * its center above the ground */
#define COCONUT_RADIUS 2.7F

static void coconut_start(struct task *task, void *arg);
static void coconut_step(struct task *task);
static void coconut_draw(struct task *task);

static struct collision_shape coconut_shapes[1];

static const struct task_desc coconut_desc = {
    sizeof(struct coconut), TASK_WORK,    coconut_start,
    coconut_step,           coconut_draw, NULL};
static const struct collision_kind coconut_collision = {COLLISION_OBJECT, 1};
static const struct coconut_start coconut_kind = {&dat_0cbd12e8,
                                                  {coconut_shapes, 1, 40.0F}};
static const NJS_VECTOR coconut_still = {0.0F, 0.0F, 0.0F};
/* the share of each axis of its velocity the coconut keeps on a bounce */
static const NJS_VECTOR coconut_bounce = {0.5F, 0.5F, 0.5F};

static struct collision_shape coconut_shapes[1] = {
    {SHAPE_SPHERE,
     0,
     PUSH_ALL | PUSH_FIRM(PUSH_OBJECTS | PUSH_OTHERS),
     {0.0F, 0.0F, 0.0F},
     COCONUT_RADIUS,
     0.0F,
     0.0F,
     {0, 0, 0},
     0,
     NULL},
};

static void coconut_turn_x(struct angles *ang)
{
    Angle turn;

    TURN_VIA(njRotateX, ang->x, turn);
    /* @hack redundant: turn cleared after its last read, which keeps
     * coconut_turn_x a function of its own that coconut_draw calls (BSR at
     * 0x0cb95f1c), as the image does. without it, SHC expands the call
     * inline and omits the function, none of its 16 words is built, and the
     * object runs 32 bytes short */
    turn = 0; /* NOLINT(clang-analyzer-deadcode.DeadStores) */
}

static void coconut_turn_y(struct angles *ang)
{
    Angle turn;

    TURN_VIA(njRotateY, ang->y, turn);
    /* @hack redundant: turn cleared after its last read, which keeps
     * coconut_turn_y a function of its own that coconut_draw calls (BSR at
     * 0x0cb95f20), as the image does. without it, SHC expands the call
     * inline and omits the function, none of its 16 words is built, and the
     * object runs 32 bytes short */
    turn = 0; /* NOLINT(clang-analyzer-deadcode.DeadStores) */
}

static void coconut_turn_z(struct angles *ang)
{
    Angle turn;

    TURN_VIA(njRotateZ, ang->z, turn);
    /* @hack redundant: turn cleared after its last read, which keeps
     * coconut_turn_z a function of its own that coconut_draw calls (BSR at
     * 0x0cb95f18), as the image does. without it, SHC expands the call
     * inline and omits the function, none of its 16 words is built, and the
     * object runs 32 bytes short */
    turn = 0; /* NOLINT(clang-analyzer-deadcode.DeadStores) */
}

void coconut_init(struct task *task)
{
    task_apply_desc(task, &coconut_desc, (void *)&coconut_kind);
}

static void coconut_start(struct task *task, void *arg)
{
    struct coconut *coconut = COCONUT(task);
    struct collision_list *list = &((struct coconut_start *)arg)->list;
    Bool live = !placement_editing(task);

    if (live) {
        object_collision_init(task, list,
                              (struct collision_kind *)&coconut_collision);
    }
    coconut->mode = COCONUT_BOUNCE;
    coconut->model = ((struct coconut_start *)arg)->object->model;
    coconut->list = list;
    coconut->speed = coconut_still;
    coconut->knocker = NULL;
    coconut->phase = 0;
}

/* a Chao that hits a resting coconut sets it bouncing again, with no new
 * velocity */
static void coconut_step(struct task *task)
{
    struct coconut *coconut;
    struct task_work *work;
    struct task *knocker;

    coconut_draw(task);
    work = task->work;
    coconut = COCONUT(task);
    if (!placement_editing(task)) {
        switch (coconut->mode) {
        case COCONUT_BOUNCE: {
            NJS_VECTOR *bounce = (NJS_VECTOR *)&coconut_bounce;

            if (!object_bounce(&work->pos, &coconut->speed, bounce,
                               COCONUT_RADIUS)) {
                coconut->mode = COCONUT_REST;
            }
            break;
        }
        case COCONUT_REST:
            knocker = item_hit_chao(work->collision);
            if (knocker == NULL) {
                collision_register(work);
            } else {
                coconut->knocker = knocker;
                coconut->phase = RANDOM_ANGLE();
                coconut->mode = COCONUT_BOUNCE;
            }
            break;
        }
    }
}

static void coconut_draw(struct task *task)
{
    struct coconut *coconut;
    struct task_work *work;
    struct angles *ang;

    if (frame_step == 0 && !placement_far_from_camera(task)) {
        work = task->work;
        coconut = COCONUT(task);
        njPushMatrix(NULL);
        njTranslateV(NULL, &work->pos);
        ang = &work->ang;
        coconut_turn_z(ang);
        coconut_turn_x(ang);
        coconut_turn_y(ang);
        texture_set_stage();
        draw_model(coconut->model);
        njPopMatrix(1);
    }
}
