/* Tikal as a player character: her step and modes, the tasks that turn her
 * head and move her mouth, and her edit mode */
#include <mathf.h>
#include <ninja.h>
#include <sg_xpt.h>
#include <shinobi.h>

#include "camera/camera.h"
#include "collision/collision.h"
#include "collision/ground.h"
#include "draw/draw.h"
#include "draw/model.h"
#include "event/event.h"
#include "player/player.h"
#include "player/target.h"
#include "sound/sound.h"
#include "stage/stage.h"
#include "system/pad.h"
#include "system/task.h"
#include "system/texture.h"
#include "system/util.h"
#include "tikal_prog/tikal.h"

/* her modes, in work->mode */
enum tikal_mode {
    TIKAL_INIT,
    TIKAL_STAND,
    TIKAL_WALK,
    TIKAL_JUMP,
    TIKAL_SPRING,
    TIKAL_LAUNCH,
    TIKAL_FALL,
    TIKAL_STOP, /* skidding to a stop, with the stick pulled back */
    TIKAL_POSE, /* held still by a request, in the action it gives her */
    TIKAL_EVENT, /* moved along an event's paths */
    /* down, as PLAYER_MODE_DOWN asks. tikal_choose_mode doesn't run in it,
     * so her step leaves it only for TIKAL_EDIT, on X */
    TIKAL_DOWN,
    TIKAL_EDIT /* flown with the pad: X enters it, and A leaves */
};

/* her actions, as indexes into tikal_actions */
enum tikal_action {
    ACTION_STAND,
    ACTION_WALK,
    ACTION_RUN,
    ACTION_JUMP,
    ACTION_FALL_START, /* as a rise turns to a fall */
    ACTION_FALL,
    ACTION_LAND, /* a landing at a standstill */
    ACTION_SKID,
    ACTION_TURN,
    ACTION_COUNT
};

/* her model */
extern NJS_OBJECT tikal_object;
/* the nodes of her model that tikal_welds joins */
extern NJS_OBJECT dat_0cb02028;
extern NJS_OBJECT dat_0cb02210;
extern NJS_OBJECT dat_0cb0264c;
extern NJS_OBJECT dat_0cb02a14;
extern NJS_OBJECT dat_0cb02cfc;
extern NJS_OBJECT dat_0cb03098;
extern NJS_OBJECT dat_0cb03280;
extern NJS_OBJECT dat_0cb036c8;
extern NJS_OBJECT dat_0cb03a90;
extern NJS_OBJECT dat_0cb03d9c;
extern NJS_OBJECT dat_0cb076e0;
extern NJS_OBJECT dat_0cb07880;
extern NJS_OBJECT dat_0cb07aac;
extern NJS_OBJECT dat_0cb07c4c;
extern NJS_OBJECT dat_0cb07eec;
extern NJS_OBJECT dat_0cb080cc;
extern NJS_OBJECT dat_0cb084a8;
extern NJS_OBJECT dat_0cb086c8;
extern NJS_OBJECT dat_0cb08878;
extern NJS_OBJECT dat_0cb08b18;
extern NJS_OBJECT dat_0cb08cf8;
extern NJS_OBJECT dat_0cb0990c;
/* the nodes her look turns, in tikal_look_nodes' order: her head, then her
 * two eyes */
extern NJS_OBJECT tikal_head_joint;
extern NJS_OBJECT tikal_eye0;
extern NJS_OBJECT tikal_eye1;
/* the face of her mouth's task, and the mouth's own shape, which hangs from
 * the face as its sibling */
extern NJS_OBJECT tikal_mouth;
extern NJS_OBJECT tikal_head;
extern NJS_OBJECT tikal_head_blend; /* the shape the mouth blends to */
extern NJS_MOTION tikal_mouth_shapes; /* the shapes the mouth moves through */
/* the actions tikal_actions plays. ACTION_STAND plays dat_0cb2d478,
 * ACTION_LAND and ACTION_SKID play dat_0cb29744, and ACTION_TURN plays
 * dat_0cb35890. tikal_walk_action serves the rest */
extern NJS_ACTION dat_0cb29744;
extern NJS_ACTION dat_0cb2d478;
extern NJS_ACTION tikal_walk_action;
extern NJS_ACTION dat_0cb35890;

static struct player_action tikal_actions[ACTION_COUNT] = {
#include "tikal_prog/tikal/tikal_actions.inc"
};
static NJS_OBJECT *tikal_look_nodes[3] = {&tikal_head_joint, &tikal_eye0,
                                          &tikal_eye1};
/* her one shape, a sphere. tikal_player_step gives it her radius, and
 * tikal_move_shape centers it on her middle */
static struct collision_shape tikal_shapes[1] = {
#include "tikal_prog/tikal/tikal_shapes.inc"
};
/* the vertex pairs each weld joins: its vertex in object_a, then in
 * object_b */
