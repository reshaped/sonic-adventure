/* O DEKA, a rock pillar that topples and lies where it falls. it topples
 * once E-102 has shot down its targets, or for another character, once a
 * player comes near */
#include <ninja.h>
#include <sg_xpt.h>

#include "collision/world.h"
#include "debug/editor.h"
#include "draw/draw.h"
#include "draw/model.h"
#include "object/placement.h"
#include "object/target.h"
#include "player/player.h"
#include "sound/sound.h"
#include "stage/stage.h"
#include "stg05/dust_line.h"
#include "stg05/rock_pillar.h"
#include "system/task.h"
#include "system/util.h"

extern NJS_TEXLIST obj_mountain_texlist;
extern NJS_ACTION rock_pillar_action;
extern NJS_OBJECT rock_pillar_world_object;

enum rock_pillar_mode { PILLAR_INIT, PILLAR_STAND, PILLAR_FALL, PILLAR_DOWN };

struct rock_pillar_vars {
    /* the pillar's object in the world from the fall on. a moving one stands
     * in while the pillar falls, and a solid one once it lies */
    NJS_OBJECT *world;
    /* rock_pillar_world_object, whose model, child and sibling world takes */
    NJS_OBJECT *from;
};

#define VARS(work) ((struct rock_pillar_vars *)&(work)->vars)
/* the set entry's scales. E-102 topples the pillar once every target with
 * the id TARGETS is shot down. another character topples it once a player
 * comes within RANGE */
#define TARGETS(work) ((work)->scl.x)
#define RANGE(work) ((work)->scl.y)
/* the fall's frame, FALL_STEP a step. the action's frame is twice it */
#define FRAME(work) ((work)->scl.z)

/* the range a RANGE under 1 stands for */
#define PILLAR_RANGE 500.0F

/* the fall ends once FRAME passes FALL_END. on the way, dust rises at
 * FALL_DUST, and the crash sounds at FALL_CRASH. the action holds its pose
 * from FALL_LAST_DRAWN on */
#define FALL_STEP 0.5F
#define FALL_END 119.0F
#define FALL_DUST 44.0F
#define FALL_CRASH 54.0F
#define FALL_LAST_DRAWN 59.0F

/* as the pillar lands, dust rises along a line from its base. the line runs
 * DUST_REACH along z, turned by the pillar's y angle. DUST_PUFFS rise from
 * each end */
#define DUST_REACH 200.0F
#define DUST_SCALE 10.0F
#define DUST_PUFFS 20

/* the scale the pillar draws at, which its objects in the world take too */
#define PILLAR_SCALE 0.1F
/* how far above the pillar its falling object starts */
#define PILLAR_DROP 60.0F
/* the world tests the pillar's object only while a player is within this
 * times PILLAR_SCALE */
#define PILLAR_REACH 2000.0F

#define DRAW_RADIUS 1500.0F

#define SE_PILLAR_TOPPLE 124 /* as the pillar starts to fall */
/* the volume of both sounds, before their falloff by distance */
#define PILLAR_VOLUME 96

static void rock_pillar_draw(struct task *task);
static void rock_pillar_destroy(struct task *task);
static void rock_pillar_lie(struct task *task);
static void rock_pillar_sync_world(struct task *task);
static void rock_pillar_topple(struct task *task);
static void rock_pillar_drop(struct task *task);

/* stand until TARGETS or RANGE topples the pillar, then fall and lie still.
 * while the set editor holds the pillar, it shows the pillar standing */
