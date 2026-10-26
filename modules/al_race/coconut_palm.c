/* the palms of a race of kind 4, one to a lane, which each lane's Chao
 * sways until its coconut drops */
#include <ninja.h>
#include <sg_xpt.h>
#include <stdlib.h>

#include "al_race/coconut_motion.h"
#include "al_race/coconut_palm.h"
#include "al_race/game.h"
#include "al_race/item.h"
#include "al_race/palm_coconut.h"
#include "al_race/racer.h"
#include "al_race/scenery.h"
#include "collision/collision.h"
#include "draw/draw.h"
#include "object/placement.h"
#include "object/simple.h"
#include "sound/cue.h"
#include "system/frame.h"
#include "system/task.h"
#include "system/task_desc.h"
#include "system/texture.h"
#include "system/util.h"

/* the palm's model and its sway */
extern NJS_ACTION dat_0cbd3758;

/* the palm's sound, as its sway starts and as each sway ends */
#define SE_PALM 601

/* a palm's modes. PALM_WAIT waits for its lane's Chao to touch it.
 * PALM_CLAIMED waits for that Chao's COCONUT_PALM_SWAY. PALM_SWAYING sways
 * the palm, and drops its coconut once timer runs out. PALM_STOPPING plays
 * the sway to its end, and PALM_STILL stands still */
enum coconut_palm_mode {
    PALM_WAIT,
    PALM_CLAIMED,
    PALM_SWAYING,
    PALM_STOPPING,
    PALM_STILL
};

/* a palm's state, in its task's motion */
struct coconut_palm {
    Sint8 mode;
    Sint8 lane; /* the lane whose racer's Chao can claim it */
    Uint8 unk_02[2];
    Float frame;
    Float speed; /* the frames the sway plays a step */
    Float frames; /* the frames of one sway, its motion's nbFrame */
    NJS_ACTION *action;
    struct collision_list *list;
    struct task *owner; /* the task of the Chao that claimed it, or null */
    NJS_ACTION *coconut; /* the coconut's action, until it drops, then null */
    /* the steps the palm sways before its coconut drops, from 0 to 59 at
     * the start */
    Sint16 timer;
};

#define COCONUT_PALM(task) ((struct coconut_palm *)(task)->motion)

/* coconut_palm_start's arg: the palm's lane, its action, its coconut's
 * action, its collision list and where it stands */
struct coconut_palm_start {
    Sint8 lane;
    NJS_ACTION *action;
    NJS_ACTION *coconut;
    struct collision_list list;
    const NJS_POINT3 *pos;
};

static void coconut_palm_start(struct task *task, void *arg);
static void coconut_palm_step(struct task *task);
static void coconut_palm_draw(struct task *task);
static void coconut_palm_destroy(struct task *task);
static void coconut_palm_drop(struct task *task);

static struct collision_shape coconut_palm_shapes[1];

/* the coconut's position, angles and scale at the palm's frame, as
 * njGetAction gives them */
static Float coconut_palm_coconut_pos[3];
static Angle coconut_palm_coconut_ang[3];
static Float coconut_palm_coconut_scl[3];

static const NJS_POINT3 coconut_palm_places[RACE_LANES] = {
#include "al_race/coconut_palm/coconut_palm_places.inc"
};
static const struct sound_falloff coconut_palm_falloff = {0, -128, 20.0F,
                                                          600.0F};
static const struct task_desc coconut_palm_desc = {
    sizeof(struct coconut_palm), TASK_WORK,         coconut_palm_start,
    coconut_palm_step,           coconut_palm_draw, coconut_palm_destroy};
static const struct angles coconut_palm_no_turn = {0, 0, 0};
static const NJS_VECTOR coconut_palm_unit = {1.0F, 1.0F, 1.0F};
static const struct collision_kind coconut_palm_collision = {COLLISION_OBJECT,
                                                             1};
static const NJS_MOTION_DATA coconut_palm_coconut_motion = {
    coconut_palm_coconut_pos, coconut_palm_coconut_ang,
    coconut_palm_coconut_scl};

