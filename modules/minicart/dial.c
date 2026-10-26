/* the dial, DIAL, whose four lamps and two faces show what cart_race_dial
 * gives. it draws dim while that is 0, before the first light or without
 * the race */
#include <ninja.h>
#include <sg_xpt.h>

#include "draw/state.h"
#include "minicart/dial.h"
#include "minicart/race.h"
#include "object/placement.h"
#include "system/frame.h"
#include "system/task.h"
#include "system/task_desc.h"
#include "system/texture.h"
#include "system/util.h"

#define DIAL_MATERIALS 6

/* the dial's textures, by their names in OBJ_MINI_CART */
#define TP1_24 0
#define MIN_LAMP2 1
#define MIN_LAMP3 2
#define MIN_LAMPK 3
#define MIN1 4
#define MIN2 5
#define MIN3 6
#define MIN4 7
#define MIN5 8
#define MIN6 9
#define MIN7 10
#define MIN8 11
#define MIN_LAMP 13

/* a dial's record, in its task's motion */
struct dial_record {
    Sint8 shown; /* what cart_race_dial gave at the last step */
    NJS_OBJECT *object;
};

/* one of the dial's materials, and its texture for each value
 * cart_race_dial gives */
struct dial_material {
    NJS_MATERIAL *material;
    Uint16 textures[DIAL_HOME_STRETCH + 1];
};

/* @hack type: a turn's ang and angle held in a struct in the frame, which
 * keeps dial_turn_x, dial_turn_y and dial_turn_z out of line (BSR at
 * 0x0c90babe, 0x0c90bac2 and 0x0c90bac6). as plain locals, SHC expands the
 * three in dial_draw, and 46 of their 48 words differ */
struct dial_turn {
    struct angles *ang;
    Angle angle;
};

extern NJS_MATERIAL dial_face_1;
extern NJS_MATERIAL dial_face_2;
extern NJS_MATERIAL dial_lamp_1;
extern NJS_MATERIAL dial_lamp_2;
extern NJS_MATERIAL dial_lamp_3;
extern NJS_MATERIAL dial_lamp_4;
extern NJS_OBJECT dial_object;

static void dial_init(struct task *task, void *arg);
static void dial_run(struct task *task);
static void dial_draw(struct task *task);
static void dial_show(Sint8 shown);

static const struct task_desc dial_desc = {sizeof(struct dial_record),
                                           TASK_WORK,
                                           dial_init,
                                           dial_run,
                                           dial_draw,
                                           NULL};
/* each material's texture, by what cart_race_dial gives */
static const struct dial_material dial_materials[DIAL_MATERIALS] = {
    {&dial_face_1, {MIN3, MIN3, MIN2, MIN1, MIN4, MIN5, MIN7, TP1_24}},
    {&dial_face_2, {MIN3, MIN3, MIN2, MIN1, MIN4, MIN6, MIN8, TP1_24}},
    {&dial_lamp_1,
     {MIN_LAMPK, MIN_LAMP, MIN_LAMP, MIN_LAMP, MIN_LAMP2, MIN_LAMPK, MIN_LAMPK,
      MIN_LAMP3}},
    {&dial_lamp_2,
     {MIN_LAMPK, MIN_LAMPK, MIN_LAMP, MIN_LAMP, MIN_LAMP2, MIN_LAMPK, MIN_LAMPK,
      MIN_LAMP3}},
    {&dial_lamp_3,
     {MIN_LAMPK, MIN_LAMPK, MIN_LAMPK, MIN_LAMP, MIN_LAMP2, MIN_LAMPK,
      MIN_LAMPK, MIN_LAMP3}},
    {&dial_lamp_4,
     {MIN_LAMPK, MIN_LAMPK, MIN_LAMPK, MIN_LAMPK, MIN_LAMP2, MIN_LAMPK,
      MIN_LAMPK, MIN_LAMP3}},
};
static NJS_OBJECT *const dial_model = &dial_object;
static const NJS_ARGB dial_dim = {1.0F, 0.25F, 0.25F, 0.25F};

