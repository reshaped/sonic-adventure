/* O TP RING ring that is also drawn in the mirrors */
#include <ninja.h>
#include <sg_xpt.h>

#include "camera/camera.h"
#include "collision/collision.h"
#include "collision/ground.h"
#include "debug/editor.h"
#include "draw/draw.h"
#include "draw/model.h"
#include "hud/hud.h"
#include "object/models.h"
#include "object/placement.h"
#include "object/ring.h"
#include "sound/sound.h"
#include "stg03/mirror.h"
#include "stg03/tp_ring.h"
#include "system/frame.h"
#include "system/task.h"
#include "system/texture.h"
#include "system/util.h"

/* the ring's turn a step */
#define RING_SPIN NJM_DEG_ANG(2)

/* a sparkle draws while its sprite frame is at least 0 and below SPARKLE_END.
 * a taken ring ends once sparkle 2's frame passes SPARKLE_END. each step adds
 * SPARKLE_SPEED to each sparkle's frame. at frame f, a sparkle's scale is f
 * times SPARKLE_SIZE. it draws SPARKLE_DROP times its scale below the ring's
 * position. each sparkle draws SPARKLE_SPACING further back in x and z than
 * the one before */
#define SPARKLE_END 7.0F
#define SPARKLE_SPEED 0.8F
#define SPARKLE_SIZE 0.05F
#define SPARKLE_DROP 3.0F
#define SPARKLE_SPACING 2.0F

/* the ring draws its shadow while work->tick is below SHADOW_TICKS */
#define SHADOW_TICKS 50

/* the steps until the ring looks for the ground again, or -1 for a ring that
 * doesn't look. init sets -1, which skips the countdown and its reset to
 * RING_PROBE_COUNT, so the ring never looks */
#define RING_PROBE_WAIT(work) ((work)->submode)
/* sparkles 1 and 2's frames. RING(work)->spin holds sparkle 0's */
#define SPARKLE_1_FRAME(work) ((work)->scl.x)
#define SPARKLE_2_FRAME(work) ((work)->scl.z)
/* the height of the ground below the ring, or -GROUND_FAR for none */
#define RING_GROUND(work) ((work)->scl.y)

static void tp_ring_draw(struct task *task);
static void tp_ring_draw_sparkle(struct task_work *work);
static void tp_ring_sprite_init(struct task_work *work, NJS_SPRITE *sprite);
static void tp_ring_destroy(struct task *task);

/* the sparkle's eight sprite frames, ring01 to ring08 */
static NJS_TEXANIM tp_ring_texanims[8] = {
#include "stg03/tp_ring/tp_ring_texanims.inc"
};
static struct collision_shape tp_ring_shape = {
    SHAPE_SPHERE,
    0,
    PUSH_FIRM(PUSH_PLAYERS | PUSH_ENEMIES | PUSH_OTHERS),
    {0.0F, 0.0F, 0.0F},
    5.0F,
    0.0F,
    0.0F,
    {0},
    0,
    NULL,
};

/* the sprite that a taken ring's sparkles draw */
static NJS_SPRITE tp_ring_sprite;

/* outside the set editor the ring spins, and a hit takes it. a taken ring adds
 * one to the rings, plays SE_RING and starts its sparkles. until then, the ring
 * steps its reflection child, which draws it in the mirrors */
void tp_ring_step(struct task *task)
{
    struct task_work *work = task->work;
    Float height;
    Float rise;

    if (placement_try_leave(task)) {
        return;
    }
    switch (work->mode) {
    case RING_INIT:
        collision_init(task, &tp_ring_shape, COLLISION_RING, 1, 1);
        RING_PROBE_WAIT(work) = -1;
        RING_GROUND(work) =
            ground_below(work->pos.x, work->pos.y, work->pos.z, &work->ang);
        work->mode = RING_WAIT;
        RING(work)->spin = 0.0F;
        SPARKLE_1_FRAME(work) = -2.0F;
        SPARKLE_2_FRAME(work) = -4.0F;
        work->id = RING_ID;
        task->destructor = tp_ring_destroy;
        task->draw = tp_ring_draw;
        tp_ring_sprite_init(work, &tp_ring_sprite);
        twinkle_reflect(task);
        break;
    case RING_WAIT:
        if (edit_mode == 0) {
            if (work->collision->flags & COLLISION_HIT) {
                work->mode = RING_SPARKLE;
                ring_add(1);
                sound_play(SE_RING, 0, 0, 0);
            }
            collision_register(work);
            work->ang.y += RING_SPIN;
        }
        if (RING_PROBE_WAIT(work) >= 0) {
            RING_PROBE_WAIT(work)--;
            if (RING_PROBE_WAIT(work) == -1) {
                RING_PROBE_WAIT(work) = RING_PROBE_COUNT;
                height = ground_below(work->pos.x, work->pos.y, work->pos.z,
                                      &work->ang);
                rise = height - RING_GROUND(work);
                RING_GROUND(work) = height;
                work->pos.y += rise;
            }
        }
        tp_ring_draw(task);
        task_step_children(task);
        break;
    case RING_SPARKLE:
        tp_ring_draw_sparkle(work);
        break;
    case RING_DONE:
        placement_kill(task);
        break;
    }
}

