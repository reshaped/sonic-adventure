/* SAND GATE: a gate that scores in the gate combo the first time something
 * passes through it */
#include <ninja.h>
#include <sg_xpt.h>

#include "collision/collision.h"
#include "debug/editor.h"
#include "draw/model.h"
#include "object/placement.h"
#include "object/simple.h"
#include "sboard/gate_combo.h"
#include "sboard/object_textures.h"
#include "sboard/sand_gate.h"
#include "sound/cue.h"
#include "system/frame.h"
#include "system/task.h"
#include "system/task_desc.h"
#include "system/texture.h"
#include "system/util.h"

/* the gate's kind, which init takes as arg */
struct sand_gate_kind {
    NJS_OBJECT *object;
    struct collision_list list;
};

/* @hack spelling: arg read through KIND at each use, which keeps arg on the
 * stack (MOV.L R5,@R15 at 0x0c9042f6); through a local, SHC keeps it in
 * R12, sand_gate_init runs 2 bytes long, and 14 of its 31 words differ */
#define KIND(arg) ((struct sand_gate_kind *)(arg))

/* a gate's record, in task->motion */
struct sand_gate_record {
    Sint8 mode;
    NJS_MODEL *model;
};

enum sand_gate_mode { SAND_GATE_WAIT, SAND_GATE_PASSED };

#define SE_SAND_GATE 21

static void sand_gate_init(struct task *task, void *arg);
static void sand_gate_run(struct task *task);
static void sand_gate_draw(struct task *task);

static const struct task_desc sand_gate_desc = {sizeof(struct sand_gate_record),
                                                TASK_WORK | TASK_FORCE,
                                                sand_gate_init,
                                                sand_gate_run,
                                                sand_gate_draw,
                                                NULL};
static const struct collision_kind sand_gate_collision = {COLLISION_OBJECT, 1};

/* the gate's shape: a box 60 wide, 88 high and 0.2 deep, centered on the
 * gate. it pushes no group, and none can push it, so a player passes
 * through */
static struct collision_shape sand_gate_shapes[1] = {
    {SHAPE_RECTANGLE,
     ATTACK(3),
     PUSH_FIRM(PUSH_ALL),
     {0.0F, 0.0F, 0.0F},
     30.0F,
     44.0F,
     0.1F,
     {0},
     0,
     NULL},
};
static struct sand_gate_kind sand_gate = {&sand_gate_object,
                                          {sand_gate_shapes, 1, 40.0F}};

static void sand_gate_turn_x(struct angles *ang)
{
    Angle x;

    TURN_VIA(njRotateX, ang->x, x);
}

static void sand_gate_turn_y(struct angles *ang)
{
    Angle y;

    TURN_VIA(njRotateY, ang->y, y);
}

static void sand_gate_turn_z(struct angles *ang)
{
    Angle z;

    TURN_VIA(njRotateZ, ang->z, z);
}

void sand_gate_step(struct task *task)
{
    task_apply_desc(task, &sand_gate_desc, &sand_gate);
}

/* give the gate its shape, unless the set editor holds it */
static void sand_gate_init(struct task *task, void *arg)
{
    struct sand_gate_record *record = (struct sand_gate_record *)task->motion;
    NJS_OBJECT *object = KIND(arg)->object;
    NJS_MODEL *model = object->model;

    if (!placement_editing(task)) {
        object_collision_init(task, &KIND(arg)->list, &sand_gate_collision);
    }
    record->mode = SAND_GATE_WAIT;
    record->model = model;
}

/* draw the gate. unless the set editor holds it, it registers its shape
 * each step until the first touch. that touch scores in the combo and plays
 * SE_SAND_GATE */
static void sand_gate_run(struct task *task)
{
    struct task_work *work;
    struct sand_gate_record *record;

    if (placement_try_leave(task)) {
        return;
    }
    sand_gate_draw(task);
    work = task->work;
    record = (struct sand_gate_record *)task->motion;
    if (!placement_editing(task)) {
        switch (record->mode) {
        case SAND_GATE_WAIT:
            /* @bug memory: a gate the set editor held as it started has no
             * collision record. once the editor lets it go, this reads the
             * record's flags through null, at 0x0c90435c */
            if (work->collision->flags & COLLISION_HIT) {
                gate_combo_add();
                sound_play_near(SE_SAND_GATE, &work->pos, NULL);
                record->mode = SAND_GATE_PASSED;
            } else {
                collision_register(work);
            }
            break;
        case SAND_GATE_PASSED:
            break;
        }
    }
}

/* on a frame's first step, draw the gate with the stage's textures */
static void sand_gate_draw(struct task *task)
{
    struct task_work *work;
    struct sand_gate_record *record;
    struct angles *ang;

    if (frame_step == 0) {
        work = task->work;
        record = (struct sand_gate_record *)task->motion;
        njPushMatrix(NULL);
        njTranslateV(NULL, &work->pos);
        ang = &work->ang;
        sand_gate_turn_z(ang);
        sand_gate_turn_x(ang);
        sand_gate_turn_y(ang);
        texture_set_stage();
        draw_model(record->model);
        njPopMatrix(1);
    }
}
