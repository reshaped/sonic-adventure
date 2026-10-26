/* O GATE1: two sliding doors that open for player 0 from the side they
 * face */
#include <ninja.h>
#include <sg_xpt.h>

#include "collision/collision.h"
#include "debug/editor.h"
#include "debug/print.h"
#include "draw/model.h"
#include "object/placement.h"
#include "player/player.h"
#include "sound/sound.h"
#include "stg12/gate_1.h"
#include "system/frame.h"
#include "system/task.h"
#include "system/texture.h"
#include "system/util.h"

extern NJS_OBJECT gate_1_object;

#define SE_GATE_SLIDE 313

/* destroyed beyond 500 units of the camera and player 0 */
#define RANGE (500.0F * 500.0F)

/* half a door's length along z */
#define DOOR_HALF 22.4F

/* player 0 opens and holds the doors open within REACH across and HEIGHT
 * above or below the gate */
#define REACH 22.4F
#define HEIGHT 30.0F
/* how far each door slides open, and how far a step */
#define SLIDE 35.0F
#define SLIDE_SPEED 0.1F

enum gate_1_mode {
    GATE_START,
    GATE_SHUT,
    GATE_OPENING,
    GATE_OPEN,
    GATE_CLOSING,
    GATE_STILL
};

/* slide is how far each door has slid open. face_x and face_z are a unit
 * vector across, toward the side player 0 opens the doors from */
struct gate_1_vars {
    Float slide;
    Float face_x;
    Float face_z;
};

#define VARS(work) ((struct gate_1_vars *)&(work)->vars)

static void gate_1_run(struct task *task);
static void gate_1_draw(struct task *task);
static void gate_1_destroy(struct task *task);

static struct collision_shape gate_1_shapes[2] = {
    {
        SHAPE_RECTANGLE,
        0,
        PUSH_ALL | PUSH_FIRM(PUSH_ALL),
        {0.0F, 54.0F, DOOR_HALF},
        4.0F,
        54.0F,
        DOOR_HALF,
        {0},
        0,
        NULL,
    },
    {
        SHAPE_RECTANGLE,
        0,
        PUSH_ALL | PUSH_FIRM(PUSH_ALL),
        {0.0F, 54.0F, -DOOR_HALF},
        4.0F,
        54.0F,
        DOOR_HALF,
        {0},
        0,
        NULL,
    },
};

void gate_1_step(struct task *task)
{
    struct task_work *work = task->work;

    if (placement_try_leave_by(task, RANGE)) {
        return;
    }
    VARS(work)->slide = 0.0F;
    /* @hack redundant: the cosine multiplied by 1, FLDI1 and FMUL at
     * 0x0c901c42; without it, 37 of 53 words differ */
    VARS(work)->face_x = -(njCos(work->ang.y) * 1.0F);
    VARS(work)->face_z = njSin(work->ang.y);
    if (!placement_editing(task)) {
        collision_init(task, gate_1_shapes, COLLISION_OBJECT, 1, 2);
    }
    work->mode = GATE_SHUT;
    task->step = gate_1_run;
    task->draw = gate_1_draw;
    task->destructor = gate_1_destroy;
}

static void gate_1_run(struct task *task)
{
    struct task_work *work = task->work;
    NJS_POINT3 pos;
    Float slide;
    Float edge;
    Float face_x;
    Float face_z;
    Float x;
    Float z;
    Float ahead;
    Float speed;

    if (placement_editing(task)) {
        debug_print(EDITOR_LABEL(EDITOR_ROW_ANG_X), "->");
        debug_print(EDITOR_LABEL(EDITOR_ROW_ANG_Y), "-> ROTATE Y");
        debug_print(EDITOR_LABEL(EDITOR_ROW_ANG_Z), "->");
        gate_1_draw(task);
        return;
    }
    speed = SLIDE_SPEED;
    switch (work->mode) {
    case GATE_SHUT:
        if (player_in_cylinder(&work->pos, REACH, HEIGHT)) {
            player_get_history(0, 0, &pos, NULL);
            face_x = VARS(work)->face_x;
            face_z = VARS(work)->face_z;
            x = pos.x - work->pos.x;
            z = pos.z - work->pos.z;
            /* @hack redundant: a copy of face_x to x that never runs, which
             * keeps the loads at 0x0c901d16, 0x0c901d1c and 0x0c901d2a in FR5,
             * FR7 and FR6; without it, they take FR6, FR5 and FR7 and 9 of 172
             * words differ */
            if (0) {
                x = face_x;
            }
            ahead = (x * face_x) + (z * face_z);
            if (ahead > 0.0F) {
                work->mode = GATE_OPENING;
                VARS(work)->slide = 0.0F;
            }
        }
        break;
    case GATE_OPENING:
        slide = VARS(work)->slide;
        slide += speed;
        if (slide >= SLIDE) {
            slide = SLIDE;
            work->mode = GATE_OPEN;
        }
        edge = slide + DOOR_HALF;
        work->collision->shapes[0].center.z = edge;
        work->collision->shapes[1].center.z = -edge;
        VARS(work)->slide = slide;
        sound_play_for(SE_GATE_SLIDE, (int)task, 1, 0, 2);
        break;
    case GATE_OPEN:
        if (!player_in_cylinder(&work->pos, REACH, HEIGHT)) {
            work->mode = GATE_CLOSING;
        }
        break;
    case GATE_CLOSING:
        slide = VARS(work)->slide;
        slide -= speed;
        if (slide <= 0.0F) {
            slide = 0.0F;
            work->mode = GATE_SHUT;
        }
        edge = slide + DOOR_HALF;
        work->collision->shapes[0].center.z = edge;
        work->collision->shapes[1].center.z = -edge;
        VARS(work)->slide = slide;
        sound_play_for(SE_GATE_SLIDE, (int)task, 1, 0, 2);
        break;
    /* @hack redundant: a case for GATE_STILL, which nothing sets, whose CMP/EQ
     * #5,R0 is at 0x0c901ce6; without it, the object's code is 32 bytes shorter
     * and 156 of 172 words differ */
    case GATE_STILL:
        break;
    }
    /* @hack spelling: task->work read again, MOV.L @(32,R13),R4 at 0x0c901e40
     * in the call's delay slot; through work, 1 of 172 words differ */
    collision_register(task->work);
    gate_1_draw(task);
}

static void gate_1_draw(struct task *task)
{
    struct task_work *work = task->work;
    Float slide = VARS(work)->slide;
    NJS_OBJECT *doors = &gate_1_object;
    NJS_OBJECT *left;
    NJS_OBJECT *right;
    Angle ang;

    if (frame_step == 0) {
        texture_set_stage();
        njPushMatrix(NULL);
        njTranslateV(NULL, &work->pos);
        TURN_VIA(njRotateY, work->ang.y, ang);
        left = doors->child;
        right = left->sibling;
        njTranslate(NULL, 0.0F, 0.0F, slide);
        draw_model(right->model);
        njTranslate(NULL, 0.0F, 0.0F, -(slide * 2.0F));
        draw_model(left->model);
        njPopMatrix(1);
    }
}

static void gate_1_destroy(struct task *task)
{
}
