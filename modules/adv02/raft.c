/* the raft the object list names O IKADA, which takes player 0 out to the Egg
 * Carrier */
#include <ninja.h>
#include <sg_xpt.h>

#include "adv02/raft.h"
#include "adv02/texlists.h"
#include "collision/collision.h"
#include "debug/editor.h"
#include "debug/print.h"
#include "draw/draw.h"
#include "object/placement.h"
#include "player/player.h"
#include "progress/progress.h"
#include "stage/stage.h"
#include "system/task.h"
#include "system/util.h"

/* @hack type: declared an array, not the one NJS_ACTION it is, so SHC pools
 * its address at 0x0c90edd0 and reads motion through it, MOV.L @(4,R3),R2 at
 * 0x0c90eca8; declared as one, it pools motion's address, and 6 of
 * raft_update's 495 words and 1 pool word differ */
extern NJS_ACTION raft_action[];

/* placement_try_leave_by ends the task once the camera and player 0 are
 * both beyond 300 */
#define RANGE (300.0F * 300.0F)

#define RADIUS 44.0F /* the collision record's radius */

/* a touch while a player is within BOARD_RANGE of the raft's center counts a
 * step, and one while none is starts the count over. once the count passes
 * BOARD_STEPS, the raft sails */
#define BOARD_RANGE 30.0F
#define BOARD_STEPS 30

/* GO_STEPS steps into the sail, the game goes to the Egg Carrier. the rider is
 * let go after SAIL_STEPS, and the raft is back at rest BACK_STEPS steps after
 * that */
#define GO_STEPS 120
#define SAIL_STEPS 540
#define BACK_STEPS 60

/* where the raft takes the player: act 0 of the Egg Carrier, at entrance 6 */
#define TO_ACT 0
#define TO_ENTRANCE 6

/* how the raft floats, in a block from the heap. the tilt and the sink spring
 * back to 0, and the bob and the roll ride the swell */
struct raft_float {
    NJS_POINT3 home; /* the rest position */
    NJS_POINT3 rider; /* where the rider stands, in the raft's frame */
    Angle tilt_x;
    Angle tilt_z;
    Float sink;
    Float sail; /* the speed it sails at */
    Float sink_speed;
    Float bob; /* the height the swell lifts it */
    Angle roll; /* the turn about z the swell gives it */
    Angle tilt_speed_x;
    Angle tilt_speed_z;
    Angle phase; /* the swell's: bob takes its sine, and roll its cosine */
};

struct raft_vars {
    /* the action frames the raft plays each step, which push the sail too */
    Float speed;
    Float frame;
    struct raft_float *afloat;
};

#define VARS(work) ((struct raft_vars *)&(work)->vars)

enum raft_mode { RAFT_INIT, RAFT_RUN };

/* the raft's states, in work->submode. work->timer counts the steps of each,
 * as a Uint16 */
enum raft_state { RAFT_AFLOAT, RAFT_SAILING, RAFT_BACK };

static void raft_init(struct task *task);
static void raft_reset(struct task *task);
static void raft_update(struct task *task);
static void raft_draw(struct task *task);
static void raft_destroy(struct task *task);
static Bool raft_open(void);

/* the rafts started and not yet destroyed. no code reads it but the empty
 * tests in raft_init and raft_destroy */
static int raft_count = 0;

/* the deck and the rudder, which push and stand firm against every group */
enum { DECK_SHAPE, RUDDER_SHAPE };
static struct collision_shape raft_shapes[2] = {
    {SHAPE_RECTANGLE2 | SHAPE_TURN_YZX,
     0,
     PUSH_ALL | PUSH_FIRM(PUSH_ALL),
     {0.0F, -1.45F, 0.0F},
     26.0F,
     4.0F,
     21.4F,
     {0},
     0,
     NULL},
    {SHAPE_RECTANGLE2 | SHAPE_TURN_YZX,
     0,
     PUSH_ALL | PUSH_FIRM(PUSH_ALL),
     {-20.0F, 0.0F, 0.0F},
     6.0F,
     5.2F,
     2.5F,
     {0},
     0,
     NULL},
};

