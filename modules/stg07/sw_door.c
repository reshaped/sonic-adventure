/* the switch door: O SW DOOR, which places its three switches and rises
 * once all are pressed. after each press, the camera shows the door while a
 * panel lights the switch's lamp */
#include <ninja.h>
#include <sg_xpt.h>

#include "camera/camera.h"
#include "collision/collision.h"
#include "debug/editor.h"
#include "draw/model.h"
#include "draw/tree.h"
#include "object/placement.h"
#include "sound/sound.h"
#include "stg07/obj_ruin_textures.h"
#include "stg07/sw_door.h"
#include "system/pad.h"
#include "system/task.h"
#include "system/util.h"

extern NJS_OBJECT sw_door_x_object;
extern NJS_OBJECT sw_door_panel_object;
/* each switch's base, whose child is its button */
extern NJS_OBJECT sw_door_switch_1_object;
extern NJS_OBJECT sw_door_switch_2_object;
extern NJS_OBJECT sw_door_switch_3_object;

struct sw_door_vars {
    Float lift; /* how far the door has risen */
    Float shake; /* its offset along its z axis this step, as it shakes */
};

struct sw_door_switch_vars {
    Float sink; /* how far its button has gone down, below 0 */
};

#define VARS(work) ((struct sw_door_vars *)&(work)->vars)
#define SWITCH(work) ((struct sw_door_switch_vars *)&(work)->vars)

/* a switch's index into the tables below, in its work's tick */
#define SWITCH_INDEX(work) ((work)->tick)

/* the door's mode. nothing sets DOOR_RESET. the door's submode holds bit n
 * once switch n's lamp is lit */
enum sw_door_mode {
    DOOR_SHUT = 0,
    DOOR_RISE = 4,
    DOOR_RESET = 5,
    DOOR_OPEN = 6
};

/* a switch's mode. only the door's DOOR_RESET sets SWITCH_RESET */
enum sw_door_switch_mode {
    SWITCH_UP = 0,
    SWITCH_SINK = 1,
    SWITCH_SHOW = 2,
    SWITCH_SHOWN = 3,
    SWITCH_RESET = 5,
    SWITCH_DOWN = 6
};

#define SWITCHES 3

/* the door's step does nothing while the camera is more than 1000 away */
#define DOOR_RANGE SQUARE(1000.0F)

/* the door rises DOOR_SPEED each step, up to DOOR_HEIGHT */
#define DOOR_HEIGHT 44.0F
#define DOOR_SPEED 0.2F

/* as it rises, the door shakes SHAKE_SOFT aside. it shakes SHAKE_HARD
 * while its timer is a multiple of SHAKE_HARD_EVERY. the offset cycles
 * through 0, the shake and its negative */
#define SHAKE_SOFT 0.5F
#define SHAKE_HARD 2.5F
#define SHAKE_HARD_EVERY 17

/* its shape's middle, half the door's height above its foot */
#define SHAPE_HALF 22.0F

/* a switch's button sinks SWITCH_SPEED each step, down to SWITCH_DEPTH */
#define SWITCH_DEPTH (-1.7F)
#define SWITCH_SPEED 0.2F

/* a switch's shape follows its button. as the button sinks, the shape's
 * center and half height are SWITCH_MIDDLE plus half the sink. a reset sets
 * both to SWITCH_MIDDLE */
#define SWITCH_MIDDLE 2.0F

/* a switch's work flag. the collision checks set it while another record is on
 * top of the switch's */
#define SWITCH_PRESSED COLLISION_UNDER

/* after a press, the camera shows the door for SHOW_STEPS steps. the lamp
 * lights once SHOW_LAMP steps are left. with two lamps lit already, the
 * count then restarts from SHOW_LAST_STEPS, which covers the door's rise */
#define SHOW_STEPS 100
#define SHOW_LAMP 50
#define SHOW_LAST_STEPS 230

/* the camera's place and target while it shows the door */
#define SHOW_CAMERA_X 5861.0F
#define SHOW_CAMERA_Y (-2436.0F)
#define SHOW_CAMERA_Z 1327.0F
#define SHOW_TARGET_X 6004.0F
#define SHOW_TARGET_Y (-2426.0F)
#define SHOW_TARGET_Z 1332.0F

/* the panel's place, and its turn about y */
#define PANEL_X 6040.0F
#define PANEL_Y (-2413.0F)
#define PANEL_Z 1335.0F
#define PANEL_ANG NJM_DEG_ANG(180)

#define SE_LAMP 6
#define SE_PRESS 200
/* the door's sound as it rises */
#define SE_DOOR 194

