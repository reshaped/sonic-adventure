/* an event's Chaos puddle, and the blobs of water round a Chaos */
#include <mathf.h>
#include <ninja.h>
#include <sg_xpt.h>
#include <stdlib.h>

#include "chaos/chaos.h"
#include "chaos/puddle.h"
#include "collision/ground.h"
#include "draw/motion.h"
#include "draw/tree.h"
#include "system/task.h"
#include "system/util.h"

#define TEXT_1_LENGTH 14
#include "chaos/puddle/text.inc"

/* the most blobs blob_place puts round a Chaos */
#define BLOB_MAX 16

/* the event puddle's water texture, whose image chaos_animate_surface
 * flows through CHAOS_SURFACE's frames */
static NJS_TEXNAME puddle_textures[] = {{TEXT_1, 0, 0}};
static NJS_TEXLIST puddle_texlist = {
#include "chaos/puddle/puddle_texlist.inc"
};

static Sint8 blob_count;
static NJS_POINT3 blob_pos[BLOB_MAX]; /* each blob's place, from the Chaos */
static Float blob_size[BLOB_MAX]; /* each blob's size, from 0.5 up to 0.9 */
/* the last puddle made, which puddle_move moves */
static struct task *puddle_task;

void blob_place(struct task_work *work, int unused, int count)
{
    int i;
    Angle ang;
    Float radius;
    Float x;
    Float y;
    Float z;
    struct angles slope;

    blob_count = count < BLOB_MAX ? count : BLOB_MAX;
    for (i = 0; i < blob_count; i++) {
        radius = RANDOM(10.0F, 15.0F);
        ang = (Angle)(360.0F / blob_count * i * ANGLE_PER_DEGREE);
        x = radius * njCos(ang);
        z = radius * njSin(ang);
        blob_pos[i].x = x;
        blob_pos[i].z = z;
        y = ground_below(work->pos.x + x, work->pos.y + 3.0F, work->pos.z + z,
                         &slope);
        blob_pos[i].y = y - work->pos.y;
        blob_size[i] = RANDOM(0.5F, 0.9F);
    }
}

void blob_draw(void *unused, struct chaos *chaos)
{
    int i;
    Float scale_x;
    Float scale_y;
    Float fade;
    Float width;
    Float size;

    for (i = 0; i < blob_count; i++) {
        width = chaos->scale.x;
        size = blob_size[i];
        scale_y = chaos->scale.y * size;
        scale_x = width * size;
        fade = width - 0.75F;
        fade = fade > 0.0F ? fade : 0.0F;
        njPushMatrix(NULL);
        njTranslate(NULL, blob_pos[i].x * fade, blob_pos[i].y,
                    blob_pos[i].z * fade);
        njScale(NULL, scale_x, scale_y, scale_x);
        draw_tree(&chaos_blob);
        njPopMatrix(1);
    }
}

void blob_draw_spread(void *unused, struct chaos *chaos, Float spread)
{
    int i;
    Float scale_x;
    Float scale_y;
    Float reach;
    Float width;
    Float size;

    for (i = 0; i < blob_count; i++) {
        scale_x = width = chaos->scale.x;
        size = blob_size[i];
        scale_y = chaos->scale.y * size;
        scale_x *= size;
        width *= spread;
        reach = width * 0.5F;
        njPushMatrix(NULL);
        njTranslate(NULL, blob_pos[i].x * reach, blob_pos[i].y,
                    blob_pos[i].z * reach);
        njScale(NULL, scale_x, scale_y, scale_x);
        draw_tree(&chaos_blob);
        njPopMatrix(1);
    }
}

/* draw the puddle, its water flowing and its points trailing over the
 * ground */
static void puddle_draw(struct task *task)
{
    struct task_work *work = task->work;
    struct chaos *puddle = task->extra;

    chaos_animate_surface(puddle);
    njSetTexture(puddle->texlist);
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    chaos_puddle_deform(work, puddle, &chaos_puddle);
    njScaleV(NULL, &puddle->scale);
    draw_tree(&chaos_puddle);
    njPopMatrix(1);
}

/* give the puddle's model its own points back, and free the kept ones */
static void puddle_destroy(struct task *task)
{
    struct task_work *work = task->work;
    struct chaos *puddle = task->extra;

    puddle->puddle_mode = PUDDLE_FREE;
    chaos_puddle_deform(work, puddle, &chaos_puddle);
}

struct task *puddle_create(Angle ang)
{
    struct task_work *work;
    struct chaos *puddle;
    NJS_OBJECT *object;

    puddle_task = task_create(TASK_WORK, 1, puddle_draw);
    puddle_task->extra = heap_calloc(1, sizeof(struct chaos));
    puddle_task->destructor = puddle_destroy;
    puddle = puddle_task->extra;
    work = puddle_task->work;
    work->ang.y = ang;
    puddle->texlist = &puddle_texlist;
    puddle->puddle_mode = PUDDLE_MAKE;
    object = &chaos_puddle;
    chaos_puddle_deform(work, puddle, object);
    object->model->mats->attrflags |= NJD_FLAG_USE_ALPHA;
    object->model->mats->diffuse.argb.a = 0x99;
    return puddle_task;
}

void puddle_move(NJS_POINT3 *pos, NJS_VECTOR *scale)
{
    struct task_work *work = puddle_task->work;
    struct chaos *puddle = puddle_task->extra;
    Angle ang = (Angle)(atan2f(pos->z - work->pos.z, pos->x - work->pos.x) *
                        ANGLE_PER_RADIAN);

    work->ang.y = angle_approach(work->ang.y, ang, NJM_DEG_ANG(5));
    work->pos = *pos;
    puddle->scale = *scale;
}
