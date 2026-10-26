/* BigStar: a sprite that turns to face the camera and shows its eight images
 * in turn */
#include <ninja.h>
#include <sg_xpt.h>

#include "camera/camera.h"
#include "stg09/big_star.h"
#include "stg09/obj_casino_textures.h"
#include "system/frame.h"
#include "system/task.h"
#include "system/util.h"

#define BIG_STAR_FRAMES 8

struct big_star_vars {
    Float shown; /* the steps the image has shown, less 1 */
};

#define VARS(work) ((struct big_star_vars *)&(work)->vars)

static NJS_TEXANIM big_star_frames[BIG_STAR_FRAMES] = {
#include "stg09/big_star/big_star_frames.inc"
};

static NJS_SPRITE big_star_sprite;

static void big_star_destroy(struct task *task)
{
}

/* the stars share big_star_sprite, which each draws at its own position */
static void big_star_draw(struct task *task)
{
    struct task_work *work = task->work;
    Angle ang;
    Float scale;

    if (frame_step) {
        return;
    }
    njSetTexture(&big_star_texlist);
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    TURN_VIA(njRotateY, camera_work->ang.y, ang);
    scale = work->scl.x;
    njScale(NULL, scale, scale, scale);
    njDrawSprite3D(&big_star_sprite, work->tick, NJD_SPRITE_ALPHA);
    njPopMatrix(1);
}

static void big_star_run(struct task *task)
{
    struct task_work *work = task->work;

    if (VARS(work)->shown >= work->scl.z) {
        work->tick = (work->tick + 1) % BIG_STAR_FRAMES;
        VARS(work)->shown = 0.0F;
    } else {
        VARS(work)->shown += 1.0F;
    }
    big_star_draw(task);
}

void big_star_step(struct task *task)
{
    struct task_work *work = task->work;

    work->tick = 0;
    big_star_sprite.tlist = &big_star_texlist;
    big_star_sprite.tanim = big_star_frames;
    big_star_sprite.sx = 1.0F;
    big_star_sprite.sy = 1.0F;
    big_star_sprite.p.x = 0.0F;
    big_star_sprite.p.y = 0.0F;
    big_star_sprite.p.z = 0.0F;
    task->step = big_star_run;
    task->draw = big_star_draw;
    task->destructor = big_star_destroy;
}
