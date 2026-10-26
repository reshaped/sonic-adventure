/* O TENSS and O TENSC, gates under a slot sign and a card sign. each sign
 * steps through its faces, and each gate's lights run while a teleporter
 * holds player 0 */
#include <ninja.h>
#include <sg_xpt.h>

#include "collision/collision.h"
#include "collision/world.h"
#include "debug/editor.h"
#include "draw/model.h"
#include "draw/tree.h"
#include "object/placement.h"
#include "player/player.h"
#include "stg09/casino.h"
#include "stg09/helpers.h"
#include "stg09/props.h"
#include "stg09/transfer_gate.h"
#include "system/task.h"
#include "system/util.h"

extern NJS_OBJECT transfer_gate_slot_object;
extern NJS_OBJECT transfer_gate_card_object;
extern NJS_MESHSET transfer_gate_slot_sign_meshset_0;
extern NJS_MESHSET transfer_gate_slot_sign_meshset_1;
extern NJS_MESHSET transfer_gate_slot_lights_meshset;
extern NJS_MESHSET transfer_gate_card_sign_meshset_0;
extern NJS_MESHSET transfer_gate_card_sign_meshset_1;
extern NJS_MESHSET transfer_gate_card_sign_meshset_2;
extern NJS_MESHSET transfer_gate_card_lights_meshset_0;
extern NJS_MESHSET transfer_gate_card_lights_meshset_1;

/* a gate starts only within about 700 units of the camera and player 0, and
 * is destroyed otherwise. RANGE is that distance squared */
#define RANGE 490010.0F

/* the flags of the gate's copy in the collision world */
#define GATE_WORLD_FLAGS (WORLD_MEDIUM | WORLD_UNCLIMBABLE | WORLD_SOLID)

/* what the gate's near test adds to its kind's radius */
#define NEAR_MARGIN 16.0F

/* the sign's faces past its own, each a shift of its meshsets' UVs */
#define FACES 3

/* the UVs the draw's room holds for the lights' moved copies */
#define LIGHT_UVS 100

struct transfer_gate_kind {
    NJS_OBJECT *object;
    /* the radius draw_in_view tests. the collision world tests the gate
     * while a player is within radius + NEAR_MARGIN of it */
    Float radius;
    struct uv_shift *faces; /* FACES rows of sign_count, a face to a row */
    int sign_count;
    struct uv_scroll *lights;
    int light_count;
    struct collision_shape *posts;
    int post_count;
};

struct transfer_gate_vars {
    NJS_OBJECT *object; /* its copy in the collision world */
    Uint8 unk_04[4];
    struct transfer_gate_kind *kind;
};

#define VARS(work) ((struct transfer_gate_vars *)&(work)->vars)
/* the sign's face, 0 for its own, then 1 to FACES */
#define FACE(work) ((work)->submode)
/* half the lights' scroll phase, 0 while they rest */
#define HALF_PHASE(work) ((work)->mode)
/* the sign shows each face until the timer passes FACE_WAIT, for FACE_WAIT
 * + 2 steps */
#define FACE_WAIT(work) ((Sint16)(work)->scl.z)
/* how far HALF_PHASE moves a step while the lights run */
#define LIGHT_SPEED(work) ((int)(work)->scl.y)

static struct uv_shift transfer_gate_card_faces[FACES * 3] = {
#include "stg09/transfer_gate/transfer_gate_card_faces.inc"
};
static struct collision_shape transfer_gate_card_posts[2] = {
    {
        SHAPE_RECTANGLE,
        0,
        PUSH_ALL | PUSH_FIRM(PUSH_ALL),
        {35.0F, 22.0F, 0.0F},
        10.0F,
        20.0F,
        10.0F,
        {0},
        0,
        NULL,
    },
    {
        SHAPE_RECTANGLE,
        0,
        PUSH_ALL | PUSH_FIRM(PUSH_ALL),
        {-35.0F, 22.0F, 0.0F},
        10.0F,
        20.0F,
        10.0F,
        {0},
        0,
        NULL,
    },
};
static struct uv_scroll transfer_gate_card_lights[2] = {
#include "stg09/transfer_gate/transfer_gate_card_lights.inc"
};
static struct uv_shift transfer_gate_slot_faces[FACES * 2] = {
#include "stg09/transfer_gate/transfer_gate_slot_faces.inc"
};
static struct collision_shape transfer_gate_slot_posts[2] = {
    {
        SHAPE_RECTANGLE,
        0,
        PUSH_ALL | PUSH_FIRM(PUSH_ALL),
        {33.38F, 21.0F, 0.0F},
        7.0F,
        20.0F,
        10.0F,
        {0},
        0,
        NULL,
    },
    {
        SHAPE_RECTANGLE,
        0,
        PUSH_ALL | PUSH_FIRM(PUSH_ALL),
        {-33.38F, 21.0F, 0.0F},
        7.0F,
        20.0F,
        10.0F,
        {0},
        0,
        NULL,
    },
};
static struct uv_scroll transfer_gate_slot_lights[1] = {
#include "stg09/transfer_gate/transfer_gate_slot_lights.inc"
};
static struct transfer_gate_kind transfer_gate_slot = {
    &transfer_gate_slot_object, 140.0F, transfer_gate_slot_faces, 2,
    transfer_gate_slot_lights,  1,      transfer_gate_slot_posts, 2,
};
static struct transfer_gate_kind transfer_gate_card = {
    &transfer_gate_card_object, 140.0F, transfer_gate_card_faces, 3,
    transfer_gate_card_lights,  2,      transfer_gate_card_posts, 2,
};

