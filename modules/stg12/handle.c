/* O KAITENKEY: a valve handle that Amy turns. it counts her whole turns each
 * way under its key */
#include <ninja.h>
#include <sg_xpt.h>

#include "collision/collision.h"
#include "debug/editor.h"
#include "debug/print.h"
#include "draw/model.h"
#include "object/placement.h"
#include "player/amy_handle.h"
#include "player/player.h"
#include "sound/sound.h"
#include "stg12/handle.h"
#include "system/frame.h"
#include "system/task.h"
#include "system/texture.h"
#include "system/util.h"

extern NJS_MODEL handle_model;
extern NJS_MODEL handle_wheel_model;
extern NJS_OBJECT handle_wheel_object;

/* destroyed beyond 1000 units of the camera and player 0 */
#define RANGE (1000.0F * 1000.0F)

/* the handle calls Amy while player 0 is in its reach. the reach runs
 * REACH_ACROSS to either side, and REACH_NEAR to REACH_FAR in front */
#define REACH_ACROSS 5.0F
#define REACH_NEAR 3.0F
#define REACH_FAR 10.0F

/* handle_turn reads the stick's angle as one of QUARTERS quarter turns,
 * numbered from 1. quarter n spans HALF_QUARTER to either side of
 * n * QUARTER */
#define QUARTERS 4
#define QUARTER 0x4000
#define HALF_QUARTER 0x2000

/* i wrapped to an index of the ring's QUARTERS slots */
#define WRAP(i) ((i) & (QUARTERS - 1))

#define SE_TURN 308

enum handle_mode { HANDLE_INIT, HANDLE_RUN };

enum handle_setting { SET_KEY, SET_MOST_RIGHT, SET_MOST_LEFT, SETTINGS };

struct handle_vars {
    Angle stick_ang; /* the way Amy last pushed her stick */
    Uint8 unk_04[4];
    /* @hack type: the settings in an array, so SHC adds 16 to the work's
     * address to read them, as ADD #16,R4 at 0x0c9014e0 does; as three Sint8
     * fields, read through @(R0,R14), 171 of the 213 words of handle_step,
     * handle_init and handle_follow differ */
    Sint8 settings[SETTINGS]; /* from the set entry's scale */
};

#define VARS(work) ((struct handle_vars *)&(work)->vars)

/* the turns under one key. right and left count the whole turns each way */
struct handle_turns {
    int quarter; /* the quarter the stick last entered, or 0 */
    /* a ring of the last QUARTERS quarters the stick entered. a step the
     * stick isn't pushed adds a 0 */
    int quarters[QUARTERS];
    int next; /* the ring's oldest slot, which takes the next entry */
    int right;
    int left;
};

static void handle_init(struct task *task);
static void handle_draw(struct task *task);
static void handle_destroy(struct task *task);
static Bool handle_in_reach(struct task_work *work);
static void handle_follow(struct task_work *work);
static void handle_turn(int key, Angle ang);
static void handle_let_go(int key);
static void handle_reset(int key);

/* the ring holds a whole turn when its quarters, oldest first, match a row of
 * one of these. handle_right_turns holds the turns right, and
 * handle_left_turns the turns left */
static const int handle_right_turns[QUARTERS][QUARTERS] = {
    {1, 2, 3, 4},
    {2, 3, 4, 1},
    {3, 4, 1, 2},
    {4, 1, 2, 3},
};
static const int handle_left_turns[QUARTERS][QUARTERS] = {
    {4, 3, 2, 1},
    {3, 2, 1, 4},
    {2, 1, 4, 3},
    {1, 4, 3, 2},
};

static struct collision_shape handle_shape = {
    SHAPE_CYLINDER,
    0,
    PUSH_ALL | PUSH_FIRM(PUSH_ALL),
    {0.0F, 5.0F, 0.0F},
    3.0F,
    5.0F,
    0.0F,
    {0},
    0,
    NULL,
};

static struct handle_turns handle_turns[HANDLE_KEYS];

/* @hack spelling: the registration in a helper, which SHC expands at both of
 * handle_step's calls. the step then saves only R14 and R13 (0x0c9011e0,
 * 0x0c9011e4). with collision_register(work) written in the step, it saves R12
 * too, and 85 of its 90 words differ */
static void handle_register(struct task *task)
{
    struct task_work *work = task->work;

    collision_register(work);
}

