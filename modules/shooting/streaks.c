/* the sky chase's streaks: wisps of cloud that rush at the camera. each
 * step starts a new one by a chance of one in five. it starts far ahead, at
 * random across the way */
#include <ninja.h>
#include <sg_xpt.h>
#include <stdlib.h>

#include "camera/camera.h"
#include "draw/tree.h"
#include "player/player.h"
#include "shooting/act1_textures.h"
#include "shooting/act2_textures.h"
#include "shooting/shooting.h"
#include "shooting/streaks.h"
#include "system/frame.h"
#include "system/task.h"
#include "system/util.h"

#define TEXT_1_LENGTH 10
#define TEXT_2_LENGTH 10
#define TEXT_3_LENGTH 10
#define TEXT_4_LENGTH 12
#define TEXT_5_LENGTH 12
#define TEXT_6_LENGTH 12
#define TEXT_7_LENGTH 10
#define TEXT_8_LENGTH 9
#define TEXT_9_LENGTH 15
#include "shooting/streaks/text.inc"

/* a streak's work->vars. its work->scl.x is its size, and its scl.z its
 * speed */
struct streak_vars {
    Uint16 steps; /* left to live */
    Uint16 kind; /* which of the act's wisps */
    Uint8 unk_04[4];
    /* from -1 up to 0 by 0.01 a step, which no draw reads */
    Float fade;
};

#define VARS(work) ((struct streak_vars *)&(work)->vars)

enum streaks_mode { STREAKS_INIT, STREAKS_RUN };

/* a step starts a streak STREAK_CHANCE times in 100 */
#define STREAK_CHANCE 20.0F
/* a streak starts STREAK_AHEAD ahead, and up to STREAK_SPREAD off in x and
 * y. it lives the steps it takes to travel STREAK_REACH, and 10 more */
#define STREAK_AHEAD 2000.0F
#define STREAK_SPREAD 1000.0F
#define STREAK_REACH 1000.0F

extern NJS_OBJECT act1_wisp_0_object;
extern NJS_OBJECT act1_wisp_1_object;
extern NJS_OBJECT act1_wisp_2_object;
extern NJS_OBJECT act2_wisp_0_object;
extern NJS_OBJECT act2_wisp_1_object;
extern NJS_OBJECT act2_wisp_2_object;

static void streaks_spawn(struct task *task);
static void streak_step(struct task *task);
static void streak_draw(struct task *task);
static void streak_destroy(struct task *task);

static NJS_OBJECT *streak_models[2][3] = {
    {&act1_wisp_0_object, &act1_wisp_1_object, &act1_wisp_2_object},
    {&act2_wisp_0_object, &act2_wisp_1_object, &act2_wisp_2_object},
};
/* the wisps' texture names, texture list and texanim, which no code reads */
static NJS_TEXNAME wisp_textures[9] = {
    {TEXT_1, 0, 0}, {TEXT_2, 0, 0}, {TEXT_3, 0, 0},
    {TEXT_4, 0, 0}, {TEXT_5, 0, 0}, {TEXT_6, 0, 0},
    {TEXT_7, 0, 0}, {TEXT_8, 0, 0}, {TEXT_9, 0, 0},
};
static NJS_TEXLIST wisp_texlist = {
#include "shooting/streaks/wisp_texlist.inc"
};
static NJS_TEXANIM wisp_frame[1] = {
#include "shooting/streaks/wisp_frame.inc"
};

void shooting_streaks_step(struct task *task)
{
    struct task_work *work = task->work;

    switch (work->mode) {
    case STREAKS_INIT:
        work->mode = STREAKS_RUN;
        work->vars.f[0] = 0.0F; /* nothing reads it */
        task->destructor = streak_destroy;
        break;
    case STREAKS_RUN:
        streaks_spawn(task);
    }
    task_step_children(task);
}

/* start a streak at random, with a random size and speed */
static void streaks_spawn(struct task *task)
{
    struct task *child;
    struct task_work *work;
    Float steps;

    if (rand() * (100.0F * RAND_SCALE) < STREAK_CHANCE) {
        /* @hack spelling: child assigned in the test, which tests
         * task_create_child's R0 with the copy to R12 in the BT/S's slot
         * (0x0c9089c0, 0x0c9089c4); assigned apart, SHC tests R12 and 3 of
         * 109 words differ */
        if ((child = task_create_child(TASK_WORK, streak_step, task)) != NULL) {
            work = child->work;
            work->pos.x = RANDOM(-STREAK_SPREAD, STREAK_SPREAD);
            work->pos.y = RANDOM(-STREAK_SPREAD, STREAK_SPREAD);
            work->pos.z = STREAK_AHEAD;
            work->scl.x = (rand() * RAND_SCALE) + 0.1F;
            work->scl.z = RANDOM(30.0F, 80.0F);
            work->ang.x = 0xc000;
            work->ang.y = 0;
            work->ang.z = 0;
            steps = (STREAK_REACH / work->scl.z) + 10.0F;
            VARS(work)->steps = steps;
            VARS(work)->kind = rand() * (3.0F * RAND_SCALE);
            VARS(work)->fade = -1.0F;
            child->draw = streak_draw;
            child->destructor = streak_destroy;
        }
    }
}

static void streak_step(struct task *task)
{
    struct task_work *work = task->work;

    if (--VARS(work)->steps > 0) {
        work->pos.z -= work->scl.z;
        streak_draw(task);
        if (VARS(work)->fade < 0.0F) {
            VARS(work)->fade += 0.01F;
        } else {
            VARS(work)->fade = 0.0F;
        }
    } else {
        task_destroy(task);
    }
}

/* draw the streak at its place from the camera, turned as player 0 is.
 * while an event or the score plays, its place is from player 0 instead. it
 * draws at a frame's first step only */
static void streak_draw(struct task *task)
{
    struct task_work *work = task->work;
    struct task_work *player = player_get_task(0)->work;
    Float size;

    if (frame_step == 0) {
        njPushMatrix(NULL);
        if (shooting_event_playing()) {
            njTranslate(NULL, player->pos.x, player->pos.y, player->pos.z);
        } else {
            njTranslate(NULL, camera_work->pos.x, camera_work->pos.y,
                        camera_work->pos.z);
        }
        njRotateXYZ(NULL, player->ang.x, player->ang.y, player->ang.z);
        njTranslate(NULL, work->pos.x, work->pos.y, work->pos.z);
        njRotateXYZ(NULL, work->ang.x, work->ang.y, work->ang.z);
        size = work->scl.x;
        njScale(NULL, size, size, size);
        switch (shooting_act) {
        case 0:
            njSetTexture(&shooting1_texlist);
            break;
        case 1:
            njSetTexture(&shooting2_texlist);
            break;
        }
        draw_tree(streak_models[shooting_act][VARS(work)->kind]);
        njPopMatrix(1);
    }
}

static void streak_destroy(struct task *task)
{
}
