/* the coconut a palm drops as its lane's Chao swings it. it bounces to
 * rest, for the Chao to come and eat */
#include <ninja.h>
#include <sg_xpt.h>
#include <stdlib.h>

#include "al_race/coconut_speed.h"
#include "al_race/item.h"
#include "al_race/palm_coconut.h"
#include "al_race/racer.h"
#include "al_race/scenery.h"
#include "collision/collision.h"
#include "draw/model.h"
#include "object/placement.h"
#include "object/simple.h"
#include "sound/cue.h"
#include "system/frame.h"
#include "system/task.h"
#include "system/task_desc.h"
#include "system/texture.h"
#include "system/util.h"

/* the coconut */
extern NJS_OBJECT dat_0cbd12e8;

#define SE_COCONUT_BOUNCE 602 /* each landing, as the fall turns to a rise */
/* while eaten, on each step the timer has bit 3 set */
#define SE_COCONUT_EAT 620

/* the coconut's radius: its sphere's, and the height object_bounce keeps its
 * center above the ground */
#define PALM_COCONUT_RADIUS 2.7F

enum palm_coconut_mode {
    PALM_COCONUT_FALLING, /* bouncing to rest */
    PALM_COCONUT_LYING, /* at rest, for its owner to come and touch */
    PALM_COCONUT_TOUCHED, /* its owner is told to eat it */
    PALM_COCONUT_BEING_EATEN, /* shrinking as its owner eats it */
    PALM_COCONUT_LEFT /* at rest, with its owner not coming */
};

/* the state of a coconut a palm drops, in its task's motion */
struct palm_coconut {
    Sint8 mode;
    Sint8 lane;
    /* the steps since the Chao began eating it. after 59 the timer goes
     * back to 45 */
    Sint16 timer;
    NJS_MODEL *model;
    struct collision_list *list;
    NJS_VECTOR speed;
    /* the task of the Chao that swung its palm, or null once the coconut lets
     * go of it */
    struct task *owner;
    Angle spin; /* its turn about z a step while it falls */
};

#define PALM_COCONUT(task) ((struct palm_coconut *)(task)->motion)

/* what palm_coconut_create gives the coconut's start */
struct palm_coconut_start {
    Sint8 lane;
    NJS_OBJECT *object;
    struct collision_list list;
    NJS_POINT3 *pos;
    struct angles *ang;
    struct task *owner;
};

static void palm_coconut_start(struct task *task, void *arg);
static void palm_coconut_step(struct task *task);
static void palm_coconut_draw(struct task *task);
static void palm_coconut_destroy(struct task *task);
static Float palm_coconut_scale(struct palm_coconut *coconut);

static struct collision_shape palm_coconut_shapes[1];

/* where each lane's last coconut came to rest */
static NJS_POINT3 palm_coconut_rests[RACE_LANES];

static const struct sound_falloff palm_coconut_bounce_falloff = {127, -128,
                                                                 0.0F, 1200.0F};
static const struct sound_falloff palm_coconut_eat_falloff = {0, -128, 0.0F,
                                                              600.0F};
static const struct task_desc palm_coconut_desc = {
    sizeof(struct palm_coconut), TASK_WORK,         palm_coconut_start,
    palm_coconut_step,           palm_coconut_draw, palm_coconut_destroy};
static const NJS_VECTOR palm_coconut_unit = {1.0F, 1.0F, 1.0F};
static const struct collision_kind palm_coconut_collision = {COLLISION_OBJECT,
                                                             1};
/* the share of each axis of its speed the coconut keeps on a bounce */
static const NJS_VECTOR palm_coconut_bounce = {0.5F, 0.5F, 0.5F};

static struct collision_shape palm_coconut_shapes[1] = {
    {SHAPE_SPHERE,
     0,
     PUSH_FIRM(PUSH_ALL),
     {0.0F, 0.0F, 0.0F},
     PALM_COCONUT_RADIUS,
     0.0F,
     0.0F,
     {0, 0, 0},
     0,
     NULL},
};
/* the start's argument. palm_coconut_create sets its lane, pos, ang and
 * owner for each coconut, and the start reads them as scenery_add creates
 * the coconut */
