/* O SUITEKI water drops, which fall one by one through a tank's height and
 * splash at its floor */
#include <ninja.h>
#include <sg_xpt.h>
#include <stdlib.h>

#include "camera/camera.h"
#include "collision/collision.h"
#include "debug/editor.h"
#include "draw/draw.h"
#include "draw/model.h"
#include "draw/tree.h"
#include "object/placement.h"
#include "stg09/casino.h"
#include "stg09/water_drops.h"
#include "system/frame.h"
#include "system/task.h"
#include "system/util.h"

/* O SUITEKI is destroyed beyond about 250 units of the camera and player 0.
 * RANGE is that distance squared */
#define RANGE 62510.0F

/* a drop, then the splash's eight frames, in obj_casino9_texlist */
#define DROP_FRAMES 9

/* a splash's sprite scale both ways, as a multiple of water_drops_height */
#define SPLASH_SCALE 1.2F

struct water_drop {
    NJS_POINT3 pos; /* from the tank's center */
    Float speed; /* up */
    /* its frame in water_drops_frames: 0, the drop, while it hangs or falls,
     * then the splash's frames */
    Sint32 frame;
};

struct water_drops_vars {
    Uint16 count;
    Uint8 unk_02[6];
    struct water_drop *drops;
};

#define VARS(work) ((struct water_drops_vars *)&(work)->vars)

static Float water_drops_gravity = -0.13F;
static Float water_drops_drag = 0.98F; /* the share of its speed kept */
/* a falling drop's sprite scale across and up */
static Float water_drops_width = 0.3F;
static Float water_drops_height = 0.5F;
static NJS_TEXANIM water_drops_frames[DROP_FRAMES] = {
#include "stg09/water_drops/water_drops_frames.inc"
};

static void water_drops_init(struct task *task);
static void water_drops_destroy(struct task *task);
static void water_drops_run(struct task *task);
static void water_drops_draw(struct task *task);

/* the tank, faint green, for the set editor */
static void water_drops_draw_tank(NJS_POINT3 *size)
{
    Float x = size->x * 0.1F;
    Float y = size->y * 0.1F;
    Float z = size->z * 0.1F;

    njPushMatrix(NULL);
    njScale(NULL, x * 2.0F, y * 2.0F, z * 2.0F);
    draw_set_material(0.03F, 0.0F, 1.0F, 0.0F);
    draw_tree(marker_cube_object);
    njPopMatrix(1);
    draw_set_material(0.0F, 0.0F, 0.0F, 0.0F);
}

/* hang each drop at the tank's top, anywhere across it */
static void water_drops_init(struct task *task)
{
    struct task_work *work = task->work;
    int count = VARS(work)->count;
    struct water_drop *drop = VARS(work)->drops;
    Float x = work->scl.x * 2.0F;
    Float y = work->scl.y;
    Float z = work->scl.z * 2.0F;
    int i;
    Float *at;

    for (i = 0; i < count; ++i, ++drop) {
        /* @hack spelling: pos stored through a Float cursor, as the image's
         * stores through R8 and R13 (0x0c92481c to 0x0c924838); through the
         * members, 77 of 85 words differ */
        at = &drop->pos.x;
        *at++ = x * ((rand() * RAND_SCALE) + -0.5F);
        *at++ = y;
        *at = z * ((rand() * RAND_SCALE) + -0.5F);
        drop->speed = 0.0F;
        drop->frame = 0;
    }
}

void water_drops_step(struct task *task)
{
    struct task_work *work = task->work;

    if (!placement_try_leave_by(task, RANGE)) {
        task->draw = water_drops_draw;
        task->step = water_drops_run;
        task->destructor = water_drops_destroy;
        VARS(work)->count = (Uint8)work->ang.z;
        VARS(work)->drops =
            heap_alloc(VARS(work)->count * sizeof(struct water_drop));
        water_drops_init(task);
        work->timer = 0;
    }
}

static void water_drops_destroy(struct task *task)
{
    struct task_work *work = task->work;

    heap_free(VARS(work)->drops);
}