/* the steps left to show the door */
static Sint32 sw_door_show_steps = 0;
static NJS_POINT3 sw_door_switch_places[SWITCHES] = {
    {5375.5F, -2610.0F, 1644.0F},
    {5194.0F, -2480.0F, 1335.0F},
    {5410.0F, -2545.0F, 1016.0F},
};
static Angle sw_door_switch_angles[SWITCHES] = {6000, 0, 2000};
static NJS_OBJECT *sw_door_switch_objects[SWITCHES] = {
    &sw_door_switch_1_object,
    &sw_door_switch_2_object,
    &sw_door_switch_3_object,
};
/* the textures of each switch's two materials, unlit in sw_door_dark_textures
 * and lit in sw_door_lit_textures. its button's materials are 1 and 2, and its
 * lamp's are its pair in sw_door_lamps. the button goes dark when the switch is
 * pressed. the lamp lights partway through the show that follows */
static Sint32 sw_door_dark_textures[SWITCHES][2] = {{4, 3}, {5, 2}, {6, 1}};
static Sint32 sw_door_lit_textures[SWITCHES][2] = {{9, 10}, {11, 12}, {13, 14}};
/* the panel's materials that make each switch's lamp */
static Sint32 sw_door_lamps[SWITCHES][2] = {{5, 4}, {6, 3}, {7, 2}};
/* clang-format off */
static struct collision_shape sw_door_switch_shape = {
    SHAPE_RECTANGLE, 0, PUSH_ALL | PUSH_FIRM(PUSH_ALL), {0.0F, 2.0F, 0.0F},
    8.0F, 3.0F, 8.0F, {0}, 0, NULL,
};
static struct collision_shape sw_door_shape = {
    SHAPE_RECTANGLE, 0, PUSH_ALL | PUSH_FIRM(PUSH_ALL), {0.0F, 22.0F, 0.0F},
    2.0F, 22.0F, 26.0F, {0}, 0, NULL,
};
/* clang-format on */

static struct task *sw_door_switches[SWITCHES];

static void sw_door_run(struct task *task);
static void sw_door_draw(struct task *task);
static void sw_door_destroy(struct task *task);
static void sw_door_switch_step(struct task *task);
static void sw_door_switch_run(struct task *task);
static void sw_door_switch_draw(struct task *task);

/* darken the panel's lamps and light the switches. outside the set editor,
 * create the switches as the door's children */
void sw_door_step(struct task *task)
{
    struct task_work *work = task->work;
    NJS_MATERIAL *mats;
    int i;

    work->timer = 0;
    work->submode = 0;
    work->mode = DOOR_SHUT;
    VARS(work)->lift = 0.0F;
    VARS(work)->shake = 0.0F;
    mats = sw_door_panel_object.model->mats;
    for (i = 0; i < SWITCHES; i++) {
        mats[sw_door_lamps[i][0]].attr_texId = sw_door_dark_textures[i][0];
        mats[sw_door_lamps[i][1]].attr_texId = sw_door_dark_textures[i][1];
        mats[sw_door_lamps[i][0]].attrflags &= ~NJD_FLAG_IGNORE_LIGHT;
        mats[sw_door_lamps[i][1]].attrflags &= ~NJD_FLAG_IGNORE_LIGHT;
    }
    for (i = 0; i < SWITCHES; i++) {
        mats = sw_door_switch_objects[i]->child->model->mats;
        mats[1].attr_texId = sw_door_lit_textures[i][0];
        mats[2].attr_texId = sw_door_lit_textures[i][1];
        mats[1].attrflags |= NJD_FLAG_IGNORE_LIGHT;
        mats[2].attrflags |= NJD_FLAG_IGNORE_LIGHT;
    }
    if (!placement_editing(task)) {
        for (i = 0; i < SWITCHES; i++) {
            sw_door_switches[i] =
                task_create_child(TASK_WORK, sw_door_switch_step, task);
            work = sw_door_switches[i]->work;
            SWITCH_INDEX(work) = i;
            work->pos = sw_door_switch_places[i];
            work->ang.x = 0;
            work->ang.y = sw_door_switch_angles[i];
            work->ang.z = 0;
        }
        collision_init(task, &sw_door_shape, COLLISION_OBJECT, 1, 1);
    }
    task->step = sw_door_run;
    task->destructor = sw_door_destroy;
    task->draw = sw_door_draw;
}

/* the door's step. it lights a switch's lamp once the switch reaches
 * SWITCH_SHOWN. with all three lamps lit, the door rises, shaking */
