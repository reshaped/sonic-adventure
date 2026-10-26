/* O TOROKKO mine cart that carries player 0 along one of three tracks */
#include <mathf.h>
#include <ninja.h>
#include <sg_xpt.h>

#include "adv02/mystic_ruins.h"
#include "adv02/texlists.h"
#include "adv02/truck.h"
#include "collision/world.h"
#include "debug/editor.h"
#include "debug/print.h"
#include "draw/draw.h"
#include "draw/model.h"
#include "draw/pose.h"
#include "draw/tree.h"
#include "object/placement.h"
#include "player/player.h"
#include "progress/progress.h"
#include "sound/sound.h"
#include "stage/stage.h"
#include "system/task.h"
#include "system/util.h"

extern NJS_OBJECT truck_object;
/* the truck, open at the action's last frame and shut at frame 0 */
/* @hack type: the action declared as an array, and read through ->. SHC then
 * loads its address and reads its motion 4 bytes past it (MOV.L @(4,R3),R2 at
 * 0x0c907f74). declared as one, 3 of truck_reset's 64 words differ */
extern NJS_ACTION truck_action[];
/* the wheels' objects, which give only the wheels' positions */
/* @hack type: both declared as arrays, so SHC pools each one's address and adds
 * 8 to reach its pos (ADD #8,R5 at 0x0c9084a4 and 0x0c9084c4). as NJS_OBJECTs
 * read through ., 83 of truck_draw's 132 words differ */
extern NJS_OBJECT truck_back_wheel_object[];
extern NJS_OBJECT truck_front_wheel_object[];
/* @hack type: the wheels' model declared as an array, which SHC keeps in R10
 * for all four draws (MOV.L at 0x0c908462). declared as one and passed as its
 * address, 112 of truck_draw's 132 words differ */
extern NJS_MODEL truck_wheel_model[];
extern NJS_OBJECT truck_world_object;
extern NJS_ACTION truck_waterfall_track;
extern NJS_ACTION truck_cave_track;
extern NJS_ACTION truck_jungle_track;

/* the task lives while the camera or player 0 is within 500 units.
 * placement_try_leave_by takes the range squared. the collision world tests
 * the truck only while a player is within NEAR */
#define RANGE (500.0F * 500.0F)
#define NEAR 50.0F

/* the seat lies at SEAT_X and SEAT_Z relative to the truck. player 0 sits in
 * the truck after more than SEAT_STEPS steps on it within SEAT_REACH of the
 * seat. the steps count from truck_reset, in a row or not. the reach is
 * squared */
#define SEAT_X (-1.25203F)
#define SEAT_Z (-0.14892F)
#define SEAT_REACH (6.0F * 6.0F)
#define SEAT_STEPS 2

/* the action frames a step while the truck shuts and while it runs its track,
 * and the wheels' turn a step */
#define CLOSE_SPEED 0.8F
#define RUN_SPEED 0.1F
#define WHEEL_SPEED 0x1000

/* the steps into the run at which the story goes where the track leads */
#define RIDE_STEPS 120
/* the station's entrance that the jungle track leads to */
#define STATION_ENTRANCE 2

/* a quarter turn. in the truck, player 0's heading is the angle about y of the
 * truck's x axis, and a quarter turn */
#define QUARTER_TURN 0x4000

/* the truck's flags in the collision world */
#define TRUCK_WORLD_FLAGS (WORLD_MEDIUM | WORLD_MOVES)

/* the sound the truck makes as it shuts, and the one it makes while it runs */
#define SE_CLOSE 589
#define SE_RUN 590

enum truck_track { TRACK_WATERFALL, TRACK_CAVE, TRACK_JUNGLE, TRACK_COUNT };

enum truck_mode { TRUCK_INIT, TRUCK_ACTIVE };

/* the truck's states, in work->submode */
enum truck_state { TRUCK_WAITING, TRUCK_CLOSING, TRUCK_RUNNING };

struct truck_vars {
    NJS_OBJECT *object; /* the truck's object in the collision world */
    Sint16 wheels; /* the wheels' angle */
    Sint16 track; /* an enum truck_track */
    /* the frame of truck_action, or of the track's action once the truck
     * runs */
    Float frame;
};