void rock_pillar_step(struct task *task)
{
    struct task_work *work = task->work;
    NJS_POINT3 landing;
    NJS_VECTOR ahead;
    Angle ang;

    if (placement_try_leave(task)) {
        return;
    }
    if (placement_editing(task)) {
        FRAME(work) = 0.0F;
        rock_pillar_draw(task);
        return;
    }
    switch (work->mode) {
    case PILLAR_INIT:
        if (RANGE(work) < 1.0F) {
            RANGE(work) = PILLAR_RANGE;
        }
        FRAME(work) = 0.0F;
        rock_pillar_draw(task);
        task->draw = rock_pillar_draw;
        task->destructor = rock_pillar_destroy;
        work->mode = PILLAR_STAND;
        break;
    case PILLAR_STAND:
        if (get_character() == CHARACTER_E102) {
            if (target_cleared((int)TARGETS(work))) {
                work->mode = PILLAR_FALL;
                sound_play_at(SE_PILLAR_TOPPLE, 0, 0, PILLAR_VOLUME,
                              work->pos.x, work->pos.y, work->pos.z);
                rock_pillar_topple(task);
            }
        } else {
            if (player_in_range(&work->pos, RANGE(work))) {
                work->mode = PILLAR_FALL;
                sound_play_at(SE_PILLAR_TOPPLE, 0, 0, PILLAR_VOLUME,
                              work->pos.x, work->pos.y, work->pos.z);
                rock_pillar_topple(task);
            }
        }
        rock_pillar_draw(task);
        break;
    case PILLAR_FALL:
        FRAME(work) += FALL_STEP;
        rock_pillar_drop(task);
        if (FRAME(work) > FALL_END) {
            work->mode = PILLAR_DOWN;
            rock_pillar_destroy(task);
            rock_pillar_lie(task);
        }
        if (FRAME(work) == FALL_DUST) {
            ahead.x = 0.0F;
            ahead.y = 0.0F;
            ahead.z = DUST_REACH;
            njPushMatrix(NULL);
            njUnitMatrix(NULL);
            TURN_VIA(njRotateY, work->ang.y, ang);
            njCalcVector(NULL, &ahead, &landing);
            njPopMatrix(1);
            landing.x += work->pos.x;
            landing.y += work->pos.y;
            landing.z += work->pos.z;
            dust_create_line(&work->pos, &landing, DUST_SCALE, DUST_PUFFS);
            dust_create_line(&landing, &work->pos, DUST_SCALE, DUST_PUFFS);
        }
        if (FRAME(work) == FALL_CRASH) {
            sound_play_at(SE_ROCK_BLAST, 0, 0, PILLAR_VOLUME, work->pos.x,
                          work->pos.y, work->pos.z);
        }
        rock_pillar_draw(task);
        rock_pillar_sync_world(task);
        break;
    case PILLAR_DOWN:
        rock_pillar_draw(task);
        rock_pillar_sync_world(task);
        break;
    }
}

static void rock_pillar_draw(struct task *task)
{
    struct task_work *work = task->work;
    Float frame;
    Angle ang;

    /* the only store to from. the first step calls this before
     * rock_pillar_topple or rock_pillar_lie can read it */
    VARS(work)->from = &rock_pillar_world_object;
    if (draw_in_view(work->pos.x, work->pos.y, work->pos.z, DRAW_RADIUS)) {
        frame = FRAME(work);
        if (frame >= FALL_LAST_DRAWN) {
            frame = FALL_LAST_DRAWN;
        }
        njSetTexture(&obj_mountain_texlist);
        njPushMatrix(NULL);
        njTranslateV(NULL, &work->pos);
        TURN_VIA(njRotateY, work->ang.y, ang);
        njScale(NULL, PILLAR_SCALE, PILLAR_SCALE, PILLAR_SCALE);
        draw_divide_light(PILLAR_SCALE);
        draw_action(&rock_pillar_action, frame * 2.0F);
        draw_reset_light();
        njPopMatrix(1);
    }
}

static void rock_pillar_destroy(struct task *task)
{
    struct task_work *work = task->work;

    if (VARS(work)->world) {
        world_remove(task, VARS(work)->world);
        world_object_free(VARS(work)->world);
    }
}

/* replace the pillar's falling object in the world with a solid one where
 * it lies. rock_pillar_destroy has removed and freed the falling object
 * already, and left world set. removing and freeing it again changes
 * nothing */