/* a drop hangs at the top until a chance each frame lets it fall */
static void water_drops_run(struct task *task)
{
    struct task_work *work = task->work;
    struct water_drop *drop;
    int count;
    Angle ang;
    Float x;
    Float y;
    Float z;
    Float chance;
    Float height;
    Float speed;

    if (placement_try_leave_by(task, RANGE)) {
        return;
    }
    if (placement_editing(task)) {
        njPrintC(EDITOR_LABEL_RIGHT(EDITOR_ROW_ANG_Z), "cnt:");
        njPrintD(NJM_LOCATION(24, EDITOR_ROW_ANG_Z), (Uint8)work->ang.z, 4);
        njPrintC(EDITOR_LABEL_RIGHT(EDITOR_ROW_SCL_X), "Suisou Scale.x");
        njPrintC(EDITOR_LABEL_RIGHT(EDITOR_ROW_SCL_Y), "Suisou Scale.y");
        njPrintC(EDITOR_LABEL_RIGHT(EDITOR_ROW_SCL_Z), "Suisou Scale.z");
        njPushMatrix(NULL);
        njTranslateV(NULL, &work->pos);
        TURN_VIA(njRotateY, work->ang.x, ang);
        TURN_VIA(njRotateY, work->ang.y, ang);
        water_drops_draw_tank(&work->scl);
        njPopMatrix(1);
        return;
    }
    if (!draw_in_view(work->pos.x, work->pos.y, work->pos.z,
                      njScalor(&work->scl) + 16.0F)) {
        return;
    }
    if (frame_step) {
        return;
    }
    water_drops_draw(task);
    count = VARS(work)->count;
    x = work->scl.x;
    y = work->scl.y;
    z = work->scl.z;
    chance = 1.0F / njSqrt(_builtin_fabsf(y * 2.5F / water_drops_gravity));
    drop = VARS(work)->drops;
    while (count--) {
        height = drop->pos.y;
        if (height < y || rand() * RAND_SCALE < chance) {
            speed = drop->speed;
            if (height <= -y) {
                height = -y;
                ++drop->frame;
                if (drop->frame >= DROP_FRAMES) {
                    height = y;
                    speed = 0.0F;
                    drop->frame = 0;
                    drop->pos.x = x * ((rand() * RAND_SCALE) + -0.5F) * 2.0F;
                    drop->pos.z = z * ((rand() * RAND_SCALE) + -0.5F) * 2.0F;
                }
            } else {
                speed += water_drops_gravity;
                speed *= water_drops_drag;
                height += speed;
            }
            drop->speed = speed;
            drop->pos.y = height;
        }
        ++drop;
    }
    ++work->timer;
}

static void water_drops_draw(struct task *task)
{
    struct task_work *work;
    struct water_drop *drop;
    NJS_SPRITE sprite;
    int count;
    Sint32 frame;
    Angle ang;

    /* @hack spelling: work set inside the test with a comma, which loads it
     * after task's store at @R15 (0x0c924bde, 0x0c924be2); set before the if,
     * the object's code runs 32 bytes short and 148 of 158 words differ */
    if (work = task->work, placement_try_leave_by(task, RANGE)) {
        return;
    }
    if (frame_step > 0) {
        return;
    }
    if (!draw_in_view(work->pos.x, work->pos.y, work->pos.z,
                      njScalor(&work->scl) + 20.0F)) {
        return;
    }
    /* @hack redundant: sprite.tlist set from camera_work and set again on
     * the next line. the store keeps the image's read of camera_work (MOV.L
     * @R4,R3 at 0x0c924c2a); without it, the object's code runs 32 bytes
     * short and 121 of 158 words differ */
    /* @hack spelling: camera_work read through a pointer to it, which keeps its
     * address in R4 (0x0c924c26); read by name, 4 of 158 words differ */
    {
        struct task_work **camera = &camera_work;

        sprite.tlist = (NJS_TEXLIST *)*camera;
    }
    sprite.tlist = &obj_casino9_texlist;
    sprite.tanim = water_drops_frames;
    /* @hack redundant: the width and height times 1, FLDI1 and FMUL at
     * 0x0c924c20, 0x0c924c36 and 0x0c924c3e; without it, the object's code
     * runs 32 bytes short and 122 of 158 words differ */
    sprite.sx = water_drops_width * 1.0F;
    sprite.sy = water_drops_height * 1.0F;
    njColorBlendingMode(NJD_SOURCE_COLOR, NJD_COLOR_BLENDING_SRCALPHA);
    njColorBlendingMode(NJD_DESTINATION_COLOR, NJD_COLOR_BLENDING_ONE);
    count = VARS(work)->count;
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    TURN_VIA(njRotateY, work->ang.x, ang);
    TURN_VIA(njRotateY, work->ang.y, ang);
    drop = VARS(work)->drops;
    while (count--) {
        frame = drop->frame;
        sprite.ang = 0;
        sprite.p = drop->pos;
        if (frame == 0) {
            njDrawSprite3D(&sprite, 0, NJD_SPRITE_SCALE | NJD_SPRITE_ALPHA);
        } else {
            sprite.sx = water_drops_height * SPLASH_SCALE;
            sprite.sy = water_drops_height * SPLASH_SCALE;
            njDrawSprite3D(&sprite, drop->frame,
                           NJD_SPRITE_SCALE | NJD_SPRITE_ALPHA);
            /* @hack redundant: the width and height times 1 again, FMUL at
             * 0x0c924cd2 and 0x0c924cda; without it, the object's code runs 32
             * bytes short and 151 of 158 words differ */
            sprite.sx = water_drops_width * 1.0F;
            sprite.sy = water_drops_height * 1.0F;
        }
        ++drop;
    }
    njPopMatrix(1);
}