#define VARS(work) ((struct truck_vars *)&(work)->vars)
/* TRUE once player 0 has sat in the truck, until truck_reset */
#define RIDING(work) ((work)->tick)

static void truck_init(struct task *task);
static void truck_reset(struct task *task);
static void truck_add_to_world(struct task *task);
static void truck_update(struct task *task);
static void truck_draw(struct task *task);
static void truck_destroy(struct task *task);
static Bool truck_open(struct task_work *work);

/* the tracks, by the set entry's scale x, and their names as the set editor
 * shows them */
static NJS_ACTION *truck_tracks[TRACK_COUNT] = {
    &truck_waterfall_track,
    &truck_cave_track,
    &truck_jungle_track,
};
static char *truck_track_names[TRACK_COUNT] = {
    "at side of TAKI of A",
    "in cave of A",
    "in C part",
};

/* the event flag that opens the waterfall and jungle tracks for each
 * character, or -1 for none, which leaves them shut */
static int truck_flags[CHARACTER_COUNT] = {
    FLAG_SONIC_MR_TRUCK,
    -1,
    FLAG_TAILS_MR_TRUCK,
    FLAG_KNUCKLES_MR_TRUCK,
    -1,
    FLAG_AMY_MR_TRUCK,
    FLAG_E102_MR_TRUCK,
    FLAG_BIG_MR_TRUCK,
};

/* player 0's position relative to the truck as the truck finishes shutting.
 * the run keeps player 0 there */
static NJS_POINT3 truck_seat = {0.0F, 0.0F, 0.0F};
Bool truck_ridden = FALSE;
/* the truck's x axis, whose direction on the track sets player 0's heading
 * during the run */
static NJS_VECTOR truck_ahead = {1.0F, 0.0F, 0.0F};

/* a truck whose track is shut is neither drawn nor run, and the collision world
 * skips it. while the set editor holds the task, the step rebuilds the truck
 * from the set entry each step. it labels the y angle and scale x it reads */
void truck_step(struct task *task)
{
    struct task_work *work = task->work;

    if (placement_try_leave_by(task, RANGE)) {
        return;
    }
    if (placement_editing(task)) {
        switch (work->mode) {
        case TRUCK_INIT:
            truck_init(task);
            break;
        case TRUCK_ACTIVE:
            truck_destroy(task);
            truck_reset(task);
            truck_draw(task);
            truck_update(task);
            debug_print(EDITOR_LABEL(EDITOR_ROW_ANG_Y), "-> ROTATE Y");
            debug_print(EDITOR_LABEL(EDITOR_ROW_SCL_X), "-> RAIL KIND(%s)",
                        truck_track_names[VARS(work)->track]);
            break;
        /* @hack redundant: an empty default. it keeps the ADD #12,R15 that pops
         * debug_print's arguments ahead of the BRA past the switch, with a NOP
         * in the BRA's slot (0x0c907e2a to 0x0c907e2e); without it, 16 of 88
         * words differ */
        default:;
        }
    } else {
        switch (work->mode) {
        case TRUCK_INIT:
            truck_init(task);
            break;
        case TRUCK_ACTIVE:
            if (!truck_open(work)) {
                work->flags &= ~WORLD_TESTED;
            } else {
                truck_draw(task);
                truck_update(task);
            }
            break;
        }
    }
}

static void truck_init(struct task *task)
{
    struct task_work *work = task->work;

    task->draw = truck_draw;
    task->destructor = truck_destroy;
    truck_reset(task);
    work->mode = TRUCK_ACTIVE;
}

