/* O SHUTTER shutter in act 0 that rises out of the way */
#include <ninja.h>
#include <sg_xpt.h>

#include "collision/collision.h"
#include "debug/editor.h"
#include "draw/tree.h"
#include "object/placement.h"
#include "player/player.h"
#include "sound/sound.h"
#include "stage/stage.h"
#include "stg03/shutter.h"
#include "system/frame.h"
#include "system/task.h"
#include "system/texture.h"
#include "system/util.h"

extern NJS_OBJECT twinkle_shutter_object;

/* the shutter's range for placement_try_leave_by, squared */
#define SHUTTER_RANGE (810.0F * 810.0F)
#define SHUTTER_ACT STAGE_ACT(STAGE_TWINKLE, 0)

/* the sound of the shutter rising. each step that lifts the shutter refreshes
 * the sound */
#define SE_SHUTTER 88

/* the reach of the shutter's collision record */
#define SHUTTER_REACH 102.8F
/* a shut shutter starts to rise once player 0 is within this distance */
#define SHUTTER_NEAR 80.0F
/* a rising shutter waits SHUTTER_WAIT + 1 steps. it then rises SHUTTER_SPEED
 * every second step, to SHUTTER_RISE above where it started */
#define SHUTTER_RISE 64.6F
#define SHUTTER_SPEED 0.5F
#define SHUTTER_WAIT 120

/* the face, the meshset whose texture scrolls, and its uvs */
#define FACE_MESHSET 3
#define FACE_UVS 16
/* the two v's the face's uvs are set to after a draw */
#define FACE_V_HIGH 255
#define FACE_V_LOW (-765)
/* the face's scroll a step, in v. a scroll that reaches SCROLL_END starts
 * again at 0. scrolling back, one that reaches 0 starts again at SCROLL_END */
#define SCROLL_SPEED 16
#define SCROLL_END 0x400

/* the face's material, and its textures. a rising shutter draws the face with
 * FACE_RISING_TEXTURE, then sets FACE_STILL_TEXTURE back */
#define FACE_MATERIAL 5
#define FACE_RISING_TEXTURE 94
#define FACE_STILL_TEXTURE 92

struct shutter_vars {
    NJS_OBJECT *object;
    Uint8 unk_04[4];
    Float top; /* the height it rises to */
};

#define VARS(work) ((struct shutter_vars *)&(work)->vars)
/* the face's scroll in v, unsigned */
#define SHUTTER_SCROLL(work) TASK_TIMER(work)
/* the steps a rising shutter has waited */
#define SHUTTER_WAITED(work) ((work)->tick)

/* shutter modes. a set entry whose scale x is below 0 starts in SHUTTER_HELD */
enum shutter_mode {
    SHUTTER_INIT,
    SHUTTER_SHUT, /* until player 0 comes within SHUTTER_NEAR units or rides */
    SHUTTER_HELD, /* until player 0 rides */
    SHUTTER_RISING
};

static struct collision_shape twinkle_shutter_shape[1] = {
    {SHAPE_RECTANGLE,
     0,
     PUSH_FIRM(PUSH_ALL) | PUSH_ALL,
     {0.0F, 32.28F, 0.0F},
     80.0F,
     32.28F,
     1.9F,
     {0},
     0,
     NULL},
};

static void twinkle_shutter_destroy(struct task *task)
{
    task_kill(task);
}

/* on a frame's first step, draw the shutter offset by work->scl along the
 * world's axes. init zeroes scl.y and scl.z, so outside the set editor only
 * scale x offsets it */
static void twinkle_shutter_draw(struct task *task)
{
    struct task_work *work = task->work;
    Angle ang;

    if (frame_step == 0) {
        texture_set_stage();
        njPushMatrix(NULL);
        njTranslateV(NULL, &work->scl);
        njTranslateV(NULL, &work->pos);
        TURN_VIA(njRotateY, work->ang.y, ang);
        draw_tree(VARS(work)->object);
        njPopMatrix(1);
    }
}

/* draw the shutter with its face scrolled by SHUTTER_SCROLL. uvs 0 to 7
 * scroll one way, and uvs 8 to 15 the other. then set each uv's v by its
 * index */