/* the flag that makes the raft appear, by character; Eggman and Tikal have
 * none */
static int raft_flags[CHARACTER_COUNT] = {
    FLAG_SONIC_RAFT,
    -1,
    FLAG_TAILS_RAFT,
    FLAG_KNUCKLES_RAFT,
    -1,
    FLAG_AMY_RAFT,
    FLAG_E102_RAFT,
    FLAG_BIG_RAFT,
};

/* each step of the sail, the sail gains raft_sail_push times the action speed,
 * and then keeps raft_sail_drag of itself. the action speed gains
 * raft_speed_up each step, up to raft_speed_most. no code reads dat_0cb2126c or
 * dat_0cb21270 */
static Float raft_sail_push = 0.01F;
static Float raft_sail_drag = 0.9F;
static Float dat_0cb2126c = 1.0F;
static Float dat_0cb21270 = 0.05F;
static Float raft_speed_most = 5.0F;
static Float raft_speed_up = 0.04F;
/* a rider tips the raft. the tilt's speed about x gains raft_tip_x times the
 * rider's z on the raft, and about z loses raft_tip_z times its x. each step,
 * the tilt's speed about x loses raft_tilt_back_x times the tilt, and about z
 * raft_tilt_back_z times it. then it keeps raft_tilt_drag of itself */
static Float raft_tip_x = 2.0F;
static Float raft_tip_z = 2.0F;
static Float raft_tilt_drag = 0.93F;
static Float raft_tilt_back_x = 0.06F;
static Float raft_tilt_back_z = 0.06F;
/* each step, the sink's speed loses raft_sink_back times the sink, and then
 * keeps raft_sink_drag of itself. each step of a touch takes raft_sink_weight
 * from it */
static Float raft_sink_back = 0.07F;
static Float raft_sink_drag = 0.92F;
static Float raft_sink_weight = 0.09F;
/* the swell's greatest roll and bob, afloat and sailing, and the phase it adds
 * each step */
static Float raft_roll_afloat = 200.0F;
static Float raft_roll_sailing = 200.0F;
static Float raft_bob_afloat = 0.4F;
static Float raft_bob_sailing = 0.4F;
static Angle raft_swell_afloat = 0x190;
static Angle raft_swell_sailing = 0x190;

/* the raft appears and runs only once the character's raft flag is set. in the
 * set editor it always runs, put back at rest each step, and labels the y angle
 * field */
void raft_step(struct task *task)
{
    struct task_work *work = task->work;

    if (placement_try_leave_by(task, RANGE)) {
        return;
    }
    if (!placement_editing(task)) {
        if (raft_open()) {
            switch (work->mode) {
            case RAFT_INIT:
                raft_init(task);
                break;
            case RAFT_RUN:
                raft_draw(task);
                raft_update(task);
                break;
            }
        }
    } else {
        switch (work->mode) {
        case RAFT_INIT:
            raft_init(task);
            break;
        case RAFT_RUN:
            raft_reset(task);
            raft_draw(task);
            raft_update(task);
            debug_print(NJM_LOCATION(17, 17), "-> ROTATE Y");
            break;
        }
    }
}

static void raft_init(struct task *task)
{
    struct task_work *work = task->work;

    work->ang.x = 0;
    work->ang.z = 0;
    /* @hack redundant: a test of raft_count with an empty body, which SHC keeps
     * as TST R3,R3 at 0x0c90e8b6; without it, 35 of raft_init's 46 words differ
     */
    if (raft_count == 0) {
    }
    raft_count++;
    VARS(work)->afloat = heap_alloc(sizeof(struct raft_float));
    task->draw = raft_draw;
    task->destructor = raft_destroy;
    collision_init(task, raft_shapes, COLLISION_OBJECT, 1, 2);
    work->collision->radius = RADIUS;
    VARS(work)->frame = 0.0F;
    raft_reset(task);
    work->mode = RAFT_RUN;
}