static struct collision_shape coconut_palm_shapes[1] = {
    {SHAPE_CYLINDER,
     0,
     PUSH_ALL | PUSH_FIRM(PUSH_ALL),
     {0.0F, 15.8F, 0.0F},
     2.5F,
     15.8F,
     0.0F,
     {0, 0, 0},
     0,
     NULL},
};
/* every palm's start record. coconut_palm_create sets lane and pos for each
 * palm in turn. coconut_palm_start, which scenery_add runs, reads them at
 * once */
static struct coconut_palm_start coconut_palm_kind = {
    0, &dat_0cbd3758, &dat_0cbd390c, {coconut_palm_shapes, 1, 40.0F}, NULL};

void coconut_palm_create(void)
{
    int i;
    const NJS_POINT3 *pos;

    for (i = 0, pos = coconut_palm_places; i < RACE_LANES; i++, pos++) {
        /* @hack type: lane stored through &coconut_palm_kind, and pos, its
         * last member, through a byte offset from its start. each store
         * then loads its own address from the pool, as the image's do
         * (0x0cb98f32, 0x0cb98f34). as coconut_palm_kind.lane and
         * coconut_palm_kind.pos, SHC keeps the record's address in R14
         * instead. the function runs 2 bytes short, 32 of 35 words differ,
         * and 32 of coconut_palm_tell's 52 */
        (&coconut_palm_kind)->lane = i;
        *(const NJS_POINT3 **)((char *)&coconut_palm_kind +
                               sizeof coconut_palm_kind -
                               sizeof(NJS_POINT3 *)) = pos;
        scenery_add(&coconut_palm_desc, &coconut_palm_kind);
    }
}

int coconut_palm_tell(struct task *task, struct task *other, Sint8 command)
{
    struct coconut_palm *palm = COCONUT_PALM(task);
    int heard = 0;

    if (palm->owner == other) {
        switch (command) {
        case ITEM_LET_GO:
            palm->owner = NULL;
            heard = 1;
            break;
        case COCONUT_PALM_SWAY:
            if (palm->mode == PALM_CLAIMED) {
                sound_play_near(SE_PALM, &task->work->pos,
                                &coconut_palm_falloff);
                palm->mode = PALM_SWAYING;
                heard = 1;
            }
            break;
        case COCONUT_PALM_STOP:
            if (palm->mode == PALM_SWAYING) {
                palm->owner = NULL;
                palm->mode = PALM_STOPPING;
                heard = 1;
            }
            break;
        }
    }
    return heard;
}

int coconut_palm_get_place(NJS_POINT3 *pos, int lane)
{
    int found;

    if (pos != NULL && lane >= 0 && lane < RACE_LANES) {
        *pos = coconut_palm_places[lane];
        found = 1;
    } else {
        found = 0;
    }
    return found;
}

static void coconut_palm_start(struct task *task, void *arg)
{
    struct coconut_palm_start *start = arg;
    struct collision_list *list;
    struct task_work *work = task->work;
    struct coconut_palm *palm = COCONUT_PALM(task);
    NJS_ACTION *action = start->action;

    list = &start->list;
    work->pos = *start->pos;
    work->ang = coconut_palm_no_turn;
    work->scl = coconut_palm_unit;
    /* @hack redundant: list->count, read and discarded, which keeps list in R12
     * and action in R11 (0x0cb9906a, 0x0cb9907a), as the image does.
     * without the read, the two swap and 10 of 84 words differ */
    (void)list->count;
    object_collision_init(task, list, &coconut_palm_collision);
    palm->mode = PALM_WAIT;
    palm->lane = start->lane;
    palm->frame = 0.0F;
    palm->speed = 1.0F;
    palm->frames = action->motion->nbFrame;
    palm->action = action;
    palm->list = list;
    palm->owner = NULL;
    palm->coconut = start->coconut;
    palm->timer = rand() * (60.0F * RAND_SCALE);
}

