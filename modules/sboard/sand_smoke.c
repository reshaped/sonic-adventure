/* sand smoke: a sprite at each point of a shape. the sprites fade in, then
 * fade out once the smoke's maker sets done. pillars and sand worms raise
 * it */
#include <ninja.h>
#include <sg_xpt.h>

#include "draw/state.h"
#include "sboard/sand_smoke.h"
#include "sboard/sand_textures.h" /* NOLINT(misc-include-cleaner) */
#include "system/frame.h"
#include "system/task.h"
#include "system/task_desc.h"
#include "system/util.h"

/* a smoke's record, in task->motion */
struct sand_smoke_record {
    Sint8 mode;
    Sint8 frame; /* the index in sand_smoke_frames the sprites show */
    Sint8 frames;
    Sint8 wait; /* the steps to skip before the next frame */
    Float alpha;
    const struct sand_smoke_shape *shape;
    Sint8 *done; /* the maker's flag, TRUE to fade out */
};

enum sand_smoke_mode {
    SAND_SMOKE_FADE_IN,
    SAND_SMOKE_WAIT,
    SAND_SMOKE_FADE_OUT,
    SAND_SMOKE_GONE
};

/* each step of a fade adds FADE_IN or FADE_OUT to the alpha */
#define FADE_IN 0.02F
#define FADE_OUT (-0.01F)

/* the sprite's frames, sunabokem01 to 16 */
#define SAND_SMOKE_FRAMES 16

static void sand_smoke_init(struct task *task, void *arg);
static void sand_smoke_step(struct task *task);
static void sand_smoke_draw(struct task *task);

static const NJS_TEXANIM sand_smoke_frames[SAND_SMOKE_FRAMES] = {
#include "sboard/sand_smoke/sand_smoke_frames.inc"
};
const struct task_desc sand_smoke_desc = {sizeof(struct sand_smoke_record),
                                          TASK_WORK,
                                          sand_smoke_init,
                                          sand_smoke_step,
                                          sand_smoke_draw,
                                          NULL};

static NJS_SPRITE sand_smoke_sprite = {
#include "sboard/sand_smoke/sand_smoke_sprite.inc"
};

static void sand_smoke_turn_x(struct angles *ang)
{
    Angle x;

    TURN_VIA(njRotateX, ang->x, x);
}

static void sand_smoke_turn_y(struct angles *ang)
{
    Angle y;

    TURN_VIA(njRotateY, ang->y, y);
}

static void sand_smoke_turn_z(struct angles *ang)
{
    Angle z;

    TURN_VIA(njRotateZ, ang->z, z);
}

static void sand_smoke_init(struct task *task, void *arg)
{
    struct sand_smoke_arg *from = arg;
    struct task_work *work = task->work;
    struct sand_smoke_record *record = (struct sand_smoke_record *)task->motion;

    work->pos = *from->pos;
    work->ang = *from->ang;
    record->mode = SAND_SMOKE_FADE_IN;
    record->frame = 0;
    record->frames = SAND_SMOKE_FRAMES;
    record->wait = 1;
    record->alpha = 0.0F;
    record->shape = from->shape;
    record->done = from->done;
}

/* run the fade, and show the sprite's next frame every second step */
static void sand_smoke_step(struct task *task)
{
    struct sand_smoke_record *record;

    sand_smoke_draw(task);
    record = (struct sand_smoke_record *)task->motion;
    switch (record->mode) {
    case SAND_SMOKE_FADE_IN:
        if (record->alpha < 1.0F) {
            record->alpha += FADE_IN;
        } else {
            record->alpha = 1.0F;
            record->mode = SAND_SMOKE_WAIT;
        }
        /* no break: a smoke still fading in tests done too */
    case SAND_SMOKE_WAIT:
        if (*record->done == TRUE) {
            record->mode = SAND_SMOKE_FADE_OUT;
        }
        break;
    case SAND_SMOKE_FADE_OUT:
        if (record->alpha > 0.0F) {
            record->alpha += FADE_OUT;
        } else {
            record->alpha = 0.0F;
            record->mode = SAND_SMOKE_GONE;
        }
        break;
    case SAND_SMOKE_GONE:
    default:
        task_kill(task);
        break;
    }
    if (record->wait > 0) {
        record->wait--;
    } else {
        record->wait = 1;
        record->frame++;
        if (record->frame >= record->frames) {
            record->frame = 0;
        }
    }
}

/* on a frame's first step, draw a sprite at each of the shape's points.
 * they draw at the smoke's alpha, without fog */
static void sand_smoke_draw(struct task *task)
{
    struct sand_smoke_record *record;
    NJS_SPRITE *sprite;
    struct task_work *work;
    const struct sand_smoke_shape *shape;
    struct angles *ang;
    Float size;
    int frame;
    int count;
    int i;

    if (frame_step != 0) {
        return;
    }
    record = (struct sand_smoke_record *)task->motion;
    sprite = &sand_smoke_sprite;
    frame = (int)record->frame;
    shape = record->shape;
    size = shape->size;
    count = shape->count;
    work = task->work;
    sprite->sx = size;
    sprite->sy = size;
    draw_state_save();
    draw_translucent(record->alpha);
    njFogDisable();
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    ang = &work->ang;
    sand_smoke_turn_z(ang);
    sand_smoke_turn_y(ang);
    sand_smoke_turn_x(ang);
    for (i = 0; i < count; i++) {
        sprite->p = shape->points[i];
        njDrawSprite3D(sprite, frame,
                       NJD_SPRITE_COLOR | NJD_SPRITE_SCALE | NJD_SPRITE_ALPHA);
    }
    njPopMatrix(1);
    njFogEnable();
    draw_state_restore();
}