static Uint16 dat_0cb01690[8] = {
#include "tikal_prog/tikal/dat_0cb01690.inc"
};
static Uint16 dat_0cb016a0[8] = {
#include "tikal_prog/tikal/dat_0cb016a0.inc"
};
static Uint16 dat_0cb016b0[8] = {
#include "tikal_prog/tikal/dat_0cb016b0.inc"
};
static Uint16 dat_0cb016c0[8] = {
#include "tikal_prog/tikal/dat_0cb016c0.inc"
};
static Uint16 dat_0cb016d0[8] = {
#include "tikal_prog/tikal/dat_0cb016d0.inc"
};
static Uint16 dat_0cb016e0[8] = {
#include "tikal_prog/tikal/dat_0cb016e0.inc"
};
static Uint16 dat_0cb016f0[8] = {
#include "tikal_prog/tikal/dat_0cb016f0.inc"
};
static Uint16 dat_0cb01700[8] = {
#include "tikal_prog/tikal/dat_0cb01700.inc"
};
static Uint16 dat_0cb01710[8] = {
#include "tikal_prog/tikal/dat_0cb01710.inc"
};
static Uint16 dat_0cb01720[8] = {
#include "tikal_prog/tikal/dat_0cb01720.inc"
};
static Uint16 dat_0cb01730[8] = {
#include "tikal_prog/tikal/dat_0cb01730.inc"
};
static Uint16 dat_0cb01740[8] = {
#include "tikal_prog/tikal/dat_0cb01740.inc"
};
static Uint16 dat_0cb01750[6] = {
#include "tikal_prog/tikal/dat_0cb01750.inc"
};
static Uint16 dat_0cb0175c[6] = {
#include "tikal_prog/tikal/dat_0cb0175c.inc"
};
/* the seams of her model, then a weld with a null base, which ends the
 * table */
static struct weld tikal_welds[15] = {
#include "tikal_prog/tikal/tikal_welds.inc"
};
/* a table that nothing reads. it holds its place in this object's data,
 * after tikal_welds */
static Uint16 dat_0cb018d0[22] = {
#include "tikal_prog/tikal/dat_0cb018d0.inc"
};

static void tikal_choose_mode(struct task_work *work,
                              struct task_motion *motion,
                              struct player *record);
static void tikal_destroy(struct task *task);
static void tikal_move_shape(struct task_work *work, struct task_motion *motion,
                             struct player *record);
static void tikal_draw(struct task *task);
static void tikal_set_walk_action(struct task_work *work,
                                  struct task_motion *motion,
                                  struct player *record);
static void tikal_edit(struct task_work *work, struct task_motion *motion,
                       struct player *record);
static Bool tikal_try_jump(struct task_work *work, struct task_motion *motion,
                           struct player *record);
static Bool tikal_try_drop(struct task_work *work, struct task_motion *motion,
                           struct player *record);
static Bool tikal_follow_event(struct task_work *work,
                               struct task_motion *motion,
                               struct player *record);
static Bool tikal_try_request(struct task_work *work,
                              struct task_motion *motion,
                              struct player *record);
static Bool tikal_try_fall(struct task_work *work, struct task_motion *motion,
                           struct player *record);
static Bool tikal_try_stop(struct task_work *work, struct task_motion *motion,
                           struct player *record);
static void tikal_mouth_start(struct task *task);
static Bool tikal_look_create(Sint8 player, NJS_OBJECT **nodes);
static void tikal_look_destroy(struct task *task);
static void tikal_look_step(struct task *task);

/* @hack spelling: the stand action set through a helper SHC expands, which
 * copies record into R4, the parameter's register, and stores through it
 * (MOV R13,R4 at 0x0cb00138); stored in place, 3 of tikal_player_step's 345
 * words differ */
static void tikal_set_stand_action(struct task_work *work,
                                   struct task_motion *motion,
                                   struct player *record)
{
    record->anim.action = ACTION_STAND;
}

/* her step: the mode she's in moves her, then she's posed and drawn. outside
 * TIKAL_DOWN, tikal_choose_mode first picks the mode. X pressed on her
 * player's pad puts her in the edit mode */
