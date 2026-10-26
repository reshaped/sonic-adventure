/* sea surface of acts 1 and 2, which follows player 0 */
#include <ninja.h>
#include <sg_xpt.h>

#include "collision/world.h"
#include "draw/tree.h"
#include "player/player.h"
#include "stg01/beach.h"
#include "stg01/sea_surface.h"
#include "system/task.h"

#define SURFACE_Y (-1.5F)
/* the surface follows player 0 in whole strides of this many units in x
 * and z */
#define STRIDE 25.0F
/* the texture changes once TASK_TIMER(work) passes TEXTURE_WAIT, every 5
 * steps, through TEXTURES in turn */
#define TEXTURE_WAIT 4
#define TEXTURES 10

struct beach_sea_surface_vars {
    NJS_OBJECT *plane; /* act 1's plane in the collision world, or NULL */
};

#define VARS(work) ((struct beach_sea_surface_vars *)&(work)->vars)

/* tick holds the next texture */
#define TEXTURE(work) ((work)->tick)

/* act 2 hides the surface while player 0 or 1 is within HIDE_RANGE of this
 * point */
static NJS_POINT3 beach_sea_surface_hide_point = {6152.0F, -85.0F, 2383.0F};
#define HIDE_RANGE 300.0F

/* the surface's submodes. only act 2's step hides it */
enum { SURFACE_SHOWN, SURFACE_HIDDEN };

static struct task *beach_sea_surface_task;

static void beach_sea_surface_act1_init(struct task *task);
static void beach_sea_surface_act1_step(struct task *task);
static void beach_sea_surface_act2_init(struct task *task);
static void beach_sea_surface_act2_step(struct task *task);
static void beach_sea_surface_draw(struct task *task);
static void beach_sea_surface_destroy(struct task *task);

void beach_sea_surface_create(int mode)
{
    struct task_work *work;

    if (mode == BEACH_ACT1) {
        beach_sea_surface_task =
            task_create(TASK_WORK | TASK_FORCE, 3, beach_sea_surface_act1_init);
        work = beach_sea_surface_task->work;
        work->mode = BEACH_ACT1;
    } else if (mode == BEACH_ACT2) {
        beach_sea_surface_task =
            task_create(TASK_WORK, 3, beach_sea_surface_act2_init);
        work = beach_sea_surface_task->work;
        work->mode = BEACH_ACT2;
    }
}

void beach_sea_surface_remove(void)
{
    task_kill(beach_sea_surface_task);
}

/* put the plane in the collision world as moving water, where the surface
 * is */
static void beach_sea_surface_act1_init(struct task *task)
{
    struct task_work *work = task->work;
    NJS_OBJECT *plane;

    work->pos.y = SURFACE_Y;
    work->ang.x = 0;
    work->ang.y = 0;
    work->ang.z = 0;
    work->submode = SURFACE_SHOWN;
    TASK_TIMER(work) = 0;
    TEXTURE(work) = 0;
    VARS(work)->plane = plane = world_object_alloc();
    plane->evalflags = NJD_EVAL_BREAK;
    plane->scl[0] = 1.0F;
    plane->scl[1] = 1.0F;
    plane->scl[2] = 1.0F;
    plane->ang[0] = 0;
    plane->ang[1] = work->ang.y;
    plane->ang[2] = 0;
    plane->pos[0] = work->pos.x;
    plane->pos[1] = work->pos.y;
    plane->pos[2] = work->pos.z;
    plane->model = beach_sea_surface_plane_object.model;
    plane->child = NULL;
    plane->sibling = NULL;
    world_add(WORLD_MOVES | WORLD_WATER, task, plane);
    work->flags |= WORLD_TESTED;
    task->step = beach_sea_surface_act1_step;
    task->draw = beach_sea_surface_draw;
    task->destructor = beach_sea_surface_destroy;
}

/* step the surface's texture, and move the surface and its plane to
 * follow player 0 */
static void beach_sea_surface_act1_step(struct task *task)
{
    struct task_work *work = task->work;
    NJS_POINT3 player;
    Float x;
    Float z;
    Float player_x;
    Float player_z;
    Float strides_x;
    Float strides_z;

    TASK_TIMER(work)++;
    if (TASK_TIMER(work) > TEXTURE_WAIT) {
        TASK_TIMER(work) = 0;
        beach_sea_surface_object.model->mats->attr_texId = TEXTURE(work);
        TEXTURE(work)++;
        TEXTURE(work) %= TEXTURES;
    }
    player_get_history(0, 0, &player, NULL);
    x = work->pos.x;
    z = work->pos.z;
    player_x = player.x;
    player_z = player.z;
    strides_x = njRoundOff((player_x - x) * (1.0F / STRIDE));
    strides_z = njRoundOff((player_z - z) * (1.0F / STRIDE));
    x += strides_x * STRIDE;
    z += strides_z * STRIDE;
    work->pos.x = x;
    work->pos.z = z;
    VARS(work)->plane->pos[0] = x;
    VARS(work)->plane->pos[2] = z;
    beach_sea_surface_draw(task);
}

static void beach_sea_surface_act2_init(struct task *task)
{
    struct task_work *work = task->work;

    work->pos.y = SURFACE_Y;
    work->ang.x = 0;
    work->ang.y = 0;
    work->ang.z = 0;
    work->submode = SURFACE_SHOWN;
    TASK_TIMER(work) = 0;
    TEXTURE(work) = 0;
    VARS(work)->plane = NULL;
    task->step = beach_sea_surface_act2_step;
    task->draw = beach_sea_surface_draw;
    task->destructor = beach_sea_surface_destroy;
}

/* as act 1's step, without a plane. it also hides the surface near the hide
 * point */
static void beach_sea_surface_act2_step(struct task *task)
{
    struct task_work *work = task->work;
    NJS_POINT3 player;
    Float x;
    Float z;
    Float player_x;
    Float player_z;
    Float strides_x;
    Float strides_z;

    TASK_TIMER(work)++;
    if (TASK_TIMER(work) > TEXTURE_WAIT) {
        TASK_TIMER(work) = 0;
        beach_sea_surface_object.model->mats->attr_texId = TEXTURE(work);
        TEXTURE(work)++;
        TEXTURE(work) %= TEXTURES;
    }
    if (player_in_range(&beach_sea_surface_hide_point, HIDE_RANGE)) {
        work->submode = SURFACE_HIDDEN;
    } else {
        work->submode = SURFACE_SHOWN;
    }
    player_get_history(0, 0, &player, NULL);
    x = work->pos.x;
    z = work->pos.z;
    player_x = player.x;
    player_z = player.z;
    strides_x = njRoundOff((player_x - x) * (1.0F / STRIDE));
    strides_z = njRoundOff((player_z - z) * (1.0F / STRIDE));
    x += strides_x * STRIDE;
    z += strides_z * STRIDE;
    work->pos.x = x;
    work->pos.z = z;
    beach_sea_surface_draw(task);
}

static void beach_sea_surface_draw(struct task *task)
{
    struct task_work *work = task->work;

    if (work->submode == SURFACE_SHOWN) {
        njSetTexture(&beach_sea_surface_texlist);
        njPushMatrix(NULL);
        njTranslateV(NULL, &work->pos);
        draw_tree(&beach_sea_surface_object);
        njPopMatrix(1);
    }
}

static void beach_sea_surface_destroy(struct task *task)
{
    struct task_work *work = task->work;

    if (VARS(work)->plane != NULL) {
        world_remove(task, VARS(work)->plane);
        world_object_free(VARS(work)->plane);
    }
}