/* on a frame's first step, draw the ring, with NJD_FLAG_IGNORE_SPECULAR
 * cleared from the attributes Ninja forces on. its shadow draws first, at
 * RING_GROUND unless that is -GROUND_FAR. this file never sets work->tick, so
 * the shadow's SHADOW_TICKS test always holds */
static void tp_ring_draw(struct task *task)
{
    struct task_work *work = task->work;
    Angle ang;

    if (frame_step != 0) {
        return;
    }
    if (work->tick < SHADOW_TICKS) {
        if (RING_GROUND(work) != -GROUND_FAR) {
            shadow_draw_at_height(work, RING_GROUND(work));
        }
    }
    draw_attr_save();
    draw_attr_clear(0, NJD_FLAG_IGNORE_SPECULAR);
    texture_set_regular();
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    TURN_VIA(njRotateY, work->ang.y, ang);
    draw_model(&ring_model);
    njPopMatrix(1);
    draw_attr_restore();
}

/* advance and draw a taken ring's sparkles 0 to 2. each runs two sprite
 * frames behind the one before. once sparkle 2's frame passes SPARKLE_END, set
 * the mode to RING_DONE */
static void tp_ring_draw_sparkle(struct task_work *work)
{
    Float size;
    Float scale;
    int frame[3];

    int i;
    Angle ang;

    RING(work)->spin += SPARKLE_SPEED;
    SPARKLE_1_FRAME(work) += SPARKLE_SPEED;
    SPARKLE_2_FRAME(work) += SPARKLE_SPEED;
    draw_set_material(0.0F, 0.0F, 0.0F, 0.0F);
    njColorBlendingMode(NJD_SOURCE_COLOR, NJD_COLOR_BLENDING_SRCALPHA);
    njColorBlendingMode(NJD_DESTINATION_COLOR, NJD_COLOR_BLENDING_INVSRCALPHA);
    njSetTexture(&ring_texlist);
    frame[0] = RING(work)->spin;
    frame[1] = SPARKLE_1_FRAME(work);
    frame[2] = SPARKLE_2_FRAME(work);
    for (i = 0; i < 3; i++) {
        if (frame[i] < SPARKLE_END && frame[i] >= 0) {
            size = frame[i] * SPARKLE_SIZE;
            scale = _builtin_fabsf(frame[i] * SPARKLE_SIZE);
            njPushMatrix(NULL);
            njTranslate(NULL, work->pos.x - (i * SPARKLE_SPACING),
                        work->pos.y - (size * SPARKLE_DROP),
                        work->pos.z - (i * SPARKLE_SPACING));
            TURN_VIA(njRotateY, camera_work->ang.y, ang);
            TURN_VIA(njRotateX, camera_work->ang.x, ang);
            njScale(NULL, scale, scale, scale);
            njDrawSprite3D(&tp_ring_sprite, frame[i],
                           NJD_SPRITE_ALPHA | NJD_SPRITE_VFLIP |
                               NJD_SPRITE_ANGLE);
            njPopMatrix(1);
        }
    }
    if (frame[2] > SPARKLE_END) {
        work->mode = RING_DONE;
    }
}

/* set sprite up to draw the sparkle's frames at a scale of 1 */
static void tp_ring_sprite_init(struct task_work *work, NJS_SPRITE *sprite)
{
    sprite->tlist = &ring_texlist;
    sprite->tanim = tp_ring_texanims;
    sprite->sx = 1.0F;
    sprite->sy = 1.0F;
    sprite->p.x = sprite->p.y = sprite->p.z = 0.0F;
}

static void tp_ring_destroy(struct task *task)
{
}
