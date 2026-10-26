/* Chao race item messages and zones */
#include <ninja.h>
#include <sg_xpt.h>

#include "al_race/ball.h"
#include "al_race/bugle.h"
#include "al_race/coconut_palm.h"
#include "al_race/item.h"
#include "al_race/palm_coconut.h"
#include "al_race/racer.h"
#include "al_race/scoop.h"
#include "al_race/watering_can.h"
#include "al_race/zone.h"
#include "collision/collision.h"
#include "system/task.h"

/* a zone with a scale z up to this is small, and one above it large */
#define ZONE_SMALL_SCALE 10.0F

static int (*const item_tells[6])(struct task *task, struct task *other,
                                  Sint8 command) = {
    coconut_palm_tell, palm_coconut_tell, ball_tell,
    watering_can_tell, bugle_tell,        scoop_tell,
};

struct task *item_hit_chao(struct collision_work *collision)
{
    struct task *task = NULL;
    struct collision_work *hit;

    if (collision->flags & COLLISION_HIT) {
        hit = collision->hit;
        if (hit != NULL && hit->kind == COLLISION_ENEMY) {
            task = hit->owner;
        }
    }
    return task;
}

int item_tell_chao(struct task *chao_task, struct task *other, Sint8 command,
                   Sint8 kind)
{
    struct racer *racer;
    int heard;

    if (chao_task == NULL || chao_task->work == NULL || other == NULL) {
        heard = 0;
    } else {
        racer = RACER_VARS(chao_task->work)->racer;
        if (racer == NULL || !racer_has_chao(racer)) {
            heard = 0;
        } else {
            heard = racer_tell(racer, command, other, kind);
        }
    }
    return heard;
}

int item_tell(struct task *task, struct task *other, Sint8 command, Sint8 kind)
{
    int (*tell)(struct task *task, struct task *other, Sint8 command);
    int heard;

    if (task == NULL || other == NULL) {
        heard = 0;
    } else {
        tell = item_tells[kind];
        if (tell == NULL) {
            heard = 0;
        } else {
            heard = tell(task, other, command);
        }
    }
    return heard;
}

Float item_distance_squared(struct task *task, struct task *other)
{
    Float distance = -1.0F;

    if (task != NULL && other != NULL) {
        NJS_POINT3 *from = &task->work->pos;
        NJS_POINT3 *to = &other->work->pos;
        Float dx = from->x - to->x;
        Float dy = from->y - to->y;
        Float dz = from->z - to->z;

        distance = (dx * dx) + (dy * dy) + (dz * dz);
    }
    return distance;
}

struct task_work *zone_get_work(struct collision_work *collision)
{
    struct task_work *work = NULL;
    struct task *owner;

    if (collision != NULL) {
        owner = collision->owner;
        if (owner != NULL && owner->destructor == zone_destroy) {
            work = owner->work;
        }
    }
    return work;
}

enum zone_size zone_get_size(struct collision_work *collision)
{
    struct task_work *work = zone_get_work(collision);
    enum zone_size size;

    if (work == NULL) {
        size = ZONE_NONE;
    } else if (work->scl.z <= ZONE_SMALL_SCALE) {
        size = ZONE_SMALL;
    } else {
        size = ZONE_LARGE;
    }
    return size;
}
