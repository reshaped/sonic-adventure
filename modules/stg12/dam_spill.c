/* the spill of act 0's dam, which pours over four sides of the wall as the
 * wall sinks */
#include <ninja.h>
#include <sg_xpt.h>
#include <stdlib.h>

#include "draw/draw.h"
#include "draw/tree.h"
#include "effect/splash.h"
#include "sound/sound.h"
#include "stg12/dam.h"
#include "stg12/dam_spill.h"
#include "stg12/texlists.h"
#include "stg12/water.h"
#include "system/frame.h"
#include "system/task.h"
#include "system/util.h"

/* the spill over one side of the wall. the draw repeats it at each quarter
 * turn */
extern NJS_OBJECT shelter_dam_spill_object;

/* each side of the spill stands this far along its own z from the dam's
 * center */
#define SIDE_OUT (-193.0F)

/* the UVs in the spill's first mesh, and in its second */
#define UVS_0 56
#define UVS_1 20

/* how far each mesh's texture v offset falls a step. the offset wraps at
 * RUN_LENGTH */
#define RUN_SPEED_0 24
#define RUN_SPEED_1 36
#define RUN_LENGTH 512

/* each step in SPILL_POUR or SPILL_FOLLOW, the spill makes SPLASHES sets of
 * four splashes on the water below. a set lies SPLASH_OUT from the dam's
 * center, mirrored across its x and z axes. its angle off the -z axis is
 * random, from 10 to 80 degrees */
#define SPLASHES 4
#define SPLASH_OUT 175.0F
#define SPLASH_ANG_LOW ((Float)NJM_DEG_ANG(10))
#define SPLASH_ANG_HIGH ((Float)NJM_DEG_ANG(80))
#define SPLASH_RISE 0.8F
#define SPLASH_SCALE 3.0F

/* the constant material alpha the spill draws with. its fade starts here */
#define ALPHA (-0.5F)

#define SE_WATERFALL 315

/* the spill's modes, in order. SPILL_GROW grows it to full height, and then
 * sets DAM_SPILLING. SPILL_POUR and SPILL_FOLLOW keep it at the dam's level
 * until that level stops falling. SPILL_FADE narrows and fades it, and
 * clears DAM_SPILLING once its width is 0, in SPILL_GONE */
enum shelter_dam_spill_mode {
    SPILL_GROW,
    SPILL_POUR,
    SPILL_FOLLOW,
    SPILL_FADE,
    SPILL_GONE
};

/* work->scl holds each side's width, height and reach */
struct shelter_dam_spill_vars {
    /* the dam's level as the step last saw it. once the spill fades, it holds
     * the spill's alpha instead */
    Float level;
    Sint16 run[2]; /* each mesh's v offset, 0 or less */
    Float narrow; /* the width scale the spill loses each step it fades */
};

#define VARS(work) ((struct shelter_dam_spill_vars *)&(work)->vars)

/* the v of each UV in the two meshes, as the start finds them. the step sets
 * each UV's v to its own here plus its mesh's offset */
static Sint16 shelter_dam_spill_v0[UVS_0];
static Sint16 shelter_dam_spill_v1[UVS_1];

static void shelter_dam_spill_step(struct task *task);
static void shelter_dam_spill_draw(struct task *task);
static void shelter_dam_spill_destroy(struct task *task);

void shelter_dam_spill_start(struct task *task)
{
    struct task_work *work = task->work;
    int i;

    work->scl.x = 1.0F;
    work->scl.y = 0.0F;
    work->scl.z = 1.0F;
    work->ang.y = NJM_DEG_ANG(45);
    VARS(work)->run[0] = 0;
    VARS(work)->run[1] = 0;
    for (i = 0; i < UVS_0; i++) {
        shelter_dam_spill_v0[i] =
            shelter_dam_spill_object.model->meshsets[0].vertuv[i].tex.v;
    }
    for (i = 0; i < UVS_1; i++) {
        shelter_dam_spill_v1[i] =
            shelter_dam_spill_object.model->meshsets[1].vertuv[i].tex.v;
    }
    task->step = shelter_dam_spill_step;
    task->draw = shelter_dam_spill_draw;
    task->destructor = shelter_dam_spill_destroy;
}