static void sw_door_run(struct task *task)
{
    struct task_work *work = task->work;
    int first;
    int second;
    NJS_OBJECT *panel;
    NJS_MATERIAL *mats;
    Float lift;
    Float shake;
    int i;
    int bit;

    if (placement_far_from_camera_by(task, DOOR_RANGE)) {
        return;
    }
    if (!placement_editing(task)) {
        switch (work->mode) {
        case DOOR_SHUT:
            bit = 1;
            i = 0;
            panel = &sw_door_panel_object;
            /* @hack redundant: three reads of the panel that SHC drops, which
             * weigh panel enough to keep it in R8 and i at @(4,R15)
             * (0x0c904cc0, 0x0c904cca). without them, 115 of sw_door_run's
             * 270 words differ */
            (void)panel->model;
            (void)panel->child;
            (void)panel->sibling;
            do {
                if (!(work->submode & bit) &&
                    sw_door_switches[i]->work->mode == SWITCH_SHOWN) {
                    work->submode |= bit;
                    mats = panel->model->mats;
                    first = sw_door_lamps[i][0];
                    second = sw_door_lamps[i][1];
                    mats[first].attr_texId = sw_door_lit_textures[i][0];
                    mats[second].attr_texId = sw_door_lit_textures[i][1];
                    mats[first].attrflags |= NJD_FLAG_IGNORE_LIGHT;
                    mats[second].attrflags |= NJD_FLAG_IGNORE_LIGHT;
                    sound_play(SE_LAMP, 0, 0, 0);
                }
                bit <<= 1;
            } while (++i < SWITCHES);
            /* @hack redundant: two tests of bit that never run, which weigh it
             * enough to take R11, the image's register for it, with SHC's
             * pointer into sw_door_switches in R9 and 0x02000000 in R10
             * (0x0c904cc4 to 0x0c904cc8); without them, the three rotate and
             * 10 of 270 words differ */
            if (0) {
                if (bit) {
                }
                if (bit) {
                }
            }
            if ((work->submode & 1) && (work->submode & 2) &&
                (work->submode & 4)) {
                work->mode = DOOR_RISE;
                work->timer = 0;
                VARS(work)->lift = 0.0F;
                sound_play_held(SE_DOOR, (int)work, 1, 0);
            }
            break;
        case DOOR_RISE:
            lift = VARS(work)->lift;
            lift += DOOR_SPEED;
            if (lift >= DOOR_HEIGHT) {
                work->collision->shapes->center.y = DOOR_HEIGHT + SHAPE_HALF;
                lift = DOOR_HEIGHT;
                work->timer = 0;
                work->mode = DOOR_OPEN;
                sound_stop_id((int)work);
            }
            work->collision->shapes->center.y = lift + SHAPE_HALF;
            if (TASK_TIMER(work) % SHAKE_HARD_EVERY == 0) {
                shake = SHAKE_HARD;
            } else {
                shake = SHAKE_SOFT;
            }
            switch (TASK_TIMER(work) % 3) {
            case 0:
                VARS(work)->shake = 0.0F;
                break;
            case 1:
                VARS(work)->shake = shake;
                break;
            case 2:
                VARS(work)->shake = -shake;
                break;
            }
            work->timer++;
            VARS(work)->lift = lift;
            break;
        case DOOR_RESET:
            /* @hack redundant: a goto to the next statement, which SHC deletes.
             * the loop then loads each switch's task into R3 and its work
             * into R2 (0x0c904e2e, 0x0c904e34). without it, the two trade
             * and 11 of sw_door_run's 270 words differ */
            goto reset;
        reset:
            for (i = 0; i < SWITCHES; i++) {
                sw_door_switches[i]->work->mode = SWITCH_RESET;
            }
            work->mode = DOOR_SHUT;
            VARS(work)->lift = 0.0F;
            VARS(work)->shake = 0.0F;
            work->collision->shapes->center.y = SHAPE_HALF;
            break;
        case DOOR_OPEN:
            break;
        }
        task_step_children(task);
        collision_register(work);
    }
    sw_door_draw(task);
}

static void sw_door_draw(struct task *task)
{
    struct task_work *work = task->work;
    /* @hack spelling: a cursor over pos, which SHC reads with FMOV.S @R4+,FR3
     * (0x0c904ef4 to 0x0c904f00). read by member, 71 of sw_door_draw's 91
     * words differ */
    Float *at = &work->pos.x;
    Float x = *at++;
    Float y = *at++;
    Float z = *at;
    Float lift = VARS(work)->lift;
    Float shake = VARS(work)->shake;
    Angle ang;

    njSetTexture(&ruin_door_texlist);
    njPushMatrix(NULL);
    njTranslate(NULL, x, y + lift, z);
    TURN_VIA(njRotateY, work->ang.y, ang);
    if (shake != 0.0F) {
        njTranslate(NULL, 0.0F, 0.0F, shake);
    }
    draw_tree(&sw_door_x_object);
    njPopMatrix(1);
    njPushMatrix(NULL);
    njTranslate(NULL, PANEL_X, PANEL_Y, PANEL_Z);
    njRotateY(NULL, PANEL_ANG);
    draw_tree(&sw_door_panel_object);
    njPopMatrix(1);
}

