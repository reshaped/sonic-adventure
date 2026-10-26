/* Red Mountain's puffs: smoke and flame sprites in four groups. each group
 * is a task that steps its puffs and draws them */
#include <ninja.h>

#include "camera/camera.h"
#include "draw/draw.h"
#include "effect/dust.h"
#include "effect/flame.h"
#include "stg05/puff.h"
#include "system/task.h"
#include "system/util.h"

static NJS_TEXANIM puff_smoke_frames[10] = {
#include "stg05/puff/puff_smoke_frames.inc"
};
static NJS_TEXANIM puff_flame_frames[8] = {
#include "stg05/puff/puff_flame_frames.inc"
};

/* each group's frames, which mountain_puff_frames gives. the flame's
 * animation holds 8 frames, but its group gives 6 */
#define SMOKE_FRAMES 10
#define FLAME_FRAMES 6

static struct task *puff_groups[4];
static NJS_SPRITE puff_smoke_sprite;
static NJS_SPRITE puff_flame_sprite;
static int puff_frames;

static void puff_draw_group(struct task *task, NJS_SPRITE *sprite);

static void puff_smoke_step(struct task *task)
{
    puff_frames = SMOKE_FRAMES;
    task_step_children(task);
    njColorBlendingMode(NJD_SOURCE_COLOR, NJD_COLOR_BLENDING_SRCALPHA);
    njColorBlendingMode(NJD_DESTINATION_COLOR, NJD_COLOR_BLENDING_INVSRCALPHA);
    draw_set_material(1.0F, 0.8F, 0.8F, 0.7F);
    njSetTexture(&dust_texlist);
    puff_draw_group(task, &puff_smoke_sprite);
}

static void puff_smoke_destroy(struct task *task)
{
    task_destroy_children(task);
}

static void puff_dark_smoke_step(struct task *task)
{
    puff_frames = SMOKE_FRAMES;
    task_step_children(task);
    njColorBlendingMode(NJD_SOURCE_COLOR, NJD_COLOR_BLENDING_SRCALPHA);
    njColorBlendingMode(NJD_DESTINATION_COLOR, NJD_COLOR_BLENDING_INVSRCALPHA);
    draw_set_material(1.0F, 0.3F, 0.3F, 0.2F);
    njSetTexture(&dust_texlist);
    puff_draw_group(task, &puff_smoke_sprite);
}

static void puff_dark_smoke_destroy(struct task *task)
{
    task_destroy_children(task);
}

static void puff_flame_step(struct task *task)
{
    puff_frames = FLAME_FRAMES;
    task_step_children(task);
    njColorBlendingMode(NJD_SOURCE_COLOR, NJD_COLOR_BLENDING_SRCALPHA);
    njColorBlendingMode(NJD_DESTINATION_COLOR, NJD_COLOR_BLENDING_ONE);
    draw_set_material(1.0F, 0.8F, 0.8F, 0.8F);
    njSetTexture(&flame_texlist);
    puff_draw_group(task, &puff_flame_sprite);
}

static void puff_flame_destroy(struct task *task)
{
    task_destroy_children(task);
}

static void puff_violet_smoke_step(struct task *task)
{
    puff_frames = SMOKE_FRAMES;
    task_step_children(task);
    njColorBlendingMode(NJD_SOURCE_COLOR, NJD_COLOR_BLENDING_SRCALPHA);
    njColorBlendingMode(NJD_DESTINATION_COLOR, NJD_COLOR_BLENDING_INVSRCALPHA);
    draw_set_material(1.0F, 0.7F, 0.6F, 1.0F);
    njSetTexture(&dust_texlist);
    puff_draw_group(task, &puff_smoke_sprite);
}

static void puff_violet_smoke_destroy(struct task *task)
{
    task_destroy_children(task);
}

void mountain_puff_init(void)
{
    struct task *task;

    puff_smoke_sprite.p.x = 0.0F;
    puff_smoke_sprite.p.y = 0.0F;
    puff_smoke_sprite.p.z = 0.0F;
    puff_smoke_sprite.tlist = &dust_texlist;
    puff_smoke_sprite.tanim = puff_smoke_frames;
    puff_flame_sprite.p.x = 0.0F;
    puff_flame_sprite.p.y = 0.0F;
    puff_flame_sprite.p.z = 0.0F;
    puff_flame_sprite.tlist = &flame_texlist;
    puff_flame_sprite.tanim = puff_flame_frames;
    task = task_create(0, 2, puff_smoke_step);
    task->destructor = puff_smoke_destroy;
    puff_groups[PUFF_SMOKE] = task;
    task = task_create(0, 2, puff_dark_smoke_step);
    task->destructor = puff_dark_smoke_destroy;
    puff_groups[PUFF_DARK_SMOKE] = task;
    task = task_create(0, 2, puff_flame_step);
    task->destructor = puff_flame_destroy;
    puff_groups[PUFF_FLAME] = task;
    task = task_create(0, 2, puff_violet_smoke_step);
    task->destructor = puff_violet_smoke_destroy;
    puff_groups[PUFF_VIOLET_SMOKE] = task;
}

static void puff_destroy(struct task *task)
{
    if (task->work != NULL) {
        heap_free(task->work);
        task->work = NULL;
    }
}

struct task *mountain_puff_create(int group, void (*step)(struct task *task))
{
    struct task *task = task_create_child(0, step, puff_groups[group]);
    void *work;

    if (task != NULL) {
        task->destructor = puff_destroy;
        /* @hack spelling: the work through a local, which copies it to R4 as
         * well, unused; stored directly, the function is 2 bytes shorter and 7
         * of 26 words differ */
        work = heap_alloc(sizeof(struct mountain_puff));
        task->work = work;
    }
    return task;
}

int mountain_puff_frames(void)
{
    return puff_frames;
}

/* draw each of the group's puffs as sprite at its pos, scale and ang. the
 * sprite turns to face the camera */
static void puff_draw_group(struct task *task, NJS_SPRITE *sprite)
{
    struct task *child = task->child;
    struct task *next;
    struct mountain_puff *puff;
    Angle ang;

    while (child != NULL) {
        next = child->next;
        /* @bug logic: the test reads the group's step, which is never null
         * while the group runs, so the test always holds. MOV.L @(16,R9),R2 at
         * 0x0c900390 reads it */
        if (task->step != NULL) {
            puff = (struct mountain_puff *)child->work;
            sprite->sx = puff->scale;
            sprite->sy = puff->scale;
            sprite->ang = puff->ang;
            njPushMatrix(NULL);
            njTranslateV(NULL, &puff->pos);
            TURN_VIA(njRotateY, camera_work->ang.y, ang);
            TURN_VIA(njRotateX, camera_work->ang.x, ang);
            njDrawSprite3D(sprite, (int)puff->frame,
                           NJD_SPRITE_ANGLE | NJD_SPRITE_COLOR |
                               NJD_SPRITE_ALPHA);
            njPopMatrix(1);
        }
        child = next;
    }
}
