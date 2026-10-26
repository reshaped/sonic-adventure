/* the shards a broken platform or cylinder throws apart */
#include <ninja.h>
#include <sg_xpt.h>
#include <stdlib.h>

#include "debug/editor.h"
#include "draw/tree.h"
#include "object/placement.h"
#include "stg06/models.h"
#include "stg06/shards.h"
#include "system/task.h"
#include "system/util.h"

extern NJS_OBJECT shard_object;

/* the square of the distance from the camera or player 0 within which the
 * shards stay */
#define SHARDS_RANGE ((1500.0F * 1500.0F) + 100.0F)
/* a shard is gone once it is farther than this from where the shards start */
#define SHARD_GONE 300.0F
/* the heap bytes each shard takes: a struct shard, and 4 more that nothing
 * reads */
#define SHARD_BYTES 52
/* each of a shard's spins is a random rate of up to SHARD_SPIN degrees a step,
 * either way */
#define SHARD_SPIN 5.0F

/* each step adds velocity to pos, and spin_x and spin_z to ang_x and ang_z.
 * nothing sets or reads ang_y or spin_y */
struct shard {
    NJS_POINT3 pos;
    NJS_VECTOR velocity;
    Angle ang_x;
    Angle ang_y;
    Angle ang_z;
    Angle spin_x;
    Angle spin_y;
    Angle spin_z;
};

/* the shards. the list and its entries are on the heap, and the destructor
 * frees both. each step adds gravity to every shard's velocity.y. scale is
 * set to 1 and never read */
struct shard_list {
    int count;
    Float gravity;
    Float scale;
    struct shard *entries;
};

/* task_work as the shards lay it out. vars holds the object and the list,
 * and pos is where the shards start */
struct shards_work {
    Sint8 mode;
    Sint8 submode;
    Uint8 id;
    Uint8 tick;
    Sint16 flags;
    Sint16 timer;
    NJS_OBJECT *object;
    char pad0c[4];
    struct shard_list *list;
    char pad14[12];
    NJS_POINT3 pos;
};

static void shards_destroy(struct task *task);
static void shards_step(struct task *task);
static void shards_draw(struct task *task);
static void shards_init(struct task *task);

/* the shards' first step: start them, unless placement_try_leave_by ends
 * the task */
static void shards_start(struct task *arg)
{
    /* @hack spelling: task, a copy of arg, which keeps it in R14 across the
     * range check (0x0c9155c8); as arg named task, with no copy, it waits on
     * the stack and 12 of 16 words differ */
    struct task *task = arg;
    if (!placement_try_leave_by(task, SHARDS_RANGE)) {
        shards_init(task);
    }
}

void shards_init(struct task *task)
{
    struct shards_work *work = (struct shards_work *)task->work;
    task->draw = shards_draw;
    task->step = shards_step;
    task->destructor = shards_destroy;
    work->timer = 0;
    work->mode = 0;
}

void shards_destroy(struct task *task)
{
    struct shards_work *work = (struct shards_work *)task->work;
    heap_free(work->list->entries);
    heap_free(work->list);
}

/* out of SHARDS_RANGE, the task ends. otherwise draw the shards. then,
 * unless the set editor holds them, move each shard and spin it about x and
 * z. the task ends once every shard is gone */
void shards_step(struct task *task)
{
    struct shards_work *work = (struct shards_work *)task->work;
    int i;
    int outside;
    struct shard *shard;

    if (!placement_try_leave_by(task, SHARDS_RANGE)) {
        shards_draw(task);
        if (!placement_editing(task)) {
            outside = 0;
            for (i = 0; i < work->list->count; ++i) {
                shard = &work->list->entries[i];
                shard->pos.x += shard->velocity.x;
                shard->pos.y += shard->velocity.y;
                shard->pos.z += shard->velocity.z;
                shard->ang_x += shard->spin_x;
                shard->ang_z += shard->spin_z;
                shard->velocity.y += work->list->gravity;
                if (njDistanceP2P(&shard->pos, &work->pos) > SHARD_GONE) {
                    ++outside;
                }
            }
            if (outside >= work->list->count) {
                task_kill(task);
            }
            ++work->timer;
        }
    }
}

void shards_draw(struct task *task)
{
    struct shards_work *work = (struct shards_work *)task->work;
    struct shard *shard;
    int i;

    if (!placement_try_leave_by(task, SHARDS_RANGE)) {
        for (i = 0; i < work->list->count; ++i) {
            shard = &work->list->entries[i];
            njSetTexture(&obj_skydeck_texlist);
            njPushMatrix(NULL);
            njTranslateV(NULL, &shard->pos);
            TURN(njRotateX, shard->ang_x);
            TURN(njRotateZ, shard->ang_z);
            draw_tree(work->object);
            njPopMatrix(1);
        }
    }
}

struct task *sky_deck_shards_create(NJS_POINT3 *pos, Float speed, Float gravity,
                                    int count, int unused)
{
    struct shard *entries;
    struct shard_list *list;
    struct task *task;
    struct shards_work *work;
    struct shard *shard;
    int i;

    /* @bug memory: a failed allocation leaks what was allocated before it.
     * the list's failure leaks the entries, and task_create's both: each
     * test's BT/S, at 0x0c915812 and 0x0c915822, leaves for the NULL return
     * at 0x0c915906 with them unfreed */
    /* @hack spelling: each pointer stored in the test that checks it, which
     * stores it in the delay slot of the test's branch (0x0c915804,
     * 0x0c915814, 0x0c915824); stored apart and tested by nested ifs, 143 of
     * 169 words and 44 of 46 other words differ, and 10 of shards_draw's 73,
     * and the function ends 32 bytes early */
    if ((entries = heap_alloc(SHARD_BYTES * count)) &&
        (list = heap_alloc(sizeof(struct shard_list))) &&
        (task = task_create(TASK_WORK, 3, shards_start))) {
        work = (struct shards_work *)task->work;
        work->pos = *pos;
        work->list = list;
        list->entries = entries;
        work->object = &shard_object;
        work->list->gravity = gravity;
        work->list->scale = 1.0F;
        work->list->count = count;
        for (i = 0; i < count; ++i) {
            shard = work->list->entries;
            shard += i;
            shard->pos = work->pos;
            /* doubled for each shard, so each shard's speed can reach twice the
             * last's */
            speed *= 2.0F;
            shard->velocity.x = RANDOM(-0.5F, 0.5F) * speed;
            shard->velocity.y = RANDOM(-0.5F, 0.5F) * speed;
            shard->velocity.z = RANDOM(-0.5F, 0.5F) * speed;
            shard->ang_x = 0;
            shard->ang_z = 0;
            shard->spin_x =
                (Angle)(RANDOM(-SHARD_SPIN, SHARD_SPIN) * ANGLE_PER_DEGREE);
            shard->spin_z =
                (Angle)(RANDOM(-SHARD_SPIN, SHARD_SPIN) * ANGLE_PER_DEGREE);
        }
        return task;
    }
    return NULL;
}