/* put the raft at rest at its position, level and still */
static void raft_reset(struct task *task)
{
    struct task_work *work = task->work;
    struct raft_float *afloat = VARS(work)->afloat;

    afloat->home = work->pos;
    afloat->tilt_speed_x = 0;
    afloat->tilt_speed_z = 0;
    afloat->sink_speed = 0.0F;
    afloat->phase = 0;
    afloat->bob = 0.0F;
    afloat->roll = 0;
    afloat->tilt_x = 0;
    afloat->tilt_z = 0;
    afloat->sink = 0.0F;
    TASK_TIMER(work) = 0;
    work->submode = RAFT_AFLOAT;
}

/* spring the tilt and the sink back, and ride the swell. afloat, a touch tips
 * the raft toward where player 0 stands and pushes it down. touches near the
 * center set it sailing, as BOARD_RANGE and BOARD_STEPS say. player 0 rides the
 * sail held still, where it boarded */
static void raft_update(struct task *task)
{
    struct task_work *work = task->work;
    struct raft_float *afloat = VARS(work)->afloat;
    struct collision_work *collision = work->collision;
    struct task_work *player;
    NJS_POINT3 local;
    Float x;
    Float z;
    Angle ang;

    afloat->tilt_x += afloat->tilt_speed_x;
    afloat->tilt_z += afloat->tilt_speed_z;
    afloat->tilt_speed_x -= afloat->tilt_x * raft_tilt_back_x;
    afloat->tilt_speed_z -= afloat->tilt_z * raft_tilt_back_z;
    afloat->tilt_speed_x *= raft_tilt_drag;
    afloat->tilt_speed_z *= raft_tilt_drag;
    afloat->sink += afloat->sink_speed;
    afloat->sink_speed -= raft_sink_back * afloat->sink;
    afloat->sink_speed *= raft_sink_drag;
    work->ang.x = afloat->tilt_x;
    work->ang.z = afloat->tilt_z + afloat->roll;
    work->pos.y = afloat->home.y + afloat->sink + afloat->bob;
    collision->shapes[DECK_SHAPE].ang.x = work->ang.x;
    collision->shapes[DECK_SHAPE].ang.z = work->ang.z;
    collision->shapes[RUDDER_SHAPE].ang.x = work->ang.x;
    collision->shapes[RUDDER_SHAPE].ang.z = work->ang.z;
    switch (work->submode) {
    case RAFT_AFLOAT:
        afloat->bob = raft_bob_afloat * njSin(afloat->phase);
        afloat->roll = raft_roll_afloat * njCos(afloat->phase);
        afloat->phase += raft_swell_afloat;
        if (collision->hit != NULL) {
            player = player_get_task(0)->work;
            afloat->sink_speed -= raft_sink_weight;
            njPushMatrix(&_unit_matrix_);
            njTranslateV(NULL, &work->pos);
            TURN_VIA(njRotateY, work->ang.y, ang);
            TURN_VIA(njRotateZ, work->ang.z, ang);
            TURN_VIA(njRotateX, work->ang.x, ang);
            njInvertMatrix(NULL);
            njCalcPoint(NULL, &player->pos, &local);
            njPopMatrix(1);
            afloat->tilt_speed_x += raft_tip_x * local.z;
            afloat->tilt_speed_z -= raft_tip_z * local.x;
            if (player_in_range(&work->pos, BOARD_RANGE)) {
                TASK_TIMER(work)++;
                if (TASK_TIMER(work) > BOARD_STEPS) {
                    VARS(work)->speed = 0.0F;
                    afloat->sail = 0.0F;
                    player_set_mode(0, PLAYER_MODE_STILL);
                    player_set_heading(0, work->ang.y + 0x4000);
                    afloat->rider = local;
                    TASK_TIMER(work) = 0;
                    work->submode = RAFT_SAILING;
                }
            } else {
                TASK_TIMER(work) = 0;
            }
        }
        break;
    case RAFT_SAILING:
        afloat->bob = raft_bob_sailing * njSin(afloat->phase);
        afloat->roll = raft_roll_sailing * njCos(afloat->phase);
        afloat->phase += raft_swell_sailing;
        player = player_get_task(0)->work;
        VARS(work)->frame += VARS(work)->speed;
        if (VARS(work)->frame >= raft_action->motion->nbFrame - 1.0F) {
            VARS(work)->frame = 0.0F;
        }
        x = afloat->sail * njCos(work->ang.y);
        z = -afloat->sail * njSin(work->ang.y);
        work->pos.x += x;
        work->pos.z += z;
        njPushMatrix(&_unit_matrix_);
        njTranslateV(NULL, &work->pos);
        TURN_VIA(njRotateY, work->ang.y, ang);
        TURN_VIA(njRotateZ, work->ang.z, ang);
        TURN_VIA(njRotateX, work->ang.x, ang);
        njCalcPoint(NULL, &afloat->rider, &player->pos);
        njPopMatrix(1);
        afloat->sail += raft_sail_push * VARS(work)->speed;
        afloat->sail *= raft_sail_drag;
        VARS(work)->speed += raft_speed_up;
        if (VARS(work)->speed > raft_speed_most) {
            VARS(work)->speed = raft_speed_most;
        }
        TASK_TIMER(work)++;
        if (TASK_TIMER(work) == GO_STEPS) {
            stage_set_entrance(TO_ENTRANCE);
            story_go_to(STAGE_EGG_CARRIER, TO_ACT);
        }
        if (TASK_TIMER(work) >= SAIL_STEPS) {
            player_set_mode(0, PLAYER_MODE_RESET);
            TASK_TIMER(work) = 0;
            work->submode = RAFT_BACK;
        }
        break;
    case RAFT_BACK:
        TASK_TIMER(work)++;
        if (TASK_TIMER(work) > BACK_STEPS) {
            work->pos = afloat->home;
            afloat->bob = 0.0F;
            afloat->phase = 0;
            afloat->roll = 0;
            afloat->tilt_x = 0;
            afloat->tilt_z = 0;
            afloat->sink = 0.0F;
            TASK_TIMER(work) = 0;
            work->submode = RAFT_AFLOAT;
        }
        break;
    }
    collision_register(work);
}