void tikal_player_step(struct task *task)
{
    struct task_work *work = task->work;
    struct task_motion *motion = task->motion;
    struct player *record = motion->record;
    struct task *mouth;

    /* coming down onto a task, she stands on it. the tilt she copies is her
     * own: her collision record's owner is her task */
    if ((work->flags & COLLISION_ON_TOP) && motion->speed.y <= 0.0F) {
        work->flags |= PLAYER_ON_TASK;
        work->ang.x = work->collision->owner->work->ang.x;
        work->ang.z = work->collision->owner->work->ang.z;
    }
    if (work->mode != TIKAL_DOWN) {
        tikal_choose_mode(work, motion, record);
    }
    switch (work->mode) {
    case TIKAL_INIT:
        PLAYER_VARS(work)->character = CHARACTER_TIKAL;
        /* player_init makes her record, so the line below reads it again */
        player_init(PLAYER_VARS(work)->player, task);
        record = motion->record;
        record->anim.actions = tikal_actions;
        record->anim.action = ACTION_STAND;
        collision_init(task, tikal_shapes, COLLISION_PLAYER, 1, 1);
        work->collision->shapes->radius = record->consts.radius;
        work->mode = TIKAL_STAND;
        texture_load_character(CHARACTER_TIKAL);
        tikal_look_create(PLAYER_VARS(work)->player, tikal_look_nodes);
        record->anim.welds = tikal_welds;
        mouth = task_create(TASK_WORK, 3, tikal_mouth_start);
        if (mouth != NULL) {
            PLAYER_VARS(mouth->work)->player = PLAYER_VARS(work)->player;
        }
        task->destructor = tikal_destroy;
        task->draw = tikal_draw;
        break;
    case TIKAL_STAND:
        player_turn_to_stick(work, motion, record);
        player_accel_ground(work, motion, record);
        player_apply_accel(work, motion, record);
        player_collide(work, motion, record);
        player_take_speed(work, motion, record);
        tikal_set_stand_action(work, motion, record);
        break;
    case TIKAL_WALK:
        if (record->anim.previous == ACTION_TURN &&
            record->anim.current != ACTION_TURN) {
            record->speed.x += record->consts.trot_speed;
        }
        player_accel_ground(work, motion, record);
        player_apply_accel(work, motion, record);
        player_collide(work, motion, record);
        player_take_speed(work, motion, record);
        tikal_set_walk_action(work, motion, record);
        break;
    case TIKAL_JUMP:
        player_turn_upright(work, motion, record);
        if (!player_steer_to_target(work, motion, record)) {
            player_accel_air(work, motion, record);
        }
        player_apply_accel(work, motion, record);
        player_collide(work, motion, record);
        player_take_speed(work, motion, record);
        break;
    case TIKAL_SPRING:
        player_accel_air(work, motion, record);
        player_apply_accel(work, motion, record);
        player_collide(work, motion, record);
        player_take_speed(work, motion, record);
        break;
    case TIKAL_LAUNCH:
        if (record->speed.y >= record->consts.launch_speed) {
            player_accel_launched(work, motion, record);
        } else {
            player_accel_air(work, motion, record);
        }
        player_apply_accel(work, motion, record);
        player_collide(work, motion, record);
        player_take_speed(work, motion, record);
        break;
    case TIKAL_FALL:
        player_turn_upright(work, motion, record);
        if (!player_steer_to_target(work, motion, record)) {
            player_accel_air(work, motion, record);
        }
        player_apply_accel(work, motion, record);
        player_collide(work, motion, record);
        player_take_speed(work, motion, record);
        break;
    case TIKAL_STOP:
        player_accel_brake(work, motion, record);
        player_apply_accel(work, motion, record);
        player_collide(work, motion, record);
        player_take_speed(work, motion, record);
        break;
    case TIKAL_POSE:
        break;
    case TIKAL_EVENT:
        event_paths_step(work);
        break;
    case TIKAL_DOWN:
        player_turn_upright(work, motion, record);
        player_accel_roll(work, motion, record);
        player_apply_accel(work, motion, record);
        player_collide(work, motion, record);
        player_take_speed(work, motion, record);
        break;
    case TIKAL_EDIT:
        /* no break ends the last case. with one, player_anim_step's and
         * player_update_welds' addresses trade R2 and R3 (0x0cb00328,
         * 0x0cb00330), and 4 of 345 words differ */
        tikal_edit(work, motion, record);
    }
    player_anim_step(&record->anim);
    player_update_welds(work, motion, record);
    tikal_move_shape(work, motion, record);
    /* she draws in her step. task->draw, the same function, draws her while
     * the tasks draw without stepping, as under the pause menu */
    tikal_draw(task);
    if ((pad_records[PLAYER_VARS(work)->player].press & PDD_DGT_TX) &&
        work->mode != TIKAL_EDIT) {
        work->mode = TIKAL_EDIT;
        work->flags &= ~(PLAYER_ON_PATH | PLAYER_REQUEST);
    }
    shadow_update(work, &record->shadow);
    collision_register(work);
    player_finish_step(work, motion, record);
}

/* pick her next mode, and its action, from the mode she's in. a tikal_try_
 * function that gives TRUE has set her mode itself */