void handle_step(struct task *task)
{
    struct task_work *work = task->work;

    if (placement_try_leave_by(task, RANGE)) {
        return;
    }
    if (!placement_editing(task)) {
        switch (work->mode) {
        case HANDLE_INIT:
            handle_init(task);
            handle_reset(VARS(work)->settings[SET_KEY]);
            break;
        case HANDLE_RUN:
            handle_draw(task);
            handle_follow(work);
            handle_register(task);
            break;
        }
    } else {
        switch (work->mode) {
        case HANDLE_INIT:
            handle_init(task);
            break;
        case HANDLE_RUN:
            debug_print(EDITOR_LABEL(EDITOR_ROW_ANG_X), "-> No Use");
            debug_print(EDITOR_LABEL(EDITOR_ROW_ANG_Y), "-> ROTATE Y");
            debug_print(EDITOR_LABEL(EDITOR_ROW_ANG_Z), "-> No Use");
            debug_print(EDITOR_LABEL(EDITOR_ROW_SCL_X), "-> Key ID");
            debug_print(EDITOR_LABEL(EDITOR_ROW_SCL_Y), "-> RIGHT MAX");
            debug_print(EDITOR_LABEL(EDITOR_ROW_SCL_Z), "-> LEFT MAX");
            handle_draw(task);
            handle_register(task);
            break;
        }
    }
}

static void handle_init(struct task *task)
{
    struct task_work *work = task->work;

    task->draw = handle_draw;
    task->destructor = handle_destroy;
    collision_init(task, &handle_shape, COLLISION_OBJECT, 1, 1);
    VARS(work)->stick_ang = 0;
    VARS(work)->settings[SET_KEY] = (int)work->scl.x & (HANDLE_KEYS - 1);
    VARS(work)->settings[SET_MOST_RIGHT] = work->scl.y;
    VARS(work)->settings[SET_MOST_LEFT] = work->scl.z;
    work->mode = HANDLE_RUN;
}

static void handle_draw(struct task *task)
{
    struct task_work *work = task->work;
    Angle ang;

    if (frame_step == 0) {
        texture_set_stage();
        njPushMatrix(NULL);
        njTranslateV(NULL, &work->pos);
        TURN_VIA(njRotateY, work->ang.y, ang);
        draw_model(&handle_model);
        njTranslateV(NULL, (NJS_VECTOR *)handle_wheel_object.pos);
        TURN_VIA(njRotateZ, -QUARTER - VARS(work)->stick_ang, ang);
        draw_model(&handle_wheel_model);
        njPopMatrix(1);
    }
}

static void handle_destroy(struct task *task)
{
}

/* TRUE while player 0 is in the handle's reach, measured in its turned axes */
static Bool handle_in_reach(struct task_work *work)
{
    NJS_VECTOR to;
    Angle ang;

    to = player_work[0]->pos;
    njSubVector(&to, &work->pos);
    njPushMatrix(&_unit_matrix_);
    TURN_VIA(njRotateY, -work->ang.y, ang);
    njCalcVector(NULL, &to, &to);
    njPopMatrix(1);
    return to.x > -REACH_ACROSS && to.x < REACH_ACROSS && to.z > REACH_NEAR &&
           to.z < REACH_FAR;
}

static void handle_follow(struct task_work *work)
{
    struct amy_handle *handle;

    if (!handle_in_reach(work)) {
        return;
    }
    /* @hack spelling: handle assigned in the test, which SHC tests in R0 (TST
     * R0,R0 at 0x0c9014cc) and copies to R13 in the branch's delay slot;
     * assigned first, 3 of 81 words differ */
    if ((handle = amy_offer_handle(&work->pos, work->ang.y)) == NULL) {
        return;
    }
    /* @hack redundant: held tested to no end, its load and CMP/EQ #1 at
     * 0x0c9014d2 and 0x0c9014d4; without it, 66 of 81 words differ */
    if (handle->held == TRUE) {
    }
    if (handle->pushed == TRUE) {
        VARS(work)->stick_ang = handle->stick_ang;
        handle_turn(VARS(work)->settings[SET_KEY], handle->stick_ang);
        if (handle_turns[VARS(work)->settings[SET_KEY]].right >
            VARS(work)->settings[SET_MOST_RIGHT]) {
            handle_turns[VARS(work)->settings[SET_KEY]].right = 0;
        }
        if (handle_turns[VARS(work)->settings[SET_KEY]].left >
            VARS(work)->settings[SET_MOST_LEFT]) {
            handle_turns[VARS(work)->settings[SET_KEY]].left = 0;
        }
    } else {
        handle_let_go(VARS(work)->settings[SET_KEY]);
    }
}

