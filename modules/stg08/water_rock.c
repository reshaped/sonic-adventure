/* O MIZUIWAA to O MIZUIWAC: water rocks in the collision world, each scaled
 * by its set entry */
#include <ninja.h>
#include <sg_xpt.h>

#include "collision/world.h"
#include "debug/editor.h"
#include "draw/model.h"
#include "object/placement.h"
#include "player/player.h"
#include "stg08/water_rock.h"
#include "system/task.h"
#include "system/texture.h"
#include "system/util.h"

extern NJS_OBJECT water_rock_a_object;
extern NJS_OBJECT water_rock_b_object;
extern NJS_OBJECT water_rock_c_object;

/* a rock keeps WORLD_TESTED while player 0 or 1 is within its reach plus
 * ROCK_NEAR */
#define ROCK_NEAR 16.0F

/* a rock's own copy of its kind's model, on the heap. the rock is scaled by
 * scl's components plus 1, and the copy's radius by the largest of those */
struct water_rock_body {
    NJS_MODEL model;
    /* a radius about the rock's position that holds the scaled model. it is
     * the copy's radius plus the scaled distance of the model's center */
    Float reach;
};

struct water_rock_vars {
    /* the rock's object in the world, null in the set editor */
    NJS_OBJECT *object;
    Uint8 unk_04[4];
    struct water_rock_body *body;
};

#define WATER_ROCK(work) ((struct water_rock_vars *)&(work)->vars)

static void water_rock_step(struct task *task);
static void water_rock_draw(struct task *task);

static void water_rock_init(struct task *task, NJS_OBJECT *object);
static void water_rock_destroy(struct task *task);

void water_rock_a_start(struct task *task)
{
    water_rock_init(task, &water_rock_a_object);
}

void water_rock_b_start(struct task *task)
{
    water_rock_init(task, &water_rock_b_object);
}

void water_rock_c_start(struct task *task)
{
    water_rock_init(task, &water_rock_c_object);
}

static void water_rock_init(struct task *task, NJS_OBJECT *object)
{
    struct task_work *work = task->work;
    struct water_rock_body *body;
    NJS_OBJECT *world;
    Float scale;
    Float reach;
    Uint32 flags;

    if (placement_try_leave(task)) {
        return;
    }
    WATER_ROCK(work)->body = body = heap_alloc(sizeof(*body));
    task->destructor = water_rock_destroy;
    task->step = water_rock_step;
    task->draw = water_rock_draw;
    work->ang.x = 0;
    work->ang.z = 0;
    scale = work->scl.x;
    if (work->scl.y > scale) {
        scale = work->scl.y;
    }
    if (work->scl.z > scale) {
        scale = work->scl.z;
    }
    scale += 1.0F;
    body->model = *object->model;
    body->model.r *= scale;
    body->reach = body->model.r + (njScalor(&body->model.center) * scale);
    if (placement_editing(task)) {
        return;
    }
    WATER_ROCK(work)->object = world = world_object_alloc();
    world->scl[0] = work->scl.x + 1.0F;
    world->scl[1] = work->scl.y + 1.0F;
    world->scl[2] = work->scl.z + 1.0F;
    world->ang[0] = 0;
    world->ang[1] = work->ang.y;
    world->ang[2] = 0;
    world->pos[0] = work->pos.x;
    world->pos[1] = work->pos.y;
    world->pos[2] = work->pos.z;
    world->model = &body->model;
    work->flags &= ~WORLD_TESTED;
    reach = body->reach;
    flags = WORLD_SOLID;
    if (reach <= WORLD_SMALL_REACH) {
        flags |= WORLD_SMALL;
    } else if (reach <= WORLD_MEDIUM_REACH) {
        flags |= WORLD_MEDIUM;
    } else if (reach <= WORLD_LARGE_REACH) {
        flags |= WORLD_LARGE;
    }
    world_add(flags, task, world);
}

static void water_rock_destroy(struct task *task)
{
    struct task_work *work = task->work;

    if (WATER_ROCK(work)->object) {
        world_remove(task, WATER_ROCK(work)->object);
        world_object_free(WATER_ROCK(work)->object);
    }
    heap_free(WATER_ROCK(work)->body);
}

static void water_rock_step(struct task *task)
{
    struct task_work *work = task->work;
    struct water_rock_body *body = WATER_ROCK(work)->body;
    NJS_OBJECT *object;

    if (!placement_try_leave(task)) {
        /* @hack spelling: the object read into a local of its own, which
         * tests it in R4 and reads body back into R2 (0x0c9095da,
         * 0x0c9095e2); tested in place, both take R3 and 4 of 47 words
         * differ */
        object = WATER_ROCK(work)->object;
        if (object != NULL) {
            if (player_in_range(&work->pos, body->reach + ROCK_NEAR)) {
                work->flags |= WORLD_TESTED;
            } else {
                work->flags &= ~WORLD_TESTED;
            }
        }
        water_rock_draw(task);
    }
}

static void water_rock_draw(struct task *task)
{
    struct task_work *work = task->work;
    struct water_rock_body *body = WATER_ROCK(work)->body;
    Angle ang;

    if (!placement_try_leave(task)) {
        if (draw_in_view(work->pos.x, work->pos.y, work->pos.z, body->reach)) {
            texture_set_stage();
            njPushMatrix(NULL);
            njTranslateV(NULL, &work->pos);
            TURN_VIA(njRotateY, work->ang.y, ang);
            njScale(NULL, work->scl.x + 1.0F, work->scl.y + 1.0F,
                    work->scl.z + 1.0F);
            draw_model(&body->model);
            njPopMatrix(1);
        }
    }
}