static void tikal_choose_mode(struct task_work *work,
                              struct task_motion *motion, struct player *record)
{
    switch (work->mode) {
    case TIKAL_STAND:
        if (tikal_try_request(work, motion, record)) {
            break;
        }
        if (tikal_try_fall(work, motion, record)) {
            break;
        }
        if (tikal_try_jump(work, motion, record)) {
            break;
        }
        if (player_read_stick(work, NULL, NULL)) {
            work->mode = TIKAL_WALK;
            record->anim.action = ACTION_WALK;
            /* @hack redundant: a break that ends the arm, though the case's own
             * break comes next. it has SHC join the action's store to the one
             * in TIKAL_FALL's landing (BRA 0x0cb0069c at 0x0cb0045a); without
             * it, 63 of 414 words differ */
            break;
        }
        break;
    case TIKAL_WALK:
        if (tikal_try_request(work, motion, record)) {
            break;
        }
        /* @hack spelling: the fall, the jump and the stop each tested with a
         * clear and a break of its own, which has SHC load ~PLAYER_ON_PATH
         * into R10 ahead of the tests for the clear they share (MOV.W at
         * 0x0cb0046e); tested with || in one arm, the clear loads it into R3
         * and 44 of 414 words differ */
        if (tikal_try_fall(work, motion, record)) {
            work->flags &= ~PLAYER_ON_PATH;
            break;
        }
        if (tikal_try_jump(work, motion, record)) {
            work->flags &= ~PLAYER_ON_PATH;
            break;
        }
        if (tikal_try_stop(work, motion, record)) {
            work->flags &= ~PLAYER_ON_PATH;
            break;
        }
        if (player_stick_back_or_end_event(work, motion) &&
            record->speed.x >= record->consts.trot_speed) {
            work->mode = TIKAL_STOP;
            record->anim.action = ACTION_SKID;
            sound_play(SE_SKID, 0, 0, 0);
            work->flags &= ~PLAYER_ON_PATH;
            /* @hack redundant: a break that ends the arm, though the case's own
             * break comes next. it has the clear join the three above at
             * 0x0cb004c2; without it, the function runs 32 bytes long and 342
             * of 414 words differ */
            break;
        }
        break;
    case TIKAL_JUMP:
        if (tikal_try_request(work, motion, record)) {
            break;
        }
        if (work->flags & PLAYER_ON_GROUND) {
            if (PLAYER_VARS(work)->player == 0) {
                sound_play(SE_LAND, 0, 0, 0);
            }
            if (player_stick_back_or_end_event(work, motion) &&
                record->speed.x > 0.0F) {
                work->mode = TIKAL_STOP;
                record->anim.action = ACTION_SKID;
            } else if (tikal_try_stop(work, motion, record)) {
                work->ang.x = motion->aim.x;
                work->ang.z = motion->aim.z;
                record->anim.action = ACTION_LAND;
            } else {
                work->ang.x = motion->aim.x;
                work->ang.z = motion->aim.z;
                work->mode = TIKAL_WALK;
                tikal_set_walk_action(work, motion, record);
            }
            break;
        }
        if (tikal_try_drop(work, motion, record)) {
            break;
        }
        if (player_count_jump_hold(work, record)) {
            record->speed.y += record->consts.jump_hold_speed * 0.8F;
        }
        break;
    /* @hack spelling: TIKAL_SPRING's and TIKAL_LAUNCH's tests kept in cases
     * of their own, as the image holds the test twice (0x0cb005b6,
     * 0x0cb005e0). as one case, the object's code runs 32 bytes short and
     * 355 of tikal_choose_mode's 414 words differ */
    case TIKAL_SPRING:
        if (tikal_try_request(work, motion, record)) {
            break;
        }
        if (motion->speed.y <= 0.0F) {
            work->mode = TIKAL_FALL;
            record->anim.action = ACTION_FALL_START;
        }
        break;
    case TIKAL_LAUNCH:
        if (tikal_try_request(work, motion, record)) {
            break;
        }
        if (motion->speed.y <= 0.0F) {
            work->mode = TIKAL_FALL;
            record->anim.action = ACTION_FALL_START;
        }
        break;
    case TIKAL_FALL:
        if (tikal_try_request(work, motion, record)) {
            break;
        }
        if (work->flags & PLAYER_ON_GROUND) {
            if (PLAYER_VARS(work)->player == 0) {
                sound_play(SE_LAND, 0, 0, 0);
            }
            if (player_stick_back_or_end_event(work, motion) &&
                record->speed.x > 0.0F) {
                work->mode = TIKAL_STOP;
                record->anim.action = ACTION_SKID;
            } else if (tikal_try_stop(work, motion, record)) {
                work->ang.x = motion->aim.x;
                work->ang.z = motion->aim.z;
                record->anim.action = ACTION_LAND;
            } else {
                work->ang.x = motion->aim.x;
                work->ang.z = motion->aim.z;
                work->mode = TIKAL_WALK;
                tikal_set_walk_action(work, motion, record);
            }
        }
        break;
    case TIKAL_STOP:
        if (tikal_try_request(work, motion, record)) {
            break;
        }
        if (tikal_try_fall(work, motion, record)) {
            break;
        }
        if (tikal_try_jump(work, motion, record)) {
            break;
        }
        /* stopped with the stick still pulled back, she turns round */
        if (record->speed.x <= 0.0F &&
            player_stick_back_or_end_event(work, motion)) {
            work->mode = TIKAL_WALK;
            record->anim.action = ACTION_TURN;
            player_turn_to_stick(work, motion, record);
            work->ang.y = motion->aim.y;
        } else if (record->anim.action != ACTION_SKID) {
            work->mode = TIKAL_STAND;
            record->anim.action = ACTION_STAND;
        }
        break;
    case TIKAL_POSE:
        /* @hack redundant: the result tested only to break, as the next line
         * does anyway (TST R0,R0 and BF at 0x0cb00774 and 0x0cb00776); called
         * as a statement, the function runs 32 bytes long and 130 of 414 words
         * differ */
        if (tikal_try_request(work, motion, record)) {
            break;
        }
        break;
    case TIKAL_EVENT:
        if (work->event->paths == NULL) {
            work->ang.x = motion->aim.x;
            work->ang.z = motion->aim.z;
            work->mode = TIKAL_STAND;
            record->anim.action = ACTION_STAND;
        }
        break;
    case TIKAL_EDIT:
        if (pad_records[PLAYER_VARS(work)->player].press & PDD_DGT_TA) {
            work->mode = TIKAL_WALK;
            record->anim.action = ACTION_WALK;
        }
        break;
    }
}

static void tikal_destroy(struct task *task)
{
    struct task_work *work = task->work;
    /* @hack redundant: record, set through motion and never read, which
     * keeps the load of task->motion (MOV.L @(36,R4),R4 at 0x0cb007f8);
     * without the two, 11 of 16 words differ */
    struct task_motion *motion = task->motion;
    struct player *record = motion->record;

    player_free(PLAYER_VARS(work)->player);
    sound_stop_id((int)work);
    texture_release_character(CHARACTER_TIKAL);
}