static void transfer_gate_init(struct task *task,
                               struct transfer_gate_kind *kind);
static void transfer_gate_destroy(struct task *task);
static void transfer_gate_draw(struct task *task);
static void transfer_gate_run(struct task *task);

void transfer_gate_slot_step(struct task *task)
{
    transfer_gate_init(task, &transfer_gate_slot);
}

void transfer_gate_card_step(struct task *task)
{
    transfer_gate_init(task, &transfer_gate_card);
}

static void transfer_gate_init(struct task *task,
                               struct transfer_gate_kind *kind)
{
    struct task_work *work = task->work;
    NJS_OBJECT *object;

    if (!placement_try_leave_by(task, RANGE)) {
        task->draw = transfer_gate_draw;
        task->step = transfer_gate_run;
        task->destructor = transfer_gate_destroy;
        VARS(work)->kind = kind;
        work->timer = 0;
        FACE(work) = 0;
        HALF_PHASE(work) = 0;
        uv_shift_make(kind->faces, kind->sign_count * FACES);
        uv_scroll_save(kind->lights, kind->light_count);
        if (!placement_editing(task)) {
            object = VARS(work)->object = world_object_alloc();
            *object = *kind->object;
            object->evalflags = 0;
            object->pos[0] = work->pos.x;
            object->pos[1] = work->pos.y;
            object->pos[2] = work->pos.z;
            object->ang[1] += work->ang.y;
            work->flags &= ~WORLD_TESTED;
            world_add(GATE_WORLD_FLAGS, task, object);
            collision_init(task, kind->posts, COLLISION_OBJECT, 1,
                           kind->post_count);
        }
    }
}

static void transfer_gate_destroy(struct task *task)
{
    struct task_work *work = task->work;
    struct transfer_gate_kind *kind = VARS(work)->kind;

    uv_shift_free(kind->faces, FACES * kind->sign_count);
    uv_scroll_release(kind->lights, kind->light_count);
    if (VARS(work)->object) {
        world_remove(task, VARS(work)->object);
        world_object_free(VARS(work)->object);
    }
}

static void transfer_gate_draw(struct task *task)
{
    struct task_work *work = task->work;
    struct transfer_gate_kind *kind = VARS(work)->kind;
    NJS_TEX room[LIGHT_UVS];
    Angle ang;

    if (placement_try_leave_by(task, RANGE)) {
        return;
    }
    if (!draw_in_view(work->pos.x, work->pos.y, work->pos.z, kind->radius)) {
        return;
    }
    if (FACE(work)) {
        uv_shift_use(kind->faces + ((FACE(work) - 1) * kind->sign_count),
                     kind->sign_count);
    }
    if (HALF_PHASE(work)) {
        uv_scroll_move(kind->lights, kind->light_count, room,
                       HALF_PHASE(work) * 2);
    }
    casino_set_texture(CASINO_TEXTURES_ACTS_12);
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    TURN_VIA(njRotateY, work->ang.y, ang);
    draw_tree(kind->object);
    njPopMatrix(1);
    if (HALF_PHASE(work)) {
        uv_scroll_restore(kind->lights, kind->light_count);
    }
    if (FACE(work)) {
        uv_shift_restore(kind->faces, kind->sign_count * FACES);
    }
}

static void transfer_gate_run(struct task *task)
{
    struct task_work *work = task->work;
    struct transfer_gate_kind *kind = VARS(work)->kind;
    NJS_OBJECT *object;

    if (placement_try_leave_by(task, RANGE)) {
        return;
    }
    transfer_gate_draw(task);
    if (TASK_TIMER(work)++ > FACE_WAIT(work)) {
        FACE(work)++;
        if (FACE(work) > FACES) {
            FACE(work) = 0;
        }
        work->timer = 0;
    }
    if (placement_editing(task)) {
        HALF_PHASE(work) += LIGHT_SPEED(work);
        njPrintC(EDITOR_LABEL_RIGHT(EDITOR_ROW_SCL_Y), "-> UV2 Pitch");
        njPrintC(EDITOR_LABEL_RIGHT(EDITOR_ROW_SCL_Z), "-> UV Pitch");
        return;
    }
    object = VARS(work)->object;
    if (player_in_range((NJS_POINT3 *)object->pos,
                        kind->radius + NEAR_MARGIN)) {
        work->flags |= WORLD_TESTED;
    } else {
        work->flags &= ~WORLD_TESTED;
    }
    if (telepot_holding()) {
        HALF_PHASE(work) += LIGHT_SPEED(work);
    } else {
        HALF_PHASE(work) = 0;
    }
    collision_register(work);
}