static void raft_draw(struct task *task)
{
    struct task_work *work = task->work;
    Angle ang;

    njSetTexture(&mrobj_texlist);
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    TURN_VIA(njRotateY, work->ang.y, ang);
    TURN_VIA(njRotateZ, work->ang.z, ang);
    TURN_VIA(njRotateX, work->ang.x, ang);
    draw_action(raft_action, VARS(work)->frame);
    njPopMatrix(1);
}

static void raft_destroy(struct task *task)
{
    /* @hack type: work volatile, which SHC keeps in the frame word at
     * @R15, MOV.L R3,@R15 at 0x0c90ef2e and MOV.L @R15,R4 at 0x0c90ef38;
     * without it, 17 of raft_destroy's 18 words differ */
    struct task_work *volatile work = task->work;

    raft_count--;
    /* @hack redundant: a test of raft_count with an empty body, which SHC keeps
     * as TST R2,R2 at 0x0c90ef3a; without it, 10 of raft_destroy's 18 words
     * differ */
    if (raft_count == 0) {
    }
    heap_free(VARS(work)->afloat);
}

/* TRUE once the flag for player 0's character is set. FALSE for a character
 * with none */
static Bool raft_open(void)
{
    int flag = raft_flags[player_get_character(0)];

    if (flag >= 0) {
        return event_flags[flag];
    }
    return FALSE;
}