/* center her shape on her middle: record->consts.center_y up from her
 * position, along her own y axis as her angles turn it */
static void tikal_move_shape(struct task_work *work, struct task_motion *motion,
                             struct player *record)
{
    struct collision_shape *shape = work->collision->shapes;
    NJS_VECTOR offset;

    njPushMatrix(&_unit_matrix_);
    offset.x = offset.z = 0.0F;
    offset.y = record->consts.center_y;
    TURN(njRotateZ, work->ang.z);
    TURN(njRotateX, work->ang.x);
    TURN(njRotateY, -work->ang.y);
    njCalcVector(NULL, &offset, &shape->center);
    njPopMatrix(1);
    shape->center.x += work->pos.x;
    shape->center.y += work->pos.y;
    shape->center.z += work->pos.z;
}

/* draw her at her shape's center, turned with her. she doesn't draw while
 * her timer is odd, so she blinks as it runs */
static void tikal_draw(struct task *task)
{
    struct task_work *work = task->work;
    struct task_motion *motion = task->motion;
    struct player *record = motion->record;
    int action;
    /* @hack redundant: an array never used, whose 16 bytes give the image's
     * 20-byte frame (ADD #-20,R15 at 0x0cb00928); without it, the frame is 4
     * bytes and 2 of 117 words differ */
    Float unused[4];

    draw_attr_save();
    draw_attr_set(0, NJD_FLAG_IGNORE_SPECULAR);
    draw_set_palette(DRAW_PLAYER_PALETTE);
    action = record->anim.action;
    if (record->anim.state == PLAYER_ANIM_BLENDING) {
        action = record->anim.current;
    }
    if (!(TASK_TIMER(work) & 1)) {
        njSetTexture(&tikal_texlist);
        njPushMatrix(NULL);
        njTranslateV(NULL, &work->collision->shapes->center);
        TURN(njRotateZ, work->ang.z);
        TURN(njRotateX, work->ang.x);
        TURN(njRotateY, NJM_DEG_ANG(180) - work->ang.y);
        if (work->event->motions != NULL) {
            event_motions_draw(work);
        } else if (record->anim.state != PLAYER_ANIM_BLENDING) {
            draw_action(record->anim.actions[action].action,
                        record->anim.frame);
        } else {
            draw_action(record->anim.blend, record->anim.frame);
        }
        njPopMatrix(1);
    }
    draw_set_palette(0);
    draw_attr_restore();
}

/* set the walk or the run her speed calls for, and how fast it plays. in
 * ACTION_TURN she keeps the turn and its speed */
static void tikal_set_walk_action(struct task_work *work,
                                  struct task_motion *motion,
                                  struct player *record)
{
    Float speed = record->speed.x + (motion->slip * 5.0F);

    if (record->anim.action != ACTION_TURN) {
        if (speed > record->consts.trot_speed) {
            record->anim.action = ACTION_RUN;
        } else {
            record->anim.action = ACTION_WALK;
        }
        record->action_speed = record->speed.x + (motion->slip * 20.0F);
    }
}

/* fly her PLAYER_EDIT_SPEED a step the way her player's stick points,
 * turned by the camera's heading. while X is held, the stick moves her up
 * and down instead. B puts her back where the stage places her */
static void tikal_edit(struct task_work *work, struct task_motion *motion,
                       struct player *record)
{
    Float x;
    Float y;
    Float sign;
    Angle ang;

    if (pad_records[PLAYER_VARS(work)->player].press & PDD_DGT_TB) {
        stage_place_player(work);
    }
    /* @hack spelling: x and y read from the stick in each arm, after the test
     * of X (MOV.W @(28,R5),R0 at 0x0cb00b32 and 0x0cb00bac); read once above
     * it, the function runs 32 bytes short and 132 of 151 words differ */
    if (pad_records[PLAYER_VARS(work)->player].on & PDD_DGT_TX) {
        x = (Float)(pad_records[PLAYER_VARS(work)->player].x1 << 8);
        y = (Float)(pad_records[PLAYER_VARS(work)->player].y1 << 8);
        if (x > PAD_DEAD_ZONE || x < -PAD_DEAD_ZONE || y > PAD_DEAD_ZONE ||
            y < -PAD_DEAD_ZONE) {
            sign = y != 0.0F ? y / njSqrt(y * y) : 0.0F;
            work->pos.y -= sign * PLAYER_EDIT_SPEED;
        }
    } else {
        x = (Float)(pad_records[PLAYER_VARS(work)->player].x1 << 8);
        y = (Float)(pad_records[PLAYER_VARS(work)->player].y1 << 8);
        /* @hack spelling: an empty arm for the stick in its dead zone, which
         * loads atan2f's address into R2 (MOV.L at 0x0cb00bd4); with the test
         * inverted and no else, R3 takes it and 2 of 151 words differ */
        if (x <= PAD_DEAD_ZONE && x >= -PAD_DEAD_ZONE && y <= PAD_DEAD_ZONE &&
            y >= -PAD_DEAD_ZONE) {
        } else {
            ang = (Angle)(atan2f(y, x) * ANGLE_PER_RADIAN) - camera_work->ang.y;
            work->pos.x += njCos(ang) * PLAYER_EDIT_SPEED;
            work->pos.z += njSin(ang) * PLAYER_EDIT_SPEED;
        }
    }
}

