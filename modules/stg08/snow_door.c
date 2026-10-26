/* O SNOW DOA, a drawbridge that falls open on its chains once the first big
 * icicle has landed */
#include <ninja.h>
#include <sg_xpt.h>

#include "collision/collision.h"
#include "debug/editor.h"
#include "draw/tree.h"
#include "effect/snow.h"
#include "object/placement.h"
#include "sound/sound.h"
#include "stg08/big_icicle.h"
#include "stg08/snow_door.h"
#include "system/task.h"
#include "system/texture.h"
#include "system/util.h"

extern NJS_OBJECT snow_door_object;
extern NJS_OBJECT snow_door_link_object;

/* destroyed beyond 1300 units of the camera and player 0 */
#define DOOR_RANGE (1300.0F * 1300.0F)

/* the door's opening as it lies flat: a quarter turn */
#define DOOR_OPEN 0x4000
#define DOOR_FALL_GAIN 2 /* the opening speed the door gains each step */

#define SE_DOOR_FALL 220
#define SE_DOOR_LAND 221
#define DOOR_FALL_UPDATES 90

/* links lie 7.6 units apart, and a chain has five more links than its span
 * needs */
#define LINK_SPAN 7.6F
#define LINK_EXTRA 5.0F

#define PUFFS 5
#define PUFF_SIZE 2.0F

struct snow_door_vars {
    Uint32 opening;
    Sint16 links; /* each chain's link count, as the last draw measured it */
    Sint16 landed; /* DOOR_IN_AIR, DOOR_LANDED or DOOR_PUFFED */
    Uint32 fall_speed;
};

#define VARS(work) ((struct snow_door_vars *)&(work)->vars)
/* the unit vector along the chains, from the door to the hook */
#define CHAIN_WAY(work) (&(work)->scl)

enum snow_door_mode { DOOR_SHUT, DOOR_FALLING, DOOR_LANDING, DOOR_DOWN };
/* how far the landing has gone. the first draw at DOOR_LANDED raises the
 * puffs */
enum snow_door_landed { DOOR_IN_AIR, DOOR_LANDED, DOOR_PUFFED };

static NJS_POINT3 snow_door_point(struct task_work *work, Angle tilt,
                                  NJS_POINT3 point);

/* the chains hang from snow_door_hook and meet the door at snow_door_chain_1
 * and snow_door_chain_2. the draw measures their way and length from
 * snow_door_top to the hook. each point lies in the door's frame as the door
 * lies open, x out from the hinge. the draw turns the door's points a
 * quarter turn less the door's opening about z. it turns the hook a whole
 * quarter turn, which holds it where the shut door's frame puts it */
static NJS_POINT3 snow_door_chain_1 = {110.0F, 0.0F, 25.0F};
static NJS_POINT3 snow_door_chain_2 = {110.0F, 0.0F, -25.0F};
static NJS_POINT3 snow_door_hook = {125.0F, 0.0F, 25.0F};
static NJS_POINT3 snow_door_top = {110.0F, 0.0F, 25.0F};
/* points on the door where the puffs rise as it lands */
static NJS_POINT3 snow_door_puffs[PUFFS] = {
    {110.0F, 0.0F, 30.0F},  {110.0F, 0.0F, -30.0F}, {120.0F, 0.0F, 20.0F},
    {120.0F, 0.0F, -20.0F}, {120.0F, 0.0F, 0.0F},
};
/* the door is a slab 120 tall that turns with its opening. a ball stands where
 * each chain meets the shut door, and stays there as the door opens */
static struct collision_shape snow_door_shapes[3] = {
    {SHAPE_RECTANGLE2 | SHAPE_TURN_YZX,
     0,
     PUSH_ALL,
     {2.5F, 60.0F, 0.0F},
     2.5F,
     60.0F,
     30.0F,
     {0, 0x7fff, 0},
     0,
     NULL},
    {SHAPE_SPHERE,
     0,
     PUSH_FIRM(PUSH_ALL) | PUSH_ALL,
     {0.0F, 110.0F, 25.0F},
     5.0F,
     0.0F,
     0.0F,
     {0},
     0,
     NULL},
    {SHAPE_SPHERE,
     0,
     PUSH_FIRM(PUSH_ALL) | PUSH_ALL,
     {0.0F, 110.0F, -25.0F},
     5.0F,
     0.0F,
     0.0F,
     {0},
     0,
     NULL},
};

static void snow_door_destroy(struct task *task)
{
}

/* draw the door, and each chain with the link count the last draw measured.
 * the first draw after the door lands raises the puffs */
