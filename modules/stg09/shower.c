/* O SHWR, a shower that holds player 0 and washes off the mud */
#include <ninja.h>
#include <sg_xpt.h>
#include <stdlib.h>

#include "debug/editor.h"
#include "draw/model.h"
#include "object/placement.h"
#include "player/player.h"
#include "sound/sound.h"
#include "stg09/casino.h"
#include "stg09/mud.h"
#include "stg09/shower.h"
#include "stg09/water_drop.h"
#include "system/frame.h"
#include "system/pad.h"
#include "system/task.h"
#include "system/util.h"

extern NJS_OBJECT shower_object;

/* placement_try_leave_by's squared range: about 250 from the camera or
 * player 0 */
#define RANGE 62510.0F

/* player 0 stands under the shower within this distance of its place */
#define UNDER_DISTANCE 4.0F

#define SE_STEP_IN 266

enum shower_mode { SHOWER_WAITING, SHOWER_DONE, SHOWER_HOLDING };

struct shower_vars {
    /* the shower pours while pour is above 0. outside a hold, pour rises by 2
     * a step while player 0 stands under the shower. it drops to 0 once he
     * steps away. during a hold, it rises by 1 a step */
    Uint16 pour;
};

#define VARS(work) ((struct shower_vars *)&(work)->vars)

/* the water's meshsets, and the UVs of the first and of the second */
#define WATER_MESHSETS 2
#define FIRST_UVS 12
#define SECOND_UVS 8

/* the spout, in the shower's frame. the drops start around it and fly
 * toward the shower's place */
#define SPOUT_X 0.125F
#define SPOUT_Y 22.5F
/* the image's 0xc0d19998, the float next nearer 0 than the 0xc0d19999 SHC
 * makes of -6.55F */
#define SPOUT_Z (-6.5499995F)

static int shower_slide = 40; /* the water's scroll in v a step, mod 256 */
static Float shower_speed = 1.3F; /* the drops' speed */
static Float shower_spread = 0.4F; /* water_drop_add's spread */
/* the drops start about 0.7 times this from the spout, in x and z */
static Float shower_radius = 0.8F;
static Float shower_wash = 0.002F; /* the mud level washed off a step */
static int shower_drops = 2; /* the drops a step */
/* the water shows once pour reaches this, 30 steps after player 0 steps
 * under the shower */
static int shower_wait = 60;
/* the hold starts once pour passes shower_wait by HOLD_POUR. at 2 a step,
 * that is 6 steps after the water shows */
#define HOLD_POUR 10
/* the hold ends once it has lasted more than this many steps */
static int shower_length = 500;

static void shower_init(struct task *task);
static void shower_destroy(struct task *task);
static void shower_run(struct task *task);
static void shower_draw(struct task *task);

void shower_step(struct task *task)
{
    /* @hack redundant: work, which nothing reads, keeps task in R14 across the
     * range check (MOV R4,R14 at 0x0c914a48). without it, SHC keeps task in
     * the frame, and 12 of 16 words differ */
    struct task_work *work = task->work;

    if (!placement_try_leave_by(task, RANGE)) {
        shower_init(task);
    }
}

static void shower_init(struct task *task)
{
    struct task_work *work = task->work;

    task->draw = shower_draw;
    task->step = shower_run;
    task->destructor = shower_destroy;
    work->timer = 0;
    VARS(work)->pour = 0;
    work->mode = SHOWER_WAITING;
}

static void shower_destroy(struct task *task)
{
    struct task_work *work = task->work;

    if (work->mode == SHOWER_HOLDING) {
        player_set_mode(0, PLAYER_MODE_SHOWER_END);
    }
}

/* the shower holds player 0 once: SHOWER_DONE never goes back to
 * SHOWER_WAITING. a jump button pressed while it pours ends the hold, or
 * keeps it from starting */