static void truck_reset(struct task *task)
{
    struct task_work *work = task->work;
    NJS_MDATA2 *data;

    VARS(work)->track = (int)work->scl.x;
    if (VARS(work)->track < TRACK_WATERFALL) {
        VARS(work)->track = TRACK_WATERFALL;
    } else if (VARS(work)->track >= TRACK_COUNT) {
        VARS(work)->track = TRACK_JUNGLE;
    }
    data = truck_tracks[VARS(work)->track]->motion->mdata;
    /* @hack type: the first key's Float key[3] read whole as an NJS_POINT3,
     * which SHC copies as three words; as three element stores, the
     * function runs 16 bytes long and 39 of 64 words differ */
    work->pos = *(NJS_POINT3 *)((NJS_MKEY_F *)data->p[0])->key;
    work->ang.y = ((NJS_MKEY_A *)data->p[1])->key[1];
    truck_add_to_world(task);
    VARS(work)->wheels = 0;
    VARS(work)->frame = truck_action->motion->nbFrame - 1.0F;
    RIDING(work) = FALSE;
    TASK_TIMER(work) = 0;
    work->submode = TRUCK_WAITING;
}

static void truck_add_to_world(struct task *task)
{
    NJS_OBJECT *from;
    struct task_force *force = task->force;
    struct task_work *work = task->work;
    NJS_OBJECT *object;

    force->turn.x = 0;
    force->turn.y = 0;
    force->turn.z = 0;
    force->speed.x = 0.0F;
    force->speed.y = 0.0F;
    force->speed.z = 0.0F;
    from = &truck_world_object;
    VARS(work)->object = object = world_object_alloc();
    object->evalflags = from->evalflags;
    object->scl[0] = 1.0F;
    object->scl[1] = 1.0F;
    object->scl[2] = 1.0F;
    object->ang[0] = 0;
    object->ang[1] = work->ang.y;
    object->ang[2] = 0;
    object->pos[0] = work->pos.x;
    object->pos[1] = work->pos.y;
    object->pos[2] = work->pos.z;
    object->model = from->model;
    object->child = NULL;
    object->sibling = NULL;
    world_add(TRUCK_WORLD_FLAGS, task, object);
}

/* the truck holds player 0 still once player 0 sits in it, shuts, and then runs
 * its track with player 0 aboard. RIDE_STEPS into the run, story_go_to takes
 * player 0 where the track leads */
static void truck_update(struct task *task)
{
    struct task_work *work = task->work;
    NJS_POINT3 local;
    Float z;
    Float x;

    switch (work->submode) {
    case TRUCK_WAITING: {
        struct task_work *player = player_get_task(0)->work;

        if (player_stands_on(0, task)) {
            njPushMatrix(&_unit_matrix_);
            njTranslateV(NULL, &work->pos);
            TURN(njRotateY, work->ang.y);
            njInvertMatrix(NULL);
            njCalcPoint(NULL, &player->pos, &local);
            njPopMatrix(1);
            x = local.x - SEAT_X;
            z = local.z - SEAT_Z;
            if ((x * x) + (z * z) < SEAT_REACH) {
                if (!RIDING(work)) {
                    TASK_TIMER(work)++;
                    if (TASK_TIMER(work) > SEAT_STEPS) {
                        player_set_mode(0, PLAYER_MODE_STILL);
                        player_set_heading(0, work->ang.y + QUARTER_TURN);
                        RIDING(work) = TRUE;
                        work->submode = TRUCK_CLOSING;
                        TASK_TIMER(work) = 0;
                    }
                } else {
                    TASK_TIMER(work) = 0;
                }
            }
        } else {
            RIDING(work) = FALSE;
        }
        break;
    }
    case TRUCK_CLOSING: {
        struct task_work *player = player_get_task(0)->work;

        VARS(work)->frame -= CLOSE_SPEED;
        if (VARS(work)->frame <= 0.0F) {
            njPushMatrix(&_unit_matrix_);
            njTranslateV(NULL, &work->pos);
            TURN(njRotateY, work->ang.y);
            njInvertMatrix(NULL);
            njCalcPoint(NULL, &player->pos, &truck_seat);
            njPopMatrix(1);
            truck_ridden = TRUE;
            VARS(work)->frame = 0.0F;
            sound_play(SE_CLOSE, 0, 0, 0);
            work->submode = TRUCK_RUNNING;
        }
        break;
    }
    case TRUCK_RUNNING: {
        struct task_work *player = player_get_task(0)->work;

        njPushMatrix(&_unit_matrix_);
        pose_action(NULL, truck_tracks[VARS(work)->track], VARS(work)->frame, 0,
                    NULL);
        pose_get_matrix(0, NULL);
        njCalcPoint(NULL, &truck_seat, &player->pos);
        njCalcVector(NULL, &truck_ahead, &local);
        player_set_heading(0,
                           (int)(atan2f(-local.z, local.x) * ANGLE_PER_RADIAN) +
                               QUARTER_TURN);
        njPopMatrix(1);
        VARS(work)->wheels += WHEEL_SPEED;
        VARS(work)->frame += RUN_SPEED;
        if (VARS(work)->frame >=
            truck_tracks[VARS(work)->track]->motion->nbFrame - 1.0F) {
            VARS(work)->frame =
                truck_tracks[VARS(work)->track]->motion->nbFrame - 1.0F;
        }
        sound_play_for(SE_RUN, (int)work, 1, 0, 2);
        TASK_TIMER(work)++;
        if (TASK_TIMER(work) == RIDE_STEPS) {
            if (VARS(work)->track == TRACK_WATERFALL) {
                stage_set_entrance(0);
                story_go_to(STAGE_MYSTIC_RUINS, MYSTIC_RUINS_JUNGLE);
            } else if (VARS(work)->track == TRACK_JUNGLE) {
                stage_set_entrance(STATION_ENTRANCE);
                story_go_to(STAGE_MYSTIC_RUINS, MYSTIC_RUINS_STATION);
            } else if (VARS(work)->track == TRACK_CAVE) {
                stage_set_entrance(0);
                story_go_to(STAGE_ALIFE_MR, 0);
            }
        }
        break;
    }
    }
    if (player_in_range(&work->pos, NEAR)) {
        work->flags |= WORLD_TESTED;
    } else {
        work->flags &= ~WORLD_TESTED;
    }
}

