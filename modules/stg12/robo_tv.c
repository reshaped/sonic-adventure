/* O ROBO TV, a television that plays a picture once switch 4 is on */
#include <ninja.h>
#include <sg_xpt.h>
#include <stdlib.h>

#include "collision/collision.h"
#include "debug/editor.h"
#include "debug/print.h"
#include "draw/tree.h"
#include "object/placement.h"
#include "object/switch.h"
#include "sound/sound.h"
#include "stg12/robo_tv.h"
#include "stg12/texlists.h"
#include "system/frame.h"
#include "system/task.h"
#include "system/util.h"

extern NJS_OBJECT robo_tv_object;
extern NJS_OBJECT robo_tv_screen_object;
extern NJS_OBJECT robo_tv_dark_object;
extern NJS_OBJECT robo_tv_picture_object;

#define SE_ROBO_TV 324

/* the switch that starts the picture. O Box Ana's first hole turns it on
 * once its four box switches are on. the picture's end turns
 * SWITCH_CRASH_TV on, which breaks the crash TV */
#define SWITCH_ROBO_TV 4
#define SWITCH_CRASH_TV 5

/* destroyed beyond 500 units of the camera and player 0 */
#define RANGE (500.0F * 500.0F)

/* the steps the screen stays dark before the picture plays */
#define DARK_STEPS 60

/* the screen strip's UVs, one at each corner */
#define SCREEN_CORNERS 4

enum robo_tv_mode { TV_INIT, TV_STATIC, TV_DARK, TV_PICTURE, TV_DONE };

struct robo_tv_vars {
    Uint8 frame; /* the entry of robo_tv_rows[row] to show next */
    Uint8 row;
};

#define VARS(work) ((struct robo_tv_vars *)&(work)->vars)

static void robo_tv_init(struct task *task);
static void robo_tv_draw(struct task *task);
static void robo_tv_destroy(struct task *task);
static void robo_tv_play(struct task_work *work);
static void robo_tv_static(struct task_work *work);
static void robo_tv_wait_switch(struct task_work *work);

static struct collision_shape robo_tv_shape = {
    SHAPE_CYLINDER,
    0,
    PUSH_ALL | PUSH_FIRM(PUSH_ALL),
    {0.0F, 0.0F, 0.0F},
    17.0F,
    1.5F,
    0.0F,
    {0},
    0,
    NULL,
};

/* the picture's one row: a texture of robotv_texlist for each step, ending
 * at -1 */
static Sint32 robo_tv_frames[139] = {
#include "stg12/robo_tv/robo_tv_frames.inc"
};

/* a copy of robo_tv_frames' last 21 entries. robo_tv_rows doesn't name it,
 * so the picture never plays it */
static Sint32 robo_tv_frames_2[21] = {
#include "stg12/robo_tv/robo_tv_frames_2.inc"
};

/* the rows the picture plays in turn, ending at a null. once the last row
 * ends, the picture stops and turns SWITCH_CRASH_TV on */
static Sint32 *robo_tv_rows[] = {robo_tv_frames, NULL};

/* @hack spelling: collision_register through an inline function, which reads
 * task->work again at each call (MOV.L @(32,R4),R4 at 0x0c9059d6 and
 * 0x0c905a10). called with robo_tv_step's own work, the step saves R11 and
 * swaps task's and work's registers. then 102 of its 108 words and 2 of 36
 * other words differ */
static void robo_tv_register(struct task *task)
{
    struct task_work *work = task->work;

    collision_register(work);
}