static void shelter_dam_spill_step(struct task *task)
{
    struct task_work *work = task->work;
    NJS_COLOR *uv;
    Sint16 *from;
    Sint16 now;
    int i;
    int j;

    switch (work->mode) {
    case SPILL_GROW:
        work->pos.y = shelter_dam_level;
        work->scl.y += 0.1F;
        work->scl.z += 0.1F;
        if (work->scl.y >= 1.0F) {
            work->scl.y = 1.0F;
            work->mode = SPILL_POUR;
            shelter_dam_flags |= DAM_SPILLING;
        }
        break;
    case SPILL_POUR:
        work->pos.y = shelter_dam_level;
        work->scl.y -= 0.002F;
        work->scl.z -= 0.02F;
        if (work->scl.z <= 1.0F) {
            work->scl.z = 1.0F;
            work->mode = SPILL_FOLLOW;
            VARS(work)->level = shelter_dam_level;
        }
        sound_play_for(SE_WATERFALL, (int)task, 1, 0, 2);
        break;
    case SPILL_FOLLOW:
        work->pos.y = shelter_dam_level;
        work->scl.y -= 0.002F;
        if (VARS(work)->level == shelter_dam_level) {
            work->mode = SPILL_FADE;
            VARS(work)->narrow = 0.002F;
            VARS(work)->level = ALPHA;
        } else {
            VARS(work)->level = shelter_dam_level;
        }
        sound_play_for(SE_WATERFALL, (int)task, 1, 0, 2);
        break;
    case SPILL_FADE:
        work->scl.x -= VARS(work)->narrow;
        VARS(work)->narrow += 0.001F;
        VARS(work)->level -= 0.02F;
        work->scl.z -= 0.01F;
        work->scl.y -= 0.002F;
        if (work->scl.x <= 0.0F) {
            work->scl.x = 0.0F;
            work->mode = SPILL_GONE;
            shelter_dam_flags &= ~DAM_SPILLING;
        }
        break;
    case SPILL_GONE:
        break;
    }
    if (work->mode == SPILL_POUR || work->mode == SPILL_FOLLOW) {
        for (j = 0; j < SPLASHES; j++) {
            NJS_POINT3 pos;
            NJS_VECTOR speed = {0.0F, SPLASH_RISE, 0.0F};
            Angle ang;
            Float x;
            Float z;

            ang = RANDOM(SPLASH_ANG_LOW, SPLASH_ANG_HIGH);
            x = njSin(ang);
            z = njCos(ang);
            x = -(x * SPLASH_OUT);
            z = -(z * SPLASH_OUT);
            /* @hack redundant: stores of the 0 that speed.x and speed.z hold
             * already, by FMOV.S FR14 at 0x0c90eb08 and 0x0c90eb0c, the first
             * of four such pairs SHC unrolls; without them, the step runs 96
             * bytes short and 599 of 724 words differ */
            speed.x = 0.0F;
            speed.z = 0.0F;
            pos.y = shelter_water_level;
            pos.x = x + DAM_X;
            pos.z = z + DAM_Z;
            splash_create(&pos, &speed, SPLASH_SCALE);
            pos.x = DAM_X - x;
            pos.z = z + DAM_Z;
            splash_create(&pos, &speed, SPLASH_SCALE);
            pos.x = x + DAM_X;
            pos.z = DAM_Z - z;
            splash_create(&pos, &speed, SPLASH_SCALE);
            pos.x = DAM_X - x;
            pos.z = DAM_Z - z;
            splash_create(&pos, &speed, SPLASH_SCALE);
        }
    }
    if (work->mode != SPILL_GONE) {
        now = VARS(work)->run[0];
        now -= RUN_SPEED_0;
        now %= RUN_LENGTH;
        VARS(work)->run[0] = now;
        uv = shelter_dam_spill_object.model->meshsets[0].vertuv;
        from = shelter_dam_spill_v0;
        for (i = 0; i < UVS_0; i++) {
            uv->tex.v = *from + now;
            from++;
            uv++;
        }
        now = VARS(work)->run[1];
        now -= RUN_SPEED_1;
        now %= RUN_LENGTH;
        VARS(work)->run[1] = now;
        uv = shelter_dam_spill_object.model->meshsets[1].vertuv;
        from = shelter_dam_spill_v1;
        for (i = 0; i < UVS_1; i++) {
            uv->tex.v = *from + now;
            from++;
            uv++;
        }
        shelter_dam_spill_draw(task);
    }
}

static void shelter_dam_spill_draw(struct task *task)
{
    struct task_work *work = task->work;
    Float width;
    Float height;
    Float reach;
    Float *scale;

    if (frame_step == 0) {
        /* @hack spelling: a cursor through work->scl, which reads its floats by
         * FMOV.S @R4+ at 0x0c90efec; read as work->scl.x, .y and .z, 16 of
         * 170 words differ */
        scale = &work->scl.x;
        width = *scale++;
        height = *scale++;
        reach = *scale;
        if (work->mode == SPILL_FADE) {
            draw_set_material(VARS(work)->level, 0.0F, 0.0F, 0.0F);
        } else {
            draw_set_material(ALPHA, 0.0F, 0.0F, 0.0F);
        }
        njSetTexture(&hotshelter1_texlist);
        njPushMatrix(NULL);
        njTranslateV(NULL, &work->pos);
        TURN(njRotateY, work->ang.y);
        njPushMatrix(NULL);
        njTranslate(NULL, 0.0F, 0.0F, SIDE_OUT);
        njScale(NULL, width, height, reach);
        draw_tree(&shelter_dam_spill_object);
        njPopMatrix(1);
        njPushMatrix(NULL);
        njRotateY(NULL, NJM_DEG_ANG(90));
        njTranslate(NULL, 0.0F, 0.0F, SIDE_OUT);
        njScale(NULL, width, height, reach);
        draw_tree(&shelter_dam_spill_object);
        njPopMatrix(1);
        njPushMatrix(NULL);
        njRotateY(NULL, NJM_DEG_ANG(180));
        njTranslate(NULL, 0.0F, 0.0F, SIDE_OUT);
        njScale(NULL, width, height, reach);
        draw_tree(&shelter_dam_spill_object);
        njPopMatrix(1);
        njPushMatrix(NULL);
        njRotateY(NULL, NJM_DEG_ANG(270));
        njTranslate(NULL, 0.0F, 0.0F, SIDE_OUT);
        njScale(NULL, width, height, reach);
        draw_tree(&shelter_dam_spill_object);
        njPopMatrix(1);
        njPopMatrix(1);
        draw_set_material(0.0F, 0.0F, 0.0F, 0.0F);
    }
}

static void shelter_dam_spill_destroy(struct task *task)
{
}