static struct palm_coconut_start palm_coconut_kind = {
    0, &dat_0cbd12e8, {palm_coconut_shapes, 1, 40.0F}, NULL, NULL, NULL};

void palm_coconut_create(int lane, NJS_POINT3 *pos, struct angles *ang,
                         struct task *owner)
{
    /* @hack redundant: a local whose address is taken, which keeps the call
     * to scenery_add a JSR with an RTS of its own (0x0cb98a96, 0x0cb98a9c);
     * without it, SHC jumps to scenery_add, and 16 of 16 words differ */
    int unused;
    int *held = &unused;

    palm_coconut_kind.lane = lane;
    /* @hack type: pos and ang stored by their offsets from the record's
     * start, which takes each one's address from a literal (MOV.L at
     * 0x0cb98a80 and 0x0cb98a84); as members, SHC stores them off the
     * record's address, and 16 of 16 words differ */
    *(NJS_POINT3 **)((char *)&palm_coconut_kind + 0x14) = pos;
    *(struct angles **)((char *)&palm_coconut_kind + 0x18) = ang;
    palm_coconut_kind.owner = owner;
    scenery_add(&palm_coconut_desc, &palm_coconut_kind);
}

int palm_coconut_tell(struct task *task, struct task *other, Sint8 command)
{
    struct palm_coconut *coconut = PALM_COCONUT(task);
    int heard = 0;

    if (coconut->owner == other) {
        switch (command) {
        case ITEM_LET_GO:
            coconut->owner = NULL;
            heard = 1;
            break;
        case PALM_COCONUT_EAT:
            if (coconut->mode == PALM_COCONUT_TOUCHED) {
                coconut->timer = 0;
                coconut->mode = PALM_COCONUT_BEING_EATEN;
                heard = 1;
            }
            break;
        case PALM_COCONUT_EATEN:
            if (coconut->mode == PALM_COCONUT_BEING_EATEN) {
                coconut->owner = NULL;
                task_kill(task);
                heard = 1;
            }
            break;
        }
    }
    return heard;
}

int palm_coconut_get_rest(NJS_POINT3 *pos, int lane)
{
    int found;

    if (pos != NULL && lane >= 0 && lane < RACE_LANES) {
        *pos = palm_coconut_rests[lane];
        found = 1;
    } else {
        found = 0;
    }
    return found;
}

static void palm_coconut_start(struct task *task, void *arg)
{
    struct palm_coconut_start *start = arg;
    struct collision_list *list;
    struct task_work *work = task->work;
    struct palm_coconut *coconut = PALM_COCONUT(task);

    list = &start->list;
    work->pos = *start->pos;
    work->ang = *start->ang;
    work->scl = palm_coconut_unit;
    object_collision_init(task, list, &palm_coconut_collision);
    coconut->mode = PALM_COCONUT_FALLING;
    coconut->lane = start->lane;
    coconut->model = start->object->model;
    coconut->list = list;
    coconut_speed.x = RANDOM(-0.5F, -0.25F);
    coconut->speed = coconut_speed;
    /* @hack spelling: coconut_speed read through its address, which SHC loads
     * again after rand's call (0x0cb98bc6) rather than hold in R12 across
     * it; as coconut_speed.x, 24 of 75 words differ */
    coconut->spin = -((&coconut_speed)->x * 364.0F);
    /* @hack redundant: two jumps to the next statement, which move SHC's choice
     * of scratch register, so that the owner's copy goes through R1
     * (0x0cb98be8, 0x0cb98bea); without them, it goes through R0, and 2 of
     * 75 words differ */
    goto owner_setup;
owner_setup:
    goto owner_copy;
owner_copy:
    coconut->owner = start->owner;
}