/* record ang's quarter on key's ring, unless the stick is still in the
 * quarter last recorded. when the ring then holds a whole turn, the turn is
 * counted, SE_TURN plays and the ring empties. the next turn then takes
 * QUARTERS new quarters */
static void handle_turn(int key, Angle ang)
{
    struct handle_turns *turns;
    int i;
    int found;
    int next;

    /* @hack redundant: a copy of ang to key that never runs, which keeps the
     * first loop's row of handle_right_turns at R15+4 and the offset of the
     * ring's third slot in R8 (0x0c9015fc, 0x0c901606); without it, the two
     * trade and 48 of 195 words differ */
    if (0) {
        key = ang;
    }
    turns = &handle_turns[key];
    ang = (Uint16)(ang - HALF_QUARTER);
    ang = (ang / QUARTER) + 1;
    /* @hack redundant: the else after the return, which puts turns->quarters'
     * address in R1 at 0x0c9015da; without it, R2 takes it and 24 of 195
     * words differ */
    if (ang == turns->quarter) {
        return;
    } else {
        /* @hack spelling: found and i set ahead of the ring's update, which
         * orders the ANDs and shifts that make the first loop's slot offsets:
         * MOV R4,R8 after AND R12,R1 at 0x0c901606, and SHLL2 R8 last at
         * 0x0c901618; set at the loop, 8 of 195 words differ */
        found = FALSE;
        i = 0;
        turns->quarter = ang;
        turns->quarters[turns->next] = ang;
        turns->next = WRAP(turns->next + 1);
        next = turns->next;
        /* @hack spelling: i = i + 1, which keeps i in R6 and found in R7
         * (0x0c9015e6, 0x0c9015e8); as i++, the two trade and 6 of 195 words
         * differ */
        for (; i < QUARTERS; i = i + 1) {
            if (turns->quarters[WRAP(next)] == handle_right_turns[i][0] &&
                turns->quarters[WRAP(next + 1)] == handle_right_turns[i][1] &&
                turns->quarters[WRAP(next + 2)] == handle_right_turns[i][2] &&
                turns->quarters[WRAP(next + 3)] == handle_right_turns[i][3]) {
                found = TRUE;
                break;
            }
        }
        if (found) {
            turns->quarters[0] = 0;
            turns->quarters[1] = 0;
            turns->quarters[2] = 0;
            turns->quarters[3] = 0;
            turns->right++;
            sound_play(SE_TURN, 0, 0, 0);
        }
        /* @hack redundant: a store of 1 to turns->quarter that never runs,
         * which puts 1 in R11 and 4 in R10 (0x0c90161e, 0x0c901620); without
         * it, the two trade and 33 of 195 words differ */
        if (0) {
            turns->quarter = 1;
        }
        next = turns->next;
        /* @hack spelling: i = i + 1, which keeps the first loop's row in R5
         * and turns->quarters in R4 (0x0c901624, 0x0c901626); as i++, the two
         * trade and 13 of 195 words differ */
        for (found = FALSE, i = 0; i < QUARTERS; i = i + 1) {
            if (turns->quarters[WRAP(next)] == handle_left_turns[i][0] &&
                turns->quarters[WRAP(next + 1)] == handle_left_turns[i][1] &&
                turns->quarters[WRAP(next + 2)] == handle_left_turns[i][2] &&
                turns->quarters[WRAP(next + 3)] == handle_left_turns[i][3]) {
                found = TRUE;
                break;
            }
        }
        if (found) {
            turns->quarters[0] = 0;
            turns->quarters[1] = 0;
            turns->quarters[2] = 0;
            turns->quarters[3] = 0;
            turns->left++;
            sound_play(SE_TURN, 0, 0, 0);
        }
    }
}

/* mark a step the stick isn't pushed with a 0 on key's ring. a whole turn
 * needs QUARTERS new quarters after it */
static void handle_let_go(int key)
{
    struct handle_turns *turns = &handle_turns[key];

    turns->quarters[turns->next] = 0;
    turns->next = WRAP(turns->next + 1);
}

static void handle_reset(int key)
{
    struct handle_turns *turns = &handle_turns[key];

    turns->quarter = 0;
    turns->quarters[0] = 0;
    turns->quarters[1] = 0;
    turns->quarters[2] = 0;
    turns->quarters[3] = 0;
    turns->next = 0;
    turns->right = 0;
    turns->left = 0;
}

void handle_get_turns(int key, int *right, int *left)
{
    *right = handle_turns[key].right;
    *left = handle_turns[key].left;
}