static void coconut_palm_step(struct task *task)
{
    struct coconut_palm *palm;
    struct task_work *work;
    struct task *chao_task;
    Bool same_lane;

    coconut_palm_draw(task);
    palm = COCONUT_PALM(task);
    work = task->work;
    switch (palm->mode) {
    case PALM_WAIT:
        chao_task = item_hit_chao(work->collision);
        /* @hack redundant: same_lane, which nothing reads, set in a test that
         * opens with TRUE &&. SHC stores the comparison's result in same_lane,
         * at @R15 (0x0cb9915c), as the image does. as a plain test of the lane,
         * 59 of 122 words differ */
        if (TRUE && (same_lane = race_lane_of_task(chao_task) == palm->lane)) {
            if (item_tell_chao(chao_task, task, ITEM_SWING,
                               COCONUT_PALM_KIND)) {
                palm->owner = chao_task;
                palm->mode = PALM_CLAIMED;
            }
        }
        break;
    case PALM_CLAIMED:
        break;
    case PALM_SWAYING:
        palm->frame += palm->speed;
        if (palm->frame >= palm->frames) {
            palm->frame -= palm->frames;
            sound_play_near(SE_PALM, &work->pos, &coconut_palm_falloff);
        }
        if (palm->coconut != NULL) {
            if (palm->timer > 0) {
                palm->timer--;
            } else {
                coconut_palm_drop(task);
                palm->coconut = NULL;
            }
        }
        break;
    case PALM_STOPPING:
        palm->frame += palm->speed;
        if (palm->frame >= palm->frames) {
            palm->frame = 0.0F;
            palm->mode = PALM_STILL;
        }
        break;
    case PALM_STILL:
        break;
    }
    collision_register(work);
}

static void coconut_palm_draw(struct task *task)
{
    struct task_work *work;
    struct coconut_palm *palm;
    NJS_ACTION *coconut;
    Float frame;

    if (frame_step == 0 && !placement_far_from_camera(task)) {
        work = task->work;
        palm = COCONUT_PALM(task);
        coconut = palm->coconut;
        frame = palm->frame;
        njPushMatrix(NULL);
        njTranslateV(NULL, &work->pos);
        texture_set_stage();
        draw_action(palm->action, frame);
        if (coconut != NULL) {
            draw_action(coconut, frame);
        }
        njPopMatrix(1);
    }
}

static void coconut_palm_destroy(struct task *task)
{
    struct coconut_palm *palm = COCONUT_PALM(task);

    item_tell_chao(palm->owner, task, ITEM_LET_GO, COCONUT_PALM_KIND);
}

/* drop the palm's coconut where its action puts it at the palm's frame,
 * turned about x alone as the action turns it there. palm_coconut_create
 * makes the falling coconut, for the palm's lane and owner */
static void coconut_palm_drop(struct task *task)
{
    struct task_work *work = task->work;
    struct coconut_palm *palm = COCONUT_PALM(task);
    NJS_POINT3 at;
    NJS_POINT3 pos;
    struct angles ang;
    Angle *coconut_ang;

    njGetAction(palm->coconut, (NJS_MOTION_DATA *)&coconut_palm_coconut_motion,
                palm->frame);
    at.x = coconut_palm_coconut_pos[0];
    at.y = coconut_palm_coconut_pos[1];
    at.z = coconut_palm_coconut_pos[2];
    /* @hack spelling: coconut_ang, the angles' address under a name of its
     * own, which SHC loads after the position's copy (0x0cb99338), as the
     * image does. read through coconut_palm_coconut_ang, it loads first and
     * 18 of 63 words differ */
    coconut_ang = coconut_palm_coconut_ang;
    ang.x = coconut_ang[0];
    ang.y = 0;
    ang.z = 0;
    njPushMatrix(NULL);
    njUnitMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    njCalcPoint(NULL, &at, &pos);
    njPopMatrix(1);
    palm_coconut_create(palm->lane, &pos, &ang, palm->owner);
}