static void dial_turn_x(struct angles *ang)
{
    struct dial_turn turn;
    /* @hack redundant: turn's address, stored and never read, which has
     * SHC address turn from R15 (MOV.L R4,@R15 at 0x0c90b9c6); without it,
     * SHC addresses turn through R14, the function runs 4 bytes longer, and
     * 16 of 16 words differ */
    void *frame = &turn;

    turn.ang = ang;
    /* @hack spelling: the angle read through turn.ang and stored in the test,
     * which copies ang to R3 and loads the angle through it (MOV R4,R3 at
     * 0x0c90b9c4); read through ang and stored before the test, 4 of 16
     * words differ */
    if ((turn.angle = turn.ang->x)) {
        njRotateX(NULL, turn.angle);
    }
}

static void dial_turn_y(struct angles *ang)
{
    struct dial_turn turn;
    /* @hack redundant: turn's address, as in dial_turn_x (MOV.L R4,@R15 at
     * 0x0c90b9e6); without it, 16 of 16 words differ */
    void *frame = &turn;

    turn.ang = ang;
    /* @hack spelling: the angle read as in dial_turn_x (MOV R4,R3 at
     * 0x0c90b9e4); read through ang and stored before the test, 4 of 16
     * words differ */
    if ((turn.angle = turn.ang->y)) {
        njRotateY(NULL, turn.angle);
    }
}

static void dial_turn_z(struct angles *ang)
{
    struct dial_turn turn;
    /* @hack redundant: turn's address, as in dial_turn_x (MOV.L R4,@R15 at
     * 0x0c90ba06); without it, 16 of 16 words differ */
    void *frame = &turn;

    turn.ang = ang;
    /* @hack spelling: the angle read as in dial_turn_x (MOV R4,R3 at
     * 0x0c90ba04); read through ang and stored before the test, 4 of 16
     * words differ */
    if ((turn.angle = turn.ang->z)) {
        njRotateZ(NULL, turn.angle);
    }
}

void dial_step(struct task *task)
{
    task_apply_desc(task, &dial_desc, (void *)&dial_model);
}

/* arg points at the pointer to the object the dial draws */
static void dial_init(struct task *task, void *arg)
{
    struct dial_record *record = (struct dial_record *)task->motion;

    record->object = *(NJS_OBJECT **)arg;
    record->shown = cart_race_dial();
}

static void dial_run(struct task *task)
{
    struct dial_record *record;

    if (!placement_try_leave(task)) {
        record = (struct dial_record *)task->motion;
        record->shown = cart_race_dial();
        dial_draw(task);
    }
}

/* draw on a frame's first step only, dim while the dial shows 0 */
static void dial_draw(struct task *task)
{
    struct dial_record *record;
    struct task_work *work;
    struct angles *ang;

    if (frame_step == 0) {
        work = task->work;
        record = (struct dial_record *)task->motion;
        dial_show(record->shown);
        draw_state_save();
        if (record->shown == 0) {
            draw_control_clear(NJD_CONTROL_3D_OFFSET_MATERIAL);
            draw_control_set(NJD_CONTROL_3D_CONSTANT_MATERIAL);
            njSetConstantMaterial((NJS_ARGB *)&dial_dim);
        }
        njPushMatrix(NULL);
        njTranslateV(NULL, &work->pos);
        ang = &work->ang;
        dial_turn_z(ang);
        dial_turn_x(ang);
        dial_turn_y(ang);
        texture_set_stage();
        njSimpleDrawObject(record->object);
        njPopMatrix(1);
        draw_state_restore();
    }
}

static void dial_show(Sint8 shown)
{
    /* @hack spelling: the bound through a local, which keeps SHC from
     * unrolling the loop into passes of three rows; with DIAL_MATERIALS in
     * the test, the function runs 32 bytes longer and 13 of 18 words
     * differ */
    int count = DIAL_MATERIALS;
    const struct dial_material *row;
    int i;

    for (i = 0, row = dial_materials; i < count; i++, row++) {
        row->material->attr_texId = row->textures[shown];
    }
}