/* jump as player_jump_pressed says to: up at her jump speed, plus the speed
 * of the task she stands on. TRUE when she jumps */
static Bool tikal_try_jump(struct task_work *work, struct task_motion *motion,
                           struct player *record)
{
    if (player_jump_pressed(work, record)) {
        work->mode = TIKAL_JUMP;
        record->speed.y = record->consts.jump_speed;
        if (record->ground_task != NULL) {
            struct task_force *force = record->ground_task->force;

            if (force != NULL) {
                player_set_accel(PLAYER_VARS(work)->player, force->speed.x,
                                 force->speed.y, force->speed.z);
            }
        }
        record->anim.action = ACTION_JUMP;
        work->flags &= ~PLAYER_ON_TASK;
        record->jump_steps = 0;
        sound_play(SE_JUMP, 0, 0, 0);
        return TRUE;
    }
    return FALSE;
}

/* an action button pressed in a jump drops her into TIKAL_FALL, with her
 * record's speed and her action's speed at 0. TRUE when she drops */
static Bool tikal_try_drop(struct task_work *work, struct task_motion *motion,
                           struct player *record)
{
    if (pad_records[PLAYER_VARS(work)->player].press & pad_action_buttons) {
        work->mode = TIKAL_FALL;
        player_clear_speed(NULL, record);
        record->anim.action = ACTION_FALL;
        record->action_speed = 0.0F;
        return TRUE;
    }
    return FALSE;
}

/* follow the event's action: let it move her, or turn as it says. TRUE for
 * an action she takes. for any other, PLAYER_EVENT_DRIVEN clears, and the
 * result is FALSE */
static Bool tikal_follow_event(struct task_work *work,
                               struct task_motion *motion,
                               struct player *record)
{
    struct event_work *event = work->event;

    switch (event->action) {
    case EVENT_ACTION_DONE:
    case EVENT_ACTION_RUN:
    case EVENT_ACTION_MOVE:
    case EVENT_ACTION_PUSH:
        work->flags |= PLAYER_EVENT_DRIVEN;
        break;
    case EVENT_ACTION_TURN_TO:
        player_set_heading(PLAYER_VARS(work)->player, event->move.ang.y);
        break;
    case EVENT_ACTION_TURN_BY:
        player_set_heading(PLAYER_VARS(work)->player,
                           event->move.ang.y - work->ang.y + NJM_DEG_ANG(90));
        break;
    default:
        work->flags &= ~PLAYER_EVENT_DRIVEN;
        return FALSE;
    }
    return TRUE;
}

/* TRUE when something outside decides her mode, which she then takes. an
 * event's paths put her in TIKAL_EVENT, and a request in work->submode in
 * the mode it asks for. while the event's record holds an action, she
 * follows it, and the result is FALSE. it is FALSE too for a request she
 * doesn't know, and for one to join a path, which can only mark her on it */
static Bool tikal_try_request(struct task_work *work,
                              struct task_motion *motion, struct player *record)
{
    if (work->event->action) {
        tikal_follow_event(work, motion, record);
        return FALSE;
    }
    if (work->event->paths != NULL) {
        work->mode = TIKAL_EVENT;
        work->flags &= ~PLAYER_ON_PATH;
        return TRUE;
    }
    if (!(work->flags & PLAYER_REQUEST)) {
        return FALSE;
    }
    switch (work->submode) {
    case PLAYER_MODE_SPRING:
    case PLAYER_MODE_BOUNCE_1:
    case PLAYER_MODE_BOUNCE_2:
    case PLAYER_MODE_BOUNCE_3:
        work->mode = TIKAL_SPRING;
        record->anim.action = ACTION_JUMP;
        work->flags &= ~PLAYER_ON_PATH;
        break;
    case PLAYER_MODE_LAUNCH:
    case PLAYER_MODE_FLING:
    case PLAYER_MODE_UPDRAFT:
        work->mode = TIKAL_LAUNCH;
        record->anim.action = ACTION_JUMP;
        work->flags &= ~PLAYER_ON_PATH;
        break;
    case PLAYER_MODE_STILL:
        work->mode = TIKAL_POSE;
        record->anim.action = ACTION_STAND;
        work->ang.z = 0;
        work->ang.x = 0;
        player_clear_speed(motion, record);
        work->flags &= ~PLAYER_ON_PATH;
        break;
    case PLAYER_MODE_GET_ON:
        work->mode = TIKAL_POSE;
        record->anim.action = ACTION_FALL;
        work->ang.z = 0;
        work->ang.x = 0;
        player_clear_speed(motion, record);
        work->flags &= ~PLAYER_ON_PATH;
        break;
    case PLAYER_MODE_SQUASH_2:
    case PLAYER_MODE_SQUASH_3:
        work->mode = TIKAL_POSE;
        record->anim.action = ACTION_STAND;
        work->ang.z = 0;
        work->ang.x = 0;
        player_clear_speed(motion, record);
        work->flags &= ~PLAYER_ON_PATH;
        break;
    case PLAYER_MODE_RESET:
        work->mode = TIKAL_STAND;
        record->anim.action = ACTION_STAND;
        work->flags &= ~PLAYER_ON_PATH;
        break;
    case PLAYER_MODE_JOIN_PATH:
        if (work->mode == TIKAL_WALK ||
            record->speed.x > record->consts.running_speed) {
            work->flags |= PLAYER_ON_PATH;
        }
        return FALSE;
    case PLAYER_MODE_DOWN:
        sound_play(SE_DOWN, 0, 0, 0);
        work->mode = TIKAL_DOWN;
        record->anim.action = ACTION_FALL;
        work->flags &= ~PLAYER_ON_PATH;
        break;
    default:
        return FALSE;
    }
    return TRUE;
}