static void palm_coconut_step(struct task *task)
{
    struct palm_coconut *coconut;
    struct task_work *work;
    NJS_VECTOR *speed;
    struct task *chao_task;

    palm_coconut_draw(task);
    coconut = PALM_COCONUT(task);
    work = task->work;
    chao_task = coconut->owner;
    switch (coconut->mode) {
    case PALM_COCONUT_FALLING: {
        Float fall;
        NJS_VECTOR *bounce;

        work->ang.z += coconut->spin;
        speed = &coconut->speed;
        fall = speed->y;
        /* @hack spelling: palm_coconut_bounce through a local, which puts
         * its address before 2.7F in the pool (0x0cb98cc0, 0x0cb98cc4);
         * passed directly, it comes after, and 11 of 157 words differ */
        bounce = (NJS_VECTOR *)&palm_coconut_bounce;
        if (!object_bounce(&work->pos, speed, bounce, PALM_COCONUT_RADIUS)) {
            palm_coconut_rests[coconut->lane] = work->pos;
            if (item_tell_chao(chao_task, task, ITEM_GO_TO,
                               PALM_COCONUT_KIND)) {
                coconut->mode = PALM_COCONUT_LYING;
            } else {
                coconut->mode = PALM_COCONUT_LEFT;
            }
        }
        if (fall < 0.0F && speed->y >= 0.0F) {
            sound_play_near(SE_COCONUT_BOUNCE, &work->pos,
                            &palm_coconut_bounce_falloff);
        }
        break;
    }
    case PALM_COCONUT_LYING:
        if (item_hit_chao(work->collision) == chao_task) {
            if (item_tell_chao(chao_task, task, ITEM_EAT, PALM_COCONUT_KIND)) {
                coconut->mode = PALM_COCONUT_TOUCHED;
            }
        }
        collision_register(work);
        break;
    case PALM_COCONUT_TOUCHED:
        break;
    case PALM_COCONUT_BEING_EATEN:
        if (coconut->timer & 8) {
            sound_play_near(SE_COCONUT_EAT, &work->pos,
                            &palm_coconut_eat_falloff);
        }
        work->scl.x = palm_coconut_scale(coconut);
        break;
    case PALM_COCONUT_LEFT:
        collision_register(work);
        break;
    }
}

static void palm_coconut_draw(struct task *task)
{
    struct task_work *work;
    struct palm_coconut *coconut;
    struct angles *ang;
    Float scale;

    if (frame_step) {
        return;
    }
    if (placement_far_from_camera(task)) {
        return;
    }
    work = task->work;
    coconut = PALM_COCONUT(task);
    scale = work->scl.x;
    ang = &work->ang;
    if (scale > 0.0F) {
        Angle turn;

        njPushMatrix(NULL);
        njTranslateV(NULL, &work->pos);
        TURN_VIA(njRotateX, ang->x, turn);
        TURN_VIA(njRotateZ, ang->z, turn);
        njScale(NULL, scale, scale, scale);
        texture_set_stage();
        draw_model(coconut->model);
        njPopMatrix(1);
    }
}

static void palm_coconut_destroy(struct task *task)
{
    struct palm_coconut *coconut = PALM_COCONUT(task);

    item_tell_chao(coconut->owner, task, ITEM_LET_GO, PALM_COCONUT_KIND);
}

/* return the coconut's scale for the timer's step of its eating, and move
 * the timer on. the scale is 1 for the first 15 steps, 0.75 for the next
 * 15 and 0.5 for the 15 after. the timer then runs from 45 to 59, round and
 * round. there the scale is -1 for the first step, then 0.5 and -1 by turns,
 * two steps each. a scale of -1 hides the coconut */
static Float palm_coconut_scale(struct palm_coconut *coconut)
{
    int timer = coconut->timer;
    Float scale;

    if (timer < 15) {
        scale = 1.0F;
    } else if (timer < 30) {
        scale = 0.75F;
    } else if (timer < 45 || (timer & 2)) {
        scale = 0.5F;
    } else {
        scale = -1.0F;
    }
    timer++;
    if (timer >= 60) {
        timer = 45;
    }
    coconut->timer = timer;
    return scale;
}