static void rock_pillar_lie(struct task *task)
{
    struct task_work *work = task->work;
    NJS_OBJECT *object;

    if (VARS(work)->world) {
        world_remove(task, VARS(work)->world);
        world_object_free(VARS(work)->world);
    }
    if (!placement_editing(task)) {
        VARS(work)->world = object = world_object_alloc();
        object->evalflags = NJD_EVAL_BREAK;
        object->scl[0] = PILLAR_SCALE;
        object->scl[1] = PILLAR_SCALE;
        object->scl[2] = PILLAR_SCALE;
        object->ang[0] = work->ang.x;
        object->ang[1] = work->ang.y;
        object->ang[2] = work->ang.z;
        object->pos[0] = work->pos.x;
        object->pos[1] = work->pos.y;
        object->pos[2] = work->pos.z;
        object->model = VARS(work)->from->model;
        object->child = VARS(work)->from->child;
        object->sibling = VARS(work)->from->sibling;
        world_add(WORLD_SOLID, task, object);
    }
}

/* move the pillar's object in the world to the pillar. set WORLD_TESTED
 * only while a player is within PILLAR_REACH times PILLAR_SCALE */
static void rock_pillar_sync_world(struct task *task)
{
    struct task_work *work = task->work;
    NJS_OBJECT *object = VARS(work)->world;

    if (object) {
        object->pos[0] = work->pos.x;
        object->pos[1] = work->pos.y;
        object->pos[2] = work->pos.z;
        object->ang[0] = work->ang.x;
        object->ang[1] = work->ang.y;
        object->ang[2] = work->ang.z;
        if (player_in_range(&work->pos, PILLAR_REACH * PILLAR_SCALE)) {
            work->flags |= WORLD_TESTED;
        } else {
            work->flags &= ~WORLD_TESTED;
        }
    }
}

/* put the pillar's falling object in the world, PILLAR_DROP above the
 * pillar */
static void rock_pillar_topple(struct task *task)
{
    struct task_work *work = task->work;
    NJS_OBJECT *object;

    if (!placement_editing(task)) {
        VARS(work)->world = object = world_object_alloc();
        object->evalflags = NJD_EVAL_BREAK;
        object->scl[0] = PILLAR_SCALE;
        object->scl[1] = PILLAR_SCALE;
        object->scl[2] = PILLAR_SCALE;
        object->ang[0] = work->ang.x;
        object->ang[1] = work->ang.y;
        object->ang[2] = work->ang.z;
        object->pos[0] = work->pos.x;
        object->pos[1] = work->pos.y + PILLAR_DROP;
        object->pos[2] = work->pos.z;
        object->model = VARS(work)->from->model;
        object->child = VARS(work)->from->child;
        object->sibling = VARS(work)->from->sibling;
        world_add(WORLD_MOVES, task, object);
    }
}

/* lower the pillar's falling object to PILLAR_DROP less FRAME above the
 * pillar, and no lower than the pillar. rock_pillar_sync_world runs later in
 * the same step, and puts the object back at the pillar's height */
static void rock_pillar_drop(struct task *task)
{
    struct task_work *work = task->work;
    Float drop = PILLAR_DROP - FRAME(work);
    NJS_OBJECT *object;

    if (drop < 0.0F) {
        drop = 0.0F;
    }
    object = VARS(work)->world;
    if (object) {
        object->pos[0] = work->pos.x;
        object->pos[1] = work->pos.y + drop;
        object->pos[2] = work->pos.z;
        object->ang[0] = work->ang.x;
        object->ang[1] = work->ang.y;
        object->ang[2] = work->ang.z;
        if (player_in_range(&work->pos, PILLAR_REACH * PILLAR_SCALE)) {
            work->flags |= WORLD_TESTED;
        } else {
            work->flags &= ~WORLD_TESTED;
        }
    }
}