static void sw_door_destroy(struct task *task)
{
    task_destroy_children(task);
}

static void sw_door_switch_step(struct task *task)
{
    struct task_work *work = task->work;

    work->mode = SWITCH_UP;
    SWITCH(work)->sink = 0.0F;
    collision_init(task, &sw_door_switch_shape, COLLISION_OBJECT, 1, 1);
    task->step = sw_door_switch_run;
    task->draw = sw_door_switch_draw;
}

/* a switch's step. a press darkens the switch and disables player 0's pad
 * while the camera shows the door. the button then sinks */
static void sw_door_switch_run(struct task *task)
{
    struct task_work *work = task->work;
    NJS_MATERIAL *mats;
    int index;
    Sint8 lit;

    switch (work->mode) {
    case SWITCH_UP:
        if (work->flags & SWITCH_PRESSED) {
            work->flags &= ~SWITCH_PRESSED;
            sound_play(SE_PRESS, 0, 0, 0);
            work->mode = SWITCH_SHOW;
            camera_work->scl.x = SHOW_CAMERA_X;
            camera_work->scl.y = SHOW_CAMERA_Y;
            camera_work->scl.z = SHOW_CAMERA_Z;
            CAMERA_VARS(camera_work)->target.x = SHOW_TARGET_X;
            CAMERA_VARS(camera_work)->target.y = SHOW_TARGET_Y;
            CAMERA_VARS(camera_work)->target.z = SHOW_TARGET_Z;
            camera_override_mode(CAMERA_MODE_RUIN_WAKA1, 0);
            sw_door_show_steps = SHOW_STEPS;
            pad_disable_player(0);
            index = SWITCH_INDEX(work);
            mats = sw_door_switch_objects[index]->child->model->mats;
            mats[1].attr_texId = sw_door_dark_textures[index][0];
            mats[2].attr_texId = sw_door_dark_textures[index][1];
            mats[1].attrflags &= ~NJD_FLAG_IGNORE_LIGHT;
            mats[2].attrflags &= ~NJD_FLAG_IGNORE_LIGHT;
        }
        break;
    case SWITCH_SHOW:
        if (sw_door_show_steps-- == SHOW_LAMP) {
            lit = task->parent->work->submode;
            if (((lit & 1) && (lit & 2)) || ((lit & 2) && (lit & 4)) ||
                ((lit & 1) && (lit & 4))) {
                sw_door_show_steps = SHOW_LAST_STEPS;
            }
            work->mode = SWITCH_SHOWN;
        }
        break;
    case SWITCH_SHOWN:
        if (sw_door_show_steps-- == 0) {
            pad_enable_player(0);
            camera_restore();
            camera_set_adjust(0);
            work->mode = SWITCH_SINK;
        }
        break;
    case SWITCH_SINK:
        SWITCH(work)->sink -= SWITCH_SPEED;
        if (SWITCH(work)->sink <= SWITCH_DEPTH) {
            SWITCH(work)->sink = SWITCH_DEPTH;
            work->mode = SWITCH_DOWN;
        }
        work->collision->shapes->center.y = work->collision->shapes->size_y =
            (SWITCH(work)->sink * 0.5F) + SWITCH_MIDDLE;
        break;
    case SWITCH_DOWN:
        break;
    case SWITCH_RESET:
        work->mode = SWITCH_UP;
        SWITCH(work)->sink = 0.0F;
        work->collision->shapes->center.y = work->collision->shapes->size_y =
            SWITCH_MIDDLE;
        break;
    }
    collision_register(work);
    sw_door_switch_draw(task);
}

static void sw_door_switch_draw(struct task *task)
{
    struct task_work *work = task->work;
    NJS_OBJECT *object;
    Angle ang;

    njSetTexture(&ruin_door_texlist);
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    TURN_VIA(njRotateY, work->ang.y, ang);
    object = sw_door_switch_objects[SWITCH_INDEX(work)];
    draw_model(object->model);
    object = object->child;
    njTranslate(NULL, object->pos[0], object->pos[1], object->pos[2]);
    if (SWITCH(work)->sink != 0.0F) {
        njTranslate(NULL, 0.0F, SWITCH(work)->sink, 0.0F);
    }
    draw_model(object->model);
    njPopMatrix(1);
}