static void shower_run(struct task *task)
{
    struct task_work *work = task->work;
    NJS_POINT3 pos;
    NJS_VECTOR speed;
    NJS_VECTOR offset;
    Angle ang;
    Float distance;
    Float *to;
    Float *at;
    Float *by;
    int i;

    if (placement_try_leave_by(task, RANGE)) {
        return;
    }
    if (!placement_editing(task)) {
        switch (work->mode) {
        case SHOWER_WAITING:
            /* @hack spelling: the offset formed through Float walks over
             * offset, pos and the player's pos; as stores to offset's members,
             * 92 of 365 words differ */
            to = &offset.x;
            at = &work->pos.x;
            by = &player_work[0]->pos.x;
            *to++ = *at++ - *by++;
            *to++ = *at++ - *by++;
            *to = *at - *by;
            distance = njScalor(&offset);
            if (distance <= UNDER_DISTANCE) {
                if (VARS(work)->pour == 0) {
                    sound_play(SE_STEP_IN, 0, 0, 0);
                }
                ++VARS(work)->pour;
                if (VARS(work)->pour > shower_wait + HOLD_POUR) {
                    work->mode = SHOWER_HOLDING;
                    work->timer = 0;
                    player_set_mode(0, PLAYER_MODE_SHOWER);
                }
            } else {
                VARS(work)->pour = 0;
            }
            break;
        case SHOWER_HOLDING:
            if (TASK_TIMER(work) > shower_length) {
                player_set_mode(0, PLAYER_MODE_SHOWER_END);
                work->mode = SHOWER_DONE;
                work->timer = 0;
            }
            break;
        case SHOWER_DONE:
            /* @hack spelling: the offset formed through the same walks; as
             * stores to offset's members, the object's code runs 32 bytes long
             * and 284 of 365 words differ */
            to = &offset.x;
            at = &work->pos.x;
            by = &player_work[0]->pos.x;
            *to++ = *at++ - *by++;
            *to++ = *at++ - *by++;
            *to = *at - *by;
            distance = njScalor(&offset);
            if (distance <= UNDER_DISTANCE) {
                ++VARS(work)->pour;
            } else {
                VARS(work)->pour = 0;
            }
            break;
        }
    }
    if (VARS(work)->pour > 0) {
        /* pour stops at 65535 rather than wrapping to 0 */
        /* @hack spelling: <= 0, which for the unsigned pour is == 0, keeps
         * the image's CMP/PL R0 at 0x0c914c38. as == 0, 2 of 365 words
         * differ */
        if (++VARS(work)->pour <= 0) {
            --VARS(work)->pour;
        }
        /* @hack type: the buttons compared as Sint32s, which puts
         * SHOWER_DONE in R2 for the mode's store (MOV #1,R2 at 0x0c914c50).
         * as the Uint32s they are, it takes R3, and 2 of 365 words differ */
        if ((Sint32)pad_records[0].press & (Sint32)pad_jump_buttons) {
            work->mode = SHOWER_DONE;
            work->timer = 0;
            player_set_mode(0, PLAYER_MODE_RESET);
        }
        sound_play_for(SE_SHOWER, (int)work, 1, 0, 10);
        mud_set_level(mud_get_level() - shower_wash);
        speed.x = SPOUT_X;
        speed.y = SPOUT_Y;
        speed.z = SPOUT_Z;
        pos = speed;
        njUnitVector(&speed);
        /* @hack spelling: speed scaled through a Float walk; as *= on its
         * members, 142 of 365 words differ */
        to = &speed.x;
        *to++ *= -shower_speed;
        *to++ *= -shower_speed;
        *to *= -shower_speed;
        ang = (Angle)(rand() * 8.444444F);
        /* @bug logic: the sine's angle is 0 to 89 in Ninja's units, not
         * degrees (0x0c914cf2 to 0x0c914cfc). its sine stays under 0.01, so
         * each drop starts about 0.7 times shower_radius from the spout, never
         * farther */
        distance = (shower_radius * 0.7F) +
                   (shower_radius *
                    njSin((Angle)(rand() * (90.0F * RAND_SCALE))) * 0.3F);
        pos.x += distance * njCos(ang);
        pos.z += distance * njSin(ang);
        njPushMatrix(&_unit_matrix_);
        njTranslateV(NULL, &work->pos);
        TURN_VIA(njRotateY, work->ang.y, ang);
        njCalcVector(NULL, &speed, &speed);
        njCalcPoint(NULL, &pos, &pos);
        njPopMatrix(1);
        shower_draw(task);
        for (i = 0; i < shower_drops; ++i) {
            water_drop_add(&pos, &speed, shower_spread,
                           player_get(0)->shadow.below);
        }
        ++work->timer;
    }
}

static void shower_draw(struct task *task)
{
    struct task_work *work = task->work;
    NJS_OBJECT *object = &shower_object;
    NJS_MESHSET *meshset;
    NJS_COLOR room[WATER_MESHSETS * FIRST_UVS];
    NJS_COLOR *own[WATER_MESHSETS];
    NJS_COLOR *from;
    NJS_COLOR *to;
    Angle ang;
    int i;
    int count;
    Uint32 slide = (Uint8)(TASK_TIMER(work) * shower_slide);

    if (!placement_try_leave_by(task, RANGE) &&
        VARS(work)->pour >= shower_wait && frame_step <= 0) {
        draw_attr_save();
        draw_attr_clear(0, NJD_FILTER_BILINEAR);
        casino_set_texture(CASINO_TEXTURES_ACTS_12);
        njPushMatrix(NULL);
        /* @hack type: the matrix passed as 0, which keeps the model in R12
         * (MOV.L @(4,R10),R12 at 0x0c914f02). as NULL, it takes R7 and the
         * slide R11, and 14 of 138 words differ */
        njTranslateV(0, &work->pos);
        TURN_VIA(njRotateY, work->ang.y, ang);
        meshset = object->model->meshsets;
        for (i = 0; i < WATER_MESHSETS; ++i, ++meshset) {
            /* @hack spelling: slide copied to shift in each pass (MOV R9,R1 at
             * 0x0c914f16). through slide itself, the code is 32 bytes short
             * and 98 of 138 words differ */
            int shift = slide;

            count = i == 0 ? FIRST_UVS : SECOND_UVS;
            from = meshset->vertuv;
            /* @bug logic: the second meshset's UVs go to
             * room[i * count], which is room[SECOND_UVS], not room[FIRST_UVS]
             * past the first's (MUL.L at 0x0c914f1e). they overwrite the
             * first's last four UVs, so the first meshset draws with the
             * second's first four in their place */
            to = &room[i * count];
            own[i] = from;
            meshset->vertuv = to;
            while (count--) {
                to->tex.u = from->tex.u;
                to->tex.v = from->tex.v - shift;
                ++from;
                ++to;
            }
        }
        draw_model(object->model);
        meshset = object->model->meshsets;
        for (i = 0; i < WATER_MESHSETS; ++i, ++meshset) {
            meshset->vertuv = own[i];
        }
        njPopMatrix(1);
        draw_attr_restore();
    }
}
