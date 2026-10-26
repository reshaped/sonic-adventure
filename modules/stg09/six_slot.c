/* O 6SLOT, a spinning slot that scatters rings when hit */
#include <ninja.h>
#include <sg_xpt.h>
#include <stdlib.h>

#include "collision/collision.h"
#include "debug/editor.h"
#include "draw/model.h"
#include "draw/tree.h"
#include "object/placement.h"
#include "object/ring.h"
#include "sound/sound.h"
#include "stg09/casino.h"
#include "stg09/fragments.h"
#include "stg09/helpers.h"
#include "stg09/six_slot.h"
#include "system/task.h"
#include "system/util.h"
#include "vmu/vibration.h"

/* the body, with the spinning top as its child */
extern NJS_OBJECT six_slot_object;
extern NJS_MESHSET six_slot_face_meshset;

/* placement_try_leave_by's squared range: about 600 from the camera or
 * player 0 */
#define RANGE 360010.0F

/* the hits that scatter rings. players can aim at the slot again once
 * SIX_SLOT_COOLDOWN steps have passed since its start or a hit */
#define SIX_SLOT_HITS 4
#define SIX_SLOT_COOLDOWN 60

/* a hit sets boost to HIT_BOOST, and each step keeps BOOST_KEPT of it. once
 * the hits are spent, each step keeps SPIN_KEPT of scl.y */
#define HIT_BOOST 4.0F
#define BOOST_KEPT 0.985F
#define SPIN_KEPT 0.98F

/* each step of a hop adds BOB_GRAVITY to its speed upward. the body draws
 * BODY_BOB times bob higher */
#define BOB_GRAVITY (-0.12F)
#define BODY_BOB 0.8F

struct six_slot_vars {
    Angle spin; /* the top's turn about y */
    /* the hop a hit starts. the body draws BODY_BOB times bob higher, and
     * the top bob higher than the body */
    Float bob;
    Float boost; /* the top spins boost + 1 times as fast as scl.y */
};

#define VARS(work) ((struct six_slot_vars *)&(work)->vars)
/* the steps since the slot's start or a hit, held at SIX_SLOT_COOLDOWN */
#define COOLDOWN(work) ((work)->mode)
/* the face showing: 0 for the face meshset's own UVs, or 1 to
 * SIX_SLOT_FACES */
#define FACE(work) ((work)->submode)
#define HITS(work) ((work)->tick) /* the hits left that scatter rings */
#define BOB_SPEED(work) ((work)->scl.x) /* the hop's speed upward */
/* the face changes once work->timer passes FACE_WAIT, after FACE_WAIT + 2
 * steps */
#define FACE_WAIT(work) ((Sint16)(work)->scl.z)

/* the faces besides the face meshset's own, each a shift of its UVs */
#define SIX_SLOT_FACES 3

/* the body, and the top's blade, which turns with the top */
static struct collision_shape six_slot_shapes[2] = {
    {
        SHAPE_SPHERE | SHAPE_HURTABLE,
        0,
        PUSH_ALL | PUSH_FIRM(PUSH_ALL),
        {0.0F, -6.0F, 0.0F},
        22.0F,
        0.0F,
        0.0F,
        {0},
        0,
        NULL,
    },
    {
        SHAPE_RECTANGLE | SHAPE_HURTABLE,
        0,
        PUSH_ALL | PUSH_FIRM(PUSH_ALL),
        {0.0F, 19.0F, 0.0F},
        1.5F,
        3.0F,
        11.0F,
        {0},
        0,
        NULL,
    },
};
/* faces 1 to 3, each 64 further along in u than the one before */
static struct uv_shift six_slot_faces[SIX_SLOT_FACES] = {
#include "stg09/six_slot/six_slot_faces.inc"
};

static void six_slot_init(struct task *task);
static void six_slot_destroy(struct task *task);
static void six_slot_run(struct task *task);
static void six_slot_draw(struct task *task);

void six_slot_step(struct task *task)
{
    /* @hack redundant: work, which nothing reads, keeps task in R14 across the
     * range check (MOV R4,R14 at 0x0c90b548). without it, SHC keeps task in
     * the frame, and 12 of 16 words differ */
    struct task_work *work = task->work;

    if (!placement_try_leave_by(task, RANGE)) {
        six_slot_init(task);
    }
}