/* TRUE once she is off the ground, with her mode set to TIKAL_FALL */
static Bool tikal_try_fall(struct task_work *work, struct task_motion *motion,
                           struct player *record)
{
    if (!(work->flags & PLAYER_ON_GROUND)) {
        work->mode = TIKAL_FALL;
        record->anim.action = ACTION_FALL;
        return TRUE;
    }
    return FALSE;
}

/* TRUE once she is still with the stick let go, with her mode set to
 * TIKAL_STAND */
static Bool tikal_try_stop(struct task_work *work, struct task_motion *motion,
                           struct player *record)
{
    if (!player_read_stick(work, NULL, NULL) && record->speed.x == 0.0F) {
        work->mode = TIKAL_STAND;
        record->anim.action = ACTION_STAND;
        return TRUE;
    }
    return FALSE;
}

/* the first step of her mouth's task: give the task its player_mouth, then
 * hand it to player_mouth_step. while the heap has no room, it tries again
 * on the next step */
static void tikal_mouth_start(struct task *task)
{
    struct player_mouth *mouth;

    if ((mouth = heap_calloc(1, sizeof(*mouth))) != NULL) {
        task->motion = (struct task_motion *)mouth;
        mouth->face = &tikal_mouth;
        mouth->mouth = &tikal_head;
        mouth->blend = &tikal_head_blend;
        mouth->shapes = &tikal_mouth_shapes;
        task->destructor = player_mouth_destroy;
        task->step = player_mouth_step;
    }
}

/* make the look task for player, which turns nodes: the head, then the two
 * eyes. FALSE when the task or its record finds no room */
/* NOLINTBEGIN(clang-diagnostic-return-type) */
static Bool tikal_look_create(Sint8 player, NJS_OBJECT **nodes)
{
    struct task *task;
    struct player_look_motion *look;

    if ((task = task_create(TASK_WORK, 2, tikal_look_step)) != NULL) {
        if ((look = heap_alloc(sizeof(*look))) == NULL) {
            task_kill(task);
            return FALSE;
        }
        task->motion = (struct task_motion *)look;
        PLAYER_VARS(task->work)->player = player;
        look->look.head = nodes[0];
        look->look.eye0 = nodes[1];
        look->look.eye1 = nodes[2];
    } else {
        return FALSE;
    }
    /* @hack spelling: a made task returns the 96 left in R0, the offset
     * of look.eye1, which MOV #96,R0 at 0x0cb010dc loads for the store above.
     * the one caller discards the result, so return TRUE does the same. with
     * return TRUE, 7 of 46 words differ */
}
/* NOLINTEND(clang-diagnostic-return-type) */

/* give the head and eye nodes back the angles the look's first step found
 * them at */
static void tikal_look_destroy(struct task *task)
{
    struct player_look_motion *look = PLAYER_LOOK(task);

    look->look.head->ang[0] = look->head_ang[0];
    look->look.head->ang[1] = look->head_ang[1];
    look->look.head->ang[2] = look->head_ang[2];
    look->look.eye1->ang[0] = look->eye1.ang[0];
    look->look.eye1->ang[1] = look->eye1.ang[1];
    look->look.eye1->ang[2] = look->eye1.ang[2];
    look->look.eye0->ang[0] = look->eye0.ang[0];
    look->look.eye0->ang[1] = look->eye0.ang[1];
    look->look.eye0->ang[2] = look->eye0.ang[2];
}

/* turn her head and eyes toward the heading she aims for, at the camera
 * while Z is held, or as her event says. the head turns as far as its
 * limits allow, and the eyes take the rest. once her player's task is gone,
 * the look's task ends */