static void twinkle_shutter_draw_face(struct task *task)
{
    struct task_work *work = task->work;
    /* @hack type: i a Uint16, which SHC zero-extends at each test and index
     * (EXTU.W R5,R3 at 0x0c9073ae); as an int, 151 of 176 words differ */
    Uint16 i;
    Sint16 rest;

    for (i = 0; i < FACE_UVS; i++) {
        if (i < FACE_UVS / 2) {
            VARS(work)->object->model->meshsets[FACE_MESHSET].vertuv[i].tex.v -=
                SHUTTER_SCROLL(work);
        } else {
            VARS(work)->object->model->meshsets[FACE_MESHSET].vertuv[i].tex.v +=
                SHUTTER_SCROLL(work);
        }
    }
    twinkle_shutter_draw(task);
    for (i = 0; i < FACE_UVS; i++) {
        /* @bug logic: the first draw trades uvs 8 to 15's high and low v's
         * for good. uvs 0, 1, 4 and 5 of each eight get FACE_V_HIGH, and the
         * rest FACE_V_LOW, by the switch's table at 0x0c907508. the model
         * holds that pattern at uvs 0 to 7, but the reverse at uvs 8 to 15
         * (0x0c9bb350) */
        switch (i) {
        case 0:
        case 1:
        case 4:
        case 5:
        case 8:
        case 9:
        case 12:
        case 13:
            rest = FACE_V_HIGH;
            break;
        case 2:
        case 3:
        case 6:
        case 7:
        case 10:
        case 11:
        case 14:
        case 15:
            rest = FACE_V_LOW;
            break;
        }
        VARS(work)->object->model->meshsets[FACE_MESHSET].vertuv[i].tex.v =
            rest;
    }
}

/* scroll the face until player 0 comes near or rides. the shutter then waits,
 * and rises SHUTTER_RISE with its face scrolling back */
void twinkle_shutter_step(struct task *task)
{
    struct task_work *work = task->work;
    NJS_OBJECT *object;

    if (placement_try_leave_by(task, SHUTTER_RANGE)) {
        return;
    }
    if (stage_get_stage_act() != SHUTTER_ACT) {
        return;
    }
    object = &twinkle_shutter_object;
    if (edit_mode) {
        VARS(work)->object = object;
        twinkle_shutter_draw_face(task);
    } else {
        switch (work->mode) {
        case SHUTTER_INIT:
            if (work->scl.x < 0.0F) {
                work->mode = SHUTTER_HELD;
            } else {
                work->mode = SHUTTER_SHUT;
            }
            work->scl.y = work->scl.z = 0.0F;
            VARS(work)->top = work->pos.y + SHUTTER_RISE;
            VARS(work)->object = object;
            task->destructor = twinkle_shutter_destroy;
            task->draw = twinkle_shutter_draw_face;
            collision_init(task, twinkle_shutter_shape, COLLISION_OBJECT, 1, 1);
            task->work->collision->radius = SHUTTER_REACH;
            break;
        case SHUTTER_SHUT:
            if (njDistanceP2P(&work->pos, &player_work[0]->pos) <
                SHUTTER_NEAR) {
                work->mode = SHUTTER_RISING;
            }
            /* falls through: a shut shutter scrolls and draws as a held one
             * does. it also rises when player 0 rides */
        case SHUTTER_HELD:
            if (player_work[0]->submode == PLAYER_MODE_RIDE) {
                work->mode = SHUTTER_RISING;
            }
            if ((SHUTTER_SCROLL(work) += SCROLL_SPEED) >= SCROLL_END) {
                SHUTTER_SCROLL(work) = 0;
            }
            twinkle_shutter_draw_face(task);
            collision_register(work);
            break;
        case SHUTTER_RISING:
            if (SHUTTER_WAITED(work)++ > SHUTTER_WAIT) {
                SHUTTER_WAITED(work) = SHUTTER_WAIT;
                if ((work->pos.y += SHUTTER_SPEED) > VARS(work)->top) {
                    work->pos.y = VARS(work)->top;
                } else {
                    sound_play_at_for(SE_SHUTTER, (int)work, 1, 0, work->pos.x,
                                      work->pos.y, work->pos.z, 2);
                }
            }
            /* @hack spelling: <= 0, which for the unsigned scroll is == 0,
             * keeps the image's CMP/PL R0 at 0x0c907726. as == 0, 2 of 195
             * words differ */
            if ((SHUTTER_SCROLL(work) -= SCROLL_SPEED) <= 0) {
                SHUTTER_SCROLL(work) = SCROLL_END;
            }
            collision_register(work);
            VARS(work)->object->model->mats[FACE_MATERIAL].attr_texId =
                FACE_RISING_TEXTURE;
            twinkle_shutter_draw_face(task);
            VARS(work)->object->model->mats[FACE_MATERIAL].attr_texId =
                FACE_STILL_TEXTURE;
            break;
        }
    }
}