static void six_slot_init(struct task *task)
{
    struct task_work *work = task->work;

    if (!placement_editing(task)) {
        work->ang.x = 0;
        work->ang.z = 0;
        collision_init(task, six_slot_shapes, COLLISION_OBJECT, 1, 2);
    }
    task->draw = six_slot_draw;
    task->step = six_slot_run;
    task->destructor = six_slot_destroy;
    work->timer = 0;
    FACE(work) = 0;
    VARS(work)->boost = 0.0F;
    HITS(work) = SIX_SLOT_HITS;
    COOLDOWN(work) = 0;
    VARS(work)->bob = 0.0F;
    VARS(work)->spin = rand() * (360.0F * RAND_SCALE) * ANGLE_PER_DEGREE;
    uv_shift_make(six_slot_faces, SIX_SLOT_FACES);
}

static void six_slot_destroy(struct task *task)
{
    uv_shift_free(six_slot_faces, SIX_SLOT_FACES);
}

static void six_slot_run(struct task *task)
{
    struct task_work *work = task->work;
    NJS_VECTOR speed;
    Float *to;

    if (placement_try_leave_by(task, RANGE)) {
        return;
    }
    six_slot_draw(task);
    VARS(work)->spin +=
        (Angle)(work->scl.y * (VARS(work)->boost + 1.0F) * ANGLE_PER_DEGREE);
    if (work->scl.z >= 0.0F) {
        if (TASK_TIMER(work)++ > FACE_WAIT(work)) {
            ++FACE(work);
            if (FACE(work) > SIX_SLOT_FACES) {
                FACE(work) = 0;
            }
            work->timer = 0;
        }
    }
    if (placement_editing(task)) {
        njPrintC(EDITOR_LABEL_RIGHT(EDITOR_ROW_SCL_Y), "-> RotSpd ");
        njPrintC(EDITOR_LABEL_RIGHT(EDITOR_ROW_SCL_Z), "-> UV Pitch");
        return;
    }
    work->collision->flags &= ~COLLISION_TARGET;
    if (HITS(work) && (work->flags & COLLISION_DAMAGED)) {
        work->flags &= ~COLLISION_DAMAGED;
        casino_knock_player_back(work);
        ring_scatter(work->pos.x, work->pos.y - 6.0F, work->pos.z, HITS(work));
        --HITS(work);
        VARS(work)->boost = HIT_BOOST;
        sound_play(SE_ITEM, 0, 0, 0);
        /* @hack spelling: the speed written through a Float walk, which steps
         * R4 across it (MOV R15,R4 and ADD #4,R4 from 0x0c90b740). through its
         * fields, the code is 32 bytes short and 116 of 233 words differ */
        to = &speed.x;
        *to++ = 0.0F;
        *to++ = 0.1F;
        *to = 0.0F;
        vibration_start(0, 6, 30, 0);
        casino_dust_toward_camera(&work->pos, &speed, 8.0F, 22.0F);
        COOLDOWN(work) = 0;
        VARS(work)->bob = 0.01F;
        BOB_SPEED(work) = 1.0F;
    } else if (HITS(work)) {
        if (COOLDOWN(work)++ >= SIX_SLOT_COOLDOWN) {
            work->collision->flags |= COLLISION_TARGET;
            COOLDOWN(work) = SIX_SLOT_COOLDOWN;
        }
    } else {
        work->scl.y *= SPIN_KEPT;
        work->scl.z = -1.0F;
    }
    VARS(work)->boost *= BOOST_KEPT;
    if (VARS(work)->bob > 0.0F) {
        BOB_SPEED(work) += BOB_GRAVITY;
        VARS(work)->bob += BOB_SPEED(work);
        if (VARS(work)->bob < 0.0F) {
            VARS(work)->bob = 0.0F;
        }
    }
    work->collision->shapes[1].ang.y = VARS(work)->spin;
    collision_register(work);
}

static void six_slot_draw(struct task *task)
{
    struct task_work *work = task->work;
    NJS_OBJECT *object = &six_slot_object;
    Angle ang;

    if (placement_try_leave_by(task, RANGE)) {
        return;
    }
    if (!draw_in_view(work->pos.x, work->pos.y, work->pos.z, 25.0F)) {
        return;
    }
    if (FACE(work) != 0) {
        uv_shift_use(&six_slot_faces[FACE(work) - 1], 1);
    }
    casino_set_texture(CASINO_TEXTURES_ACTS_12);
    njPushMatrix(NULL);
    njTranslate(NULL, work->pos.x, work->pos.y + (VARS(work)->bob * BODY_BOB),
                work->pos.z);
    TURN_VIA(njRotateY, work->ang.y, ang);
    draw_model(object->model);
    TURN_VIA(njRotateY, VARS(work)->spin, ang);
    njTranslate(NULL, 0.0F, VARS(work)->bob, 0.0F);
    draw_tree(object->child);
    njPopMatrix(1);
    if (FACE(work) != 0) {
        uv_shift_restore(six_slot_faces, SIX_SLOT_FACES);
    }
}