static void tikal_look_step(struct task *task)
{
    struct task_work *work = task->work;
    struct player_look_motion *look = PLAYER_LOOK(task);
    struct task_work *owner = player_work[PLAYER_VARS(work)->player];
    struct task_motion *owner_motion = player_motion[PLAYER_VARS(work)->player];
    struct event_work *event;
    struct task_work *target;
    struct collision_shape *shape;
    struct task_work *camera;
    Angle angles[3];
    NJS_POINT3 direction;
    NJS_VECTOR facing;

    /* @hack spelling: an empty arm for LOOK_INIT ahead of the three tests,
     * whose chain loads owner->event into R3 (MOV.L @(60,R12),R3 at
     * 0x0cb011e2); with the three nested under a test of work->mode, R2
     * takes it and 2 of 395 words differ */
    if (owner == NULL) {
        work->mode = LOOK_END;
    } else if (work->mode == LOOK_INIT) {
    } else if (owner->event->look != 0) {
        work->mode = LOOK_EVENT;
    } else if (pad_records[PLAYER_VARS(work)->player].on & PDD_DGT_TZ) {
        work->mode = LOOK_CAMERA;
    } else {
        work->mode = LOOK_AHEAD;
    }
    switch (work->mode) {
    case LOOK_INIT:
        work->mode = LOOK_AHEAD;
        look->head_ang[0] = look->look.head->ang[0];
        look->head_ang[1] = look->look.head->ang[1];
        look->head_ang[2] = look->look.head->ang[2];
        look->eye0.ang[0] = look->look.eye0->ang[0];
        look->eye0.ang[1] = look->look.eye0->ang[1];
        look->eye0.ang[2] = look->look.eye0->ang[2];
        look->eye1.ang[0] = look->look.eye1->ang[0];
        look->eye1.ang[1] = look->look.eye1->ang[1];
        look->eye1.ang[2] = look->look.eye1->ang[2];
        look->look.head_max_y = 0x2000;
        look->look.head_min_y = -0x2000;
        look->look.head_max_z = 0x400;
        look->look.head_min_z = -0x1800;
        look->look.eye0_max_y = 0x200;
        look->look.eye0_min_y = -0x1b00;
        look->look.eye1_max_y = 0x1b00;
        look->look.eye1_min_y = -0x200;
        look->look.eye0_max_z = 0x800;
        look->look.eye0_min_z = -0xd00;
        look->look.eyes_max_z = 0x800;
        look->look.eyes_min_z = -0xd00;
        task->destructor = tikal_look_destroy;
        break;
    case LOOK_AHEAD:
        look->look.head_aim_y =
            -angle_difference(owner->ang.y, owner_motion->aim.y);
        look->look.head_aim_z = 0;
        player_look_turn_head(&look->look);
        look->look.eyes_aim_y = 0;
        look->look.eyes_aim_z = 0;
        player_look_turn_eyes(&look->look);
        break;
    case LOOK_CAMERA:
        camera = camera_work;
        shape = owner->collision->shapes;
        /* @hack spelling: facing.y and facing.z in one chained assignment,
         * which stores z first (FMOV.S at 0x0cb01392, then 0x0cb01396); as
         * two statements, y comes first and 2 of 395 words differ */
        facing.y = facing.z = 0.0F;
        facing.x = 1.0F;
        player_turn_to_world(owner, &facing);
        direction = camera->pos;
        njSubVector(&direction, &shape->center);
        vector_get_angles(&direction, &facing, angles);
        look->look.head_aim_y = -angles[1];
        look->look.head_aim_z = angles[2];
        player_look_turn_head(&look->look);
        look->look.eyes_aim_y = angles[1] + look->look.head_y;
        look->look.eyes_aim_z = angles[2] - look->look.head_z;
        player_look_turn_eyes(&look->look);
        break;
    case LOOK_EVENT:
        event = owner->event;
        shape = owner->collision->shapes;
        switch (event->look) {
        case EVENT_LOOK_ANGLES:
            angles[1] = event->look_ang.y;
            angles[2] = event->look_ang.z;
            break;
        case EVENT_LOOK_POINT:
            /* @hack spelling: facing.y and facing.z in one chained assignment,
             * which stores z first (FMOV.S at 0x0cb0142c, then 0x0cb01430); as
             * two statements, y comes first and 2 of 395 words differ */
            facing.y = facing.z = 0.0F;
            facing.x = 1.0F;
            player_turn_to_world(owner, &facing);
            direction = event->look_pos;
            njSubVector(&direction, &shape->center);
            vector_get_angles(&direction, &facing, angles);
            break;
        case EVENT_LOOK_TASK:
            /* @hack spelling: two tests, each with its own store and break,
             * which have SHC load only player_look_turn_eyes' address ahead of
             * the switch, into R8 (MOV.L at 0x0cb01242); joined in one test, it
             * loads player_look_turn_head's there too, and 172 of 395 words
             * differ */
            if (event->look_task == NULL) {
                event->look = 0;
                break;
            }
            target = event->look_task->work;
            if (target == NULL) {
                event->look = 0;
                break;
            }
            /* @hack spelling: facing.y and facing.z in one chained assignment,
             * which stores z first (FMOV.S at 0x0cb01476, then 0x0cb0147a); as
             * two statements, y comes first and 2 of 395 words differ */
            facing.y = facing.z = 0.0F;
            facing.x = 1.0F;
            player_turn_to_world(owner, &facing);
            njPushMatrix(&_unit_matrix_);
            TURN(njRotateZ, target->ang.z);
            TURN(njRotateX, target->ang.x);
            TURN(njRotateY, target->ang.y);
            njCalcVector(NULL, &event->look_pos, &direction);
            njPopMatrix(1);
            njAddVector(&direction, &target->pos);
            njSubVector(&direction, &shape->center);
            vector_get_angles(&direction, &facing, angles);
            break;
        default:
            angles[1] = 0;
            angles[2] = 0;
            break;
        }
        look->look.head_aim_y = -angles[1];
        look->look.head_aim_z = angles[2];
        player_look_turn_head(&look->look);
        look->look.eyes_aim_y = angles[1] + look->look.head_y;
        look->look.eyes_aim_z = angles[2] - look->look.head_z;
        player_look_turn_eyes(&look->look);
        break;
    default:
        task_kill(task);
        break;
    }
}