static void snow_door_draw(struct task *task)
{
    struct task_work *work = task->work;
    NJS_POINT3 hook;
    NJS_POINT3 end;
    int links;
    int i;
    Float span;

    if (placement_try_leave_by(task, DOOR_RANGE)) {
        return;
    }
    links = VARS(work)->links;
    hook = snow_door_point(work, DOOR_OPEN, snow_door_hook);
    end =
        snow_door_point(work, -VARS(work)->opening + DOOR_OPEN, snow_door_top);
    CHAIN_WAY(work)->x = hook.x - end.x;
    CHAIN_WAY(work)->y = hook.y - end.y;
    CHAIN_WAY(work)->z = hook.z - end.z;
    njUnitVector(CHAIN_WAY(work));
    span = njDistanceP2P(&hook, &end);
    /* a multiplication by 1/LINK_SPAN, 0x3e06bca1, as SHC's code under
     * -aggressive=2 does */
    VARS(work)->links = (span * (1.0F / LINK_SPAN)) + LINK_EXTRA;
    texture_set_stage();
    /* @hack type: njPushMatrix's argument as a bare 0, which makes the stack
     * frame 56 bytes (ADD #-56,R15 at 0x0c9085d8); as NULL, it is 52 bytes
     * and 222 of 364 words differ */
    njPushMatrix(0);
    njTranslateV(NULL, &work->pos);
    TURN(njRotateY, work->ang.y);
    TURN(njRotateZ, -VARS(work)->opening);
    draw_tree(&snow_door_object);
    njPopMatrix(1);
    texture_set_stage();
    end = snow_door_point(work, -VARS(work)->opening + DOOR_OPEN,
                          snow_door_chain_1);
    for (i = 0; i < links; i++) {
        njPushMatrix(NULL);
        njTranslate(NULL, end.x + (CHAIN_WAY(work)->x * i * LINK_SPAN),
                    end.y + (CHAIN_WAY(work)->y * i * LINK_SPAN),
                    end.z + (CHAIN_WAY(work)->z * i * LINK_SPAN));
        TURN(njRotateY, work->ang.y);
        TURN(njRotateZ, (-VARS(work)->opening >> 1) + 0x7fff);
        draw_tree(&snow_door_link_object);
        njPopMatrix(1);
    }
    end = snow_door_point(work, -VARS(work)->opening + DOOR_OPEN,
                          snow_door_chain_2);
    for (i = 0; i < links; i++) {
        njPushMatrix(NULL);
        njTranslate(NULL, end.x + (CHAIN_WAY(work)->x * i * LINK_SPAN),
                    end.y + (CHAIN_WAY(work)->y * i * LINK_SPAN),
                    end.z + (CHAIN_WAY(work)->z * i * LINK_SPAN));
        TURN(njRotateY, work->ang.y);
        TURN(njRotateZ, (-VARS(work)->opening >> 1) + 0x7fff);
        draw_tree(&snow_door_link_object);
        njPopMatrix(1);
    }
    if (VARS(work)->landed == DOOR_LANDED) {
        NJS_POINT3 puff;
        NJS_VECTOR rise = {0.0F, 1.0F, 0.0F};

        /* @hack spelling: the increment written out, which keeps links in R11,
         * njPushMatrix's address in R12, draw_tree's in R9 and njPopMatrix's
         * in R10 (0x0c908602, 0x0c908688, 0x0c9086bc, 0x0c9086c2); as i++,
         * each pair trades and 19 of 364 words differ */
        for (i = 0; i < PUFFS; i = i + 1) {
            puff = snow_door_point(work, 0, snow_door_puffs[i]);
            snow_create(&puff, &rise, PUFF_SIZE);
        }
        VARS(work)->landed = DOOR_PUFFED;
    }
}

static void snow_door_run(struct task *task)
{
    struct task_work *work = task->work;
    struct collision_work *collision = work->collision;

    if (placement_try_leave_by(task, DOOR_RANGE)) {
        return;
    }
    if (!placement_editing(task)) {
        switch (work->mode) {
        case DOOR_SHUT:
            if (big_icicle_landed()) {
                sound_play_for(SE_DOOR_FALL, (int)work, 1, 0,
                               DOOR_FALL_UPDATES);
                work->mode = DOOR_FALLING;
            } else {
                collision->shapes->ang.z = VARS(work)->opening;
            }
            break;
        case DOOR_FALLING:
            /* @bug logic: the opening is unsigned, so the test against 0
             * always holds. SHC folds it to the 1 it keeps in R11 (MOV #1 at
             * 0x0c90897e), and tests that (TST R11,R11 at 0x0c9089ea) */
            if (VARS(work)->opening >= 0 && VARS(work)->opening < DOOR_OPEN) {
                VARS(work)->fall_speed += DOOR_FALL_GAIN;
                VARS(work)->opening += VARS(work)->fall_speed;
                collision->shapes->ang.z = VARS(work)->opening;
            }
            if (VARS(work)->opening >= DOOR_OPEN &&
                VARS(work)->landed == DOOR_IN_AIR) {
                VARS(work)->opening = DOOR_OPEN;
                VARS(work)->landed = DOOR_LANDED;
                work->mode = DOOR_LANDING;
                collision->shapes->ang.z = VARS(work)->opening;
            }
            work->ang.z = -VARS(work)->opening;
            break;
        case DOOR_LANDING:
            sound_play(SE_DOOR_LAND, 0, 0, 0);
            work->mode = DOOR_DOWN;
            break;
        case DOOR_DOWN:
            break;
        }
    }
    snow_door_draw(task);
    if (!placement_editing(task)) {
        collision_register(work);
    }
}

void snow_door_step(struct task *task)
{
    struct task_work *work = task->work;

    if (!placement_try_leave_by(task, DOOR_RANGE)) {
        VARS(work)->opening = 0;
        VARS(work)->fall_speed = 0;
        VARS(work)->links = 0;
        VARS(work)->landed = DOOR_IN_AIR;
        if (!placement_editing(task)) {
            collision_init(task, snow_door_shapes, COLLISION_OBJECT, 1, 3);
            work->collision->radius = 150.0F;
        }
        task->step = snow_door_run;
        task->draw = snow_door_draw;
        task->destructor = snow_door_destroy;
    }
}

/* where point, in the door's frame, lies in the world with that frame
 * turned tilt about z */
static NJS_POINT3 snow_door_point(struct task_work *work, Angle tilt,
                                  NJS_POINT3 point)
{
    NJS_POINT3 out;

    njPushMatrix(&_unit_matrix_);
    TURN(njRotateY, work->ang.y);
    TURN(njRotateZ, tilt);
    njCalcPoint(NULL, &point, &out);
    njPopMatrix(1);
    out.x += work->pos.x;
    out.y += work->pos.y;
    out.z += work->pos.z;
    return out;
}