void robo_tv_step(struct task *task)
{
    struct task_work *work = task->work;

    if (placement_try_leave_by(task, RANGE)) {
        return;
    }
    if (!placement_editing(task)) {
        switch (work->mode) {
        case TV_INIT:
            robo_tv_init(task);
            break;
        case TV_STATIC:
            robo_tv_static(work);
            robo_tv_draw(task);
            robo_tv_register(task);
            robo_tv_wait_switch(work);
            break;
        case TV_DARK:
            if (--TASK_TIMER(work) <= 0) {
                work->mode = TV_PICTURE;
            }
            robo_tv_draw(task);
            robo_tv_register(task);
            break;
        case TV_PICTURE:
            robo_tv_play(work);
            robo_tv_draw(task);
            robo_tv_register(task);
            break;
        case TV_DONE:
            robo_tv_static(work);
            robo_tv_draw(task);
            robo_tv_register(task);
            break;
        }
    } else {
        switch (work->mode) {
        case TV_INIT:
            robo_tv_init(task);
            break;
        default:
            debug_print(EDITOR_LABEL(EDITOR_ROW_ANG_X), "-> ROTATE X");
            debug_print(EDITOR_LABEL(EDITOR_ROW_ANG_Y), "-> ROTATE Y");
            debug_print(EDITOR_LABEL(EDITOR_ROW_ANG_Z), "-> ROTATE Z");
            robo_tv_draw(task);
            robo_tv_register(task);
            break;
        }
    }
}

static void robo_tv_init(struct task *task)
{
    struct task_work *work = task->work;

    task->draw = robo_tv_draw;
    task->destructor = robo_tv_destroy;
    collision_init(task, &robo_tv_shape, COLLISION_OBJECT, 1, 1);
    VARS(work)->frame = 0;
    VARS(work)->row = 0;
    work->mode = TV_STATIC;
}

static void robo_tv_draw(struct task *task)
{
    struct task_work *work = task->work;
    Angle ang;

    if (frame_step == 0) {
        njSetTexture(&hotshelter2_texlist);
        njPushMatrix(NULL);
        njTranslateV(NULL, &work->pos);
        TURN_VIA(njRotateY, work->ang.y, ang);
        switch (work->mode) {
        case TV_STATIC:
        case TV_DONE:
            draw_tree(&robo_tv_object);
            break;
        case TV_DARK:
        case TV_PICTURE:
            draw_tree(&robo_tv_dark_object);
            njSetTexture(&robotv_texlist);
            draw_tree(&robo_tv_picture_object);
            break;
        }
        njPopMatrix(1);
    }
}

static void robo_tv_destroy(struct task *task)
{
}

/* @hack type: the picture's row and frame read through work->vars.b, which
 * SHC reaches through a pointer to the two bytes (MOV R4,R6 and ADD #8,R6 at
 * 0x0c905b56 and 0x0c905b94); through VARS, it reads each at its offset from
 * work, robo_tv_play runs 4 bytes shorter, and 47 of 50 words differ */
#define TEXTURE(work)                                                          \
    (robo_tv_rows[(Uint8)(work)->vars.b[1]][(Uint8)(work)->vars.b[0]])

static void robo_tv_play(struct task_work *work)
{
    if (TEXTURE(work) == -1) {
        VARS(work)->frame = 0;
        VARS(work)->row++;
        if (robo_tv_rows[VARS(work)->row] == NULL) {
            work->mode = TV_DONE;
            switch_set(SWITCH_CRASH_TV, TRUE);
            return;
        }
    }
    robo_tv_picture_object.child->model->mats[0].attr_texId = TEXTURE(work);
    VARS(work)->frame++;
}

static void robo_tv_static(struct task_work *work)
{
    NJS_TEX corners[SCREEN_CORNERS] = {
#include "stg12/robo_tv/corners.inc"
    };
    NJS_COLOR *uv;
    int i;
    int drop;

    uv = robo_tv_screen_object.model->meshsets[0].vertuv;
    rand();
    drop = (int)(rand() * (256.0F * RAND_SCALE));
    for (i = 0; i < SCREEN_CORNERS; i++) {
        uv[i].tex.u = corners[i].u;
        uv[i].tex.v = corners[i].v + drop;
    }
}

static void robo_tv_wait_switch(struct task_work *work)
{
    if (switch_is_on(SWITCH_ROBO_TV)) {
        robo_tv_picture_object.child->model->mats[0].attr_texId = 0;
        work->mode = TV_DARK;
        TASK_TIMER(work) = DARK_STEPS;
        sound_play(SE_ROBO_TV, 0, 0, 0);
    }
}