static void truck_draw(struct task *task)
{
    struct task_work *work = task->work;
    Angle ang;

    if (work->submode == TRUCK_RUNNING) {
        njPushMatrix(NULL);
        pose_action(NULL, truck_tracks[VARS(work)->track], VARS(work)->frame, 0,
                    NULL);
        pose_get_matrix(0, NULL);
        njSetTexture(&mrobj_texlist);
        draw_tree(&truck_object);
        njSetTexture(&mrobj_texlist);
        njPushMatrix(NULL);
        njTranslateV(NULL, (NJS_VECTOR *)truck_back_wheel_object->pos);
        TURN_VIA(njRotateZ, VARS(work)->wheels, ang);
        draw_model(truck_wheel_model);
        njPopMatrix(1);
        njTranslateV(NULL, (NJS_VECTOR *)truck_front_wheel_object->pos);
        TURN_VIA(njRotateZ, VARS(work)->wheels, ang);
        draw_model(truck_wheel_model);
        njPopMatrix(1);
    } else {
        njPushMatrix(NULL);
        njTranslateV(NULL, &work->pos);
        TURN_VIA(njRotateY, work->ang.y, ang);
        njSetTexture(&mrobj_texlist);
        draw_action(truck_action, VARS(work)->frame);
        njSetTexture(&mrobj_texlist);
        njPushMatrix(NULL);
        njTranslateV(NULL, (NJS_VECTOR *)truck_back_wheel_object->pos);
        draw_model(truck_wheel_model);
        njPopMatrix(1);
        njTranslateV(NULL, (NJS_VECTOR *)truck_front_wheel_object->pos);
        draw_model(truck_wheel_model);
        njPopMatrix(1);
    }
}

static void truck_destroy(struct task *task)
{
    struct task_work *work = task->work;

    world_remove(task, VARS(work)->object);
    world_object_free(VARS(work)->object);
}

/* TRUE if the truck's track is open: the cave track always, and the others once
 * player 0's character has its truck flag set */
static Bool truck_open(struct task_work *work)
{
    int flag;

    if (VARS(work)->track == TRACK_WATERFALL ||
        VARS(work)->track == TRACK_JUNGLE) {
        flag = truck_flags[player_get_character(0)];
        if (flag >= 0) {
            return event_flags[flag];
        }
        return FALSE;
    }
    return TRUE;
}
