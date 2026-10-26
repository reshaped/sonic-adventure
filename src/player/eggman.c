/* Eggman: his player task, on foot, and the edit mode that flies him */
#include <mathf.h>
#include <ninja.h>
#include <sg_xpt.h>
#include <shinobi.h>

#include "camera/camera.h"
#include "collision/collision.h"
#include "collision/ground.h"
#include "draw/draw.h"
#include "draw/model.h"
#include "draw/tree.h"
#include "event/event.h"
#include "player/eggman.h"
#include "player/player.h"
#include "player/target.h"
#include "player/textures.h"
#include "sound/sound.h"
#include "stage/stage.h"
#include "system/pad.h"
#include "system/task.h"
#include "system/texture.h"
#include "system/util.h"

/* his modes, in work->mode. eggman_mode_names holds the image's name for
 * each */
enum eggman_mode {
    EGGMAN_INIT,
    EGGMAN_STAND,
    EGGMAN_WALK,
    EGGMAN_PATH,
    EGGMAN_PATH_SPIN,
    EGGMAN_JUMP,
    EGGMAN_SPRING,
    EGGMAN_LAUNCH,
    EGGMAN_FALL,
    EGGMAN_STOP, /* skidding to a stop, with the stick pulled back */
    EGGMAN_PUSH,
    EGGMAN_HURT,
    EGGMAN_BACK,
    EGGMAN_POSE,
    EGGMAN_CRASH, /* thrown back from a wall he ran into */
    EGGMAN_EVENT, /* moved along an event's paths */
    EGGMAN_DOWN,
    EGGMAN_EDIT, /* flown with the pad: X enters it, and A leaves */
    EGGMAN_MODES
};

/* his actions, as indexes into eggman_actions */
enum eggman_action {
    ACTION_STAND = 0,
    ACTION_LAND = 1, /* a landing at a standstill */
    ACTION_WALK = 4,
    ACTION_TROT = 5,
    ACTION_RUN = 6,
    ACTION_SPRINT = 7,
    ACTION_FULL_SPEED = 8,
    ACTION_JUMP = 9,
    ACTION_FALL_START = 12, /* as a rise turns to a fall */
    ACTION_FALL = 13,
    ACTION_SKID = 15,
    ACTION_TURN = 16,
    ACTION_PUSH = 17,
    ACTION_CRASH = 22,
    ACTION_CRASH_LAND = 23,
    ACTION_COUNT = 25 /* eggman_actions' entries, an empty one last */
};

/* where eggman_step's second test for a press of X would put him. the
 * first test sets EGGMAN_EDIT whenever the second's condition holds, so the
 * second never passes */
#define EDIT_X 100.0F
#define EDIT_Y 0.0F
#define EDIT_Z (-600.0F)

extern NJS_OBJECT eggman_object;
extern NJS_OBJECT eggman_upper_arm_plus_z;
extern NJS_OBJECT eggman_forearm_plus_z;
extern NJS_OBJECT eggman_hand_plus_z;
extern NJS_OBJECT eggman_fingers_plus_z;
extern NJS_OBJECT eggman_upper_arm_minus_z;
extern NJS_OBJECT eggman_forearm_minus_z;
extern NJS_OBJECT eggman_hand_minus_z;
extern NJS_OBJECT eggman_fingers_minus_z;
extern NJS_OBJECT eggman_thigh_plus_z;
extern NJS_OBJECT eggman_shin_plus_z;
extern NJS_OBJECT eggman_foot_plus_z;
extern NJS_OBJECT eggman_toes_plus_z;
extern NJS_OBJECT eggman_thigh_minus_z;
extern NJS_OBJECT eggman_shin_minus_z;
extern NJS_OBJECT eggman_foot_minus_z;
extern NJS_OBJECT eggman_toes_minus_z;
extern NJS_ACTION eggman_stand_action;
extern NJS_ACTION eggman_walk_action;
extern NJS_ACTION eggman_run_action;
extern NJS_ACTION eggman_skid_action;
extern NJS_ACTION eggman_turn_action;

static void eggman_choose_mode(struct task_work *work,
                               struct task_motion *motion,
                               struct player *record);
static void eggman_destroy(struct task *task);
static void eggman_draw(struct task_work *work, struct task_motion *motion,
                        struct player *record);
static void eggman_set_walk_action(struct task_work *work,
                                   struct task_motion *motion,
                                   struct player *record);
static void eggman_edit(struct task_work *work, struct task_motion *motion,
                        struct player *record);
static Bool eggman_try_jump(struct task_work *work, struct task_motion *motion,
                            struct player *record);
static Bool eggman_follow_event(struct task_work *work,
                                struct task_motion *motion,
                                struct player *record);
static Bool eggman_try_request(struct task_work *work,
                               struct task_motion *motion,
                               struct player *record);
static Bool eggman_try_fall(struct task_work *work, struct task_motion *motion,
                            struct player *record);
static Bool eggman_try_stop(struct task_work *work, struct task_motion *motion,
                            struct player *record);

/* his modes' names, which nothing reads. they start this object's data, at
 * 0x8c586910 */
static char eggman_mode_names[EGGMAN_MODES][8] = {
    "MD_INIT", "MD_STND", "MD_WALK", "MD_1DWK", "MD_1DSP", "MD_JUMP",
    "MD_SPRG", "MD_LNCH", "MD_FALL", "MD_STOP", "MD_PUSH", "MD_DAMG",
    "MD_BACK", "MD_PLON", "MD_CRSH", "EVENT",   "MD_KILL", "MD_EDIT",
};

static struct player_action eggman_actions[ACTION_COUNT] = {
    {&eggman_stand_action, 24, PLAYER_ANIM_LOOP, ACTION_STAND, 1.0F, 1.0F},
    {&eggman_stand_action, 24, PLAYER_ANIM_ONCE, 2, 1.0F, 1.0F},
    {&eggman_stand_action, 24, PLAYER_ANIM_LOOP, 2, 1.0F, 1.0F},
    {&eggman_stand_action, 24, PLAYER_ANIM_ONCE, ACTION_STAND, 1.0F, 1.0F},
    {&eggman_walk_action, 24, PLAYER_ANIM_LOOP, ACTION_WALK, 1.0F, 1.0F},
    {&eggman_walk_action, 24, PLAYER_ANIM_LOOP, ACTION_TROT, 1.0F, 1.0F},
    {&eggman_run_action, 24, PLAYER_ANIM_LOOP, ACTION_RUN, 1.0F, 1.0F},
    {&eggman_run_action, 24, PLAYER_ANIM_LOOP, ACTION_SPRINT, 1.0F, 1.0F},
    {&eggman_run_action, 24, PLAYER_ANIM_LOOP, ACTION_FULL_SPEED, 1.0F, 1.0F},
    {&eggman_stand_action, 24, PLAYER_ANIM_LOOP_PACE, ACTION_JUMP, 1.0F, 1.0F},
    {&eggman_stand_action, 24, PLAYER_ANIM_LOOP, 10, 1.0F, 1.0F},
    {&eggman_stand_action, 24, PLAYER_ANIM_LOOP, 11, 1.0F, 1.0F},
    {&eggman_stand_action, 24, PLAYER_ANIM_ONCE, ACTION_FALL, 1.0F, 1.0F},
    {&eggman_stand_action, 24, PLAYER_ANIM_LOOP, ACTION_FALL, 1.0F, 1.0F},
    {&eggman_stand_action, 24, PLAYER_ANIM_ONCE, ACTION_STAND, 1.0F, 1.0F},
    {&eggman_skid_action, 24, PLAYER_ANIM_ONCE, ACTION_STAND, 1.0F, 1.0F},
    {&eggman_turn_action, 24, PLAYER_ANIM_ONCE, ACTION_STAND, 1.0F, 1.0F},
    {&eggman_stand_action, 24, PLAYER_ANIM_ONCE, 18, 1.0F, 1.0F},
    {&eggman_stand_action, 24, PLAYER_ANIM_LOOP, 18, 1.0F, 1.0F},
    {&eggman_stand_action, 24, PLAYER_ANIM_LOOP, ACTION_STAND, 1.0F, 1.0F},
    {&eggman_stand_action, 24, PLAYER_ANIM_LOOP, 20, 1.0F, 1.0F},
    {&eggman_stand_action, 24, PLAYER_ANIM_LOOP, 21, 1.0F, 1.0F},
    {&eggman_stand_action, 24, PLAYER_ANIM_ONCE, ACTION_CRASH_LAND, 1.0F, 1.0F},
    {&eggman_stand_action, 24, PLAYER_ANIM_ONCE, ACTION_STAND, 1.0F, 1.0F},
    {NULL, 0, 0, 0, 0.0F, 0.0F},
};

/* his one shape, a sphere. eggman_step gives it his radius, and
 * eggman_draw centers it on his middle */
/* clang-format off */
static struct collision_shape eggman_shapes[] = {
    {SHAPE_SPHERE | SHAPE_IN_WORLD | SHAPE_HURTABLE,
     SHAPE_HURTS_PLAYERS | SHAPE_HURTS_ENEMIES | SHAPE_HURTS_OBJECTS,
     PUSH_ALL | PUSH_FIRM(PUSH_OTHERS), {0.0F, 1.0F, 0.0F}, 4.0F, 4.0F, 0.65F,
     {0}, 0, NULL},
};
/* clang-format on */

/* each joint's shared vertices, as pairs of indices: a vertex in object_a,
 * then the one in object_b it welds to. a joint's two welds, plus_z and
 * minus_z, share one array */
static Uint16 elbow_vertices[10] = {
#include "player/eggman/elbow_vertices.inc"
};
static Uint16 finger_vertices[8] = {
#include "player/eggman/finger_vertices.inc"
};
static Uint16 knee_vertices[10] = {
#include "player/eggman/knee_vertices.inc"
};
static Uint16 ankle_vertices[10] = {
#include "player/eggman/ankle_vertices.inc"
};
static Uint16 toe_vertices[16] = {
#include "player/eggman/toe_vertices.inc"
};

/* his welds: the elbows, fingers, knees, ankles and toes. the four
 * WELD_MARK welds after them keep points of his fingers and feet in
 * weld_point0 to weld_point3 */
static struct weld eggman_welds[] = {
    {&eggman_object, &eggman_upper_arm_plus_z, &eggman_forearm_plus_z, 5,
     WELD_BOTH, 0, 0, NULL, elbow_vertices},
    {&eggman_object, &eggman_upper_arm_minus_z, &eggman_forearm_minus_z, 5,
     WELD_BOTH, 0, 0, NULL, elbow_vertices},
    {&eggman_object, &eggman_hand_plus_z, &eggman_fingers_plus_z, 4, WELD_BOTH,
     0, 0, NULL, finger_vertices},
    {&eggman_object, &eggman_hand_minus_z, &eggman_fingers_minus_z, 4,
     WELD_BOTH, 0, 0, NULL, finger_vertices},
    {&eggman_object, &eggman_thigh_plus_z, &eggman_shin_plus_z, 5, WELD_BOTH, 0,
     0, NULL, knee_vertices},
    {&eggman_object, &eggman_thigh_minus_z, &eggman_shin_minus_z, 5, WELD_BOTH,
     0, 0, NULL, knee_vertices},
    {&eggman_object, &eggman_shin_plus_z, &eggman_foot_plus_z, 5, WELD_BOTH, 0,
     0, NULL, ankle_vertices},
    {&eggman_object, &eggman_shin_minus_z, &eggman_foot_minus_z, 5, WELD_BOTH,
     0, 0, NULL, ankle_vertices},
    {&eggman_object, &eggman_foot_plus_z, &eggman_toes_plus_z, 8, WELD_BOTH, 0,
     0, NULL, toe_vertices},
    {&eggman_object, &eggman_foot_minus_z, &eggman_toes_minus_z, 8, WELD_BOTH,
     0, 0, NULL, toe_vertices},
    {&eggman_object, &eggman_fingers_minus_z, NULL, 2, WELD_MARK, 0, 0, NULL,
     NULL},
    {&eggman_object, &eggman_fingers_plus_z, NULL, 2, WELD_MARK + 1, 0, 0, NULL,
     NULL},
    {&eggman_object, &eggman_foot_minus_z, NULL, 0, WELD_MARK + 2, 0, 0, NULL,
     NULL},
    {&eggman_object, &eggman_foot_plus_z, NULL, 0, WELD_MARK + 3, 0, 0, NULL,
     NULL},
    {NULL, NULL, NULL, 0, 0, 0, 0, NULL, NULL},
};

/* his step: the mode he's in moves him, then he's posed and drawn. outside
 * EGGMAN_DOWN, eggman_choose_mode first picks the mode. X pressed on pad 0
 * puts him in the edit mode */
void eggman_step(struct task *task)
{
    struct task_work *work = task->work;
    struct task_motion *motion = task->motion;
    struct player *record = motion->record;

    /* on top of another task's shape, and not rising, he stands on it. his
     * collision record's owner is his own task, so the angles copied from it
     * don't change */
    if ((work->flags & COLLISION_ON_TOP) && motion->speed.y <= 0.0F) {
        work->flags |= PLAYER_ON_TASK;
        work->ang.x = work->collision->owner->work->ang.x;
        work->ang.z = work->collision->owner->work->ang.z;
    }
    if (work->mode != EGGMAN_DOWN) {
        eggman_choose_mode(work, motion, record);
    }
    switch (work->mode) {
    case EGGMAN_INIT:
        PLAYER_VARS(work)->character = CHARACTER_EGGMAN;
        player_init(PLAYER_VARS(work)->player, task);
        record = motion->record;
        record->anim.actions = eggman_actions;
        record->anim.action = ACTION_STAND;
        collision_init(task, eggman_shapes, COLLISION_PLAYER, 1, 1);
        work->collision->shapes->radius = record->consts.radius;
        work->mode = EGGMAN_STAND;
        texture_load_character(CHARACTER_EGGMAN);
        record->anim.welds = eggman_welds;
        task->destructor = eggman_destroy;
        break;
    case EGGMAN_STAND:
        player_turn_to_stick(work, motion, record);
        player_accel_ground(work, motion, record);
        player_apply_accel(work, motion, record);
        player_collide(work, motion, record);
        player_take_speed(work, motion, record);
        break;
    case EGGMAN_WALK:
        if (record->anim.previous == ACTION_TURN &&
            record->anim.current != ACTION_TURN) {
            record->speed.x += record->consts.trot_speed;
        }
        player_accel_ground(work, motion, record);
        player_apply_accel(work, motion, record);
        if (player_collide(work, motion, record) == PLAYER_CRASHED) {
            work->mode = EGGMAN_CRASH;
            record->anim.action = ACTION_CRASH;
            player_crash_burst(work);
            record->speed.x = -record->speed.x;
            record->speed.z = -record->speed.z;
            break;
        }
        player_take_speed(work, motion, record);
        eggman_set_walk_action(work, motion, record);
        break;
    case EGGMAN_PATH:
        player_accel_path(work, motion, record);
        player_apply_path_accel(work, motion, record);
        player_move_on_path(work, motion, record);
        eggman_set_walk_action(work, motion, record);
        break;
    case EGGMAN_PATH_SPIN:
        player_accel_roll(work, motion, record);
        player_apply_path_accel(work, motion, record);
        player_move_on_path(work, motion, record);
        break;
    case EGGMAN_JUMP:
        player_turn_upright(work, motion, record);
        if (!player_steer_to_target(work, motion, record)) {
            player_accel_air(work, motion, record);
        }
        player_apply_accel(work, motion, record);
        player_collide(work, motion, record);
        player_take_speed(work, motion, record);
        break;
    case EGGMAN_SPRING:
        player_accel_air(work, motion, record);
        player_apply_accel(work, motion, record);
        player_collide(work, motion, record);
        player_take_speed(work, motion, record);
        break;
    case EGGMAN_LAUNCH:
        if (record->speed.y >= record->consts.launch_speed) {
            player_accel_launched(work, motion, record);
        } else {
            player_accel_air(work, motion, record);
        }
        player_apply_accel(work, motion, record);
        player_collide(work, motion, record);
        player_take_speed(work, motion, record);
        break;
    case EGGMAN_FALL:
        player_turn_upright(work, motion, record);
        if (!player_steer_to_target(work, motion, record)) {
            player_accel_air(work, motion, record);
        }
        player_apply_accel(work, motion, record);
        player_collide(work, motion, record);
        player_take_speed(work, motion, record);
        break;
    case EGGMAN_STOP:
        player_accel_brake(work, motion, record);
        player_apply_accel(work, motion, record);
        player_collide(work, motion, record);
        player_take_speed(work, motion, record);
        /* stopped, with the stick still pointing back, he turns round */
        if (record->speed.x <= 0.0F &&
            player_stick_back_or_end_event(work, motion)) {
            work->mode = EGGMAN_WALK;
            record->anim.action = ACTION_TURN;
            player_turn_to_stick(work, motion, record);
            work->ang.y = motion->aim.y;
            /* @hack redundant: a redundant break ahead of the case's own, whose
             * jump (BRA at 0x8c0f89aa) has the cases after it call through R3,
             * then R2 (0x8c0f89e0, 0x8c0f89ea); without it, the two trade
             * and 24 of 469 words differ */
            break;
        }
        break;
    case EGGMAN_PUSH:
        player_accel_ground(work, motion, record);
        player_apply_push(work, motion, record);
        player_collide(work, motion, record);
        player_take_speed(work, motion, record);
        break;
    case EGGMAN_HURT:
        player_turn_upright(work, motion, record);
        player_accel_ground(work, motion, record);
        player_apply_accel(work, motion, record);
        player_collide(work, motion, record);
        player_take_speed(work, motion, record);
        break;
    case EGGMAN_BACK:
        player_accel_brake(work, motion, record);
        player_apply_accel(work, motion, record);
        player_collide(work, motion, record);
        player_take_speed(work, motion, record);
        break;
    case EGGMAN_POSE:
        break;
    case EGGMAN_CRASH:
        player_accel_brake(work, motion, record);
        player_apply_accel(work, motion, record);
        player_collide(work, motion, record);
        player_take_speed(work, motion, record);
        break;
    case EGGMAN_EVENT:
        event_paths_step(work);
        break;
    case EGGMAN_DOWN:
        player_turn_upright(work, motion, record);
        player_accel_roll(work, motion, record);
        player_apply_accel(work, motion, record);
        player_collide(work, motion, record);
        player_take_speed(work, motion, record);
        break;
    case EGGMAN_EDIT:
        eggman_edit(work, motion, record);
        break;
    }
    player_anim_step(&record->anim);
    player_update_welds(work, motion, record);
    eggman_draw(work, motion, record);
    if ((pad_records[0].press & PDD_DGT_TX) && work->mode != EGGMAN_EDIT) {
        work->mode = EGGMAN_EDIT;
        work->flags &= ~(PLAYER_ON_PATH | PLAYER_REQUEST | PLAYER_BALL);
    }
    /* the same test as the one above, which sets EGGMAN_EDIT whenever it
     * passes, so this one never passes */
    if ((pad_records[0].press & PDD_DGT_TX) && work->mode != EGGMAN_EDIT) {
        work->mode = EGGMAN_EDIT;
        work->pos.x = EDIT_X;
        work->pos.y = EDIT_Y;
        work->pos.z = EDIT_Z;
    }
    shadow_update(work, &record->shadow);
    collision_register(work);
    player_finish_step(work, motion, record);
}

/* choose his next mode from the one he's in. first his timer counts down.
 * PLAYER_HURT clears while it runs, and PLAYER_RECOILING once it's out. an
 * eggman_try_ call that returns TRUE has settled the mode, and ends the
 * case */
static void eggman_choose_mode(struct task_work *work,
                               struct task_motion *motion,
                               struct player *record)
{
    if (work->timer == 0) {
        work->flags &= ~PLAYER_RECOILING;
    } else {
        work->timer--;
        work->flags &= ~PLAYER_HURT;
    }
    switch (work->mode) {
    case EGGMAN_STAND:
        if (eggman_try_request(work, motion, record)) {
            break;
        }
        if (eggman_try_fall(work, motion, record)) {
            break;
        }
        if (eggman_try_jump(work, motion, record)) {
            break;
        }
        if (player_read_stick(work, NULL, NULL) || record->speed.x != 0.0F) {
            work->mode = EGGMAN_WALK;
            record->anim.action = ACTION_WALK;
            /* @hack redundant: a redundant break ahead of the case's own, whose
             * jump (BRA at 0x8c0f8c66) takes the action store four arms share
             * at 0x8c0f919c, with ACTION_WALK set in its slot; without it, the
             * arm stores its own and 8 of 687 words differ */
            break;
        }
        break;
    case EGGMAN_WALK:
        if (eggman_try_request(work, motion, record)) {
            break;
        }
        if (eggman_try_fall(work, motion, record)) {
            break;
        }
        if (eggman_try_jump(work, motion, record)) {
            break;
        }
        if (eggman_try_stop(work, motion, record)) {
            break;
        }
        /* another task's shape has pushed him at his side. if that shape
         * pushes any group, and he is unhurt and faces it head on, he pushes
         * against it */
        {
            struct collision_work *collision = work->collision;
            int flags = collision->flags;

            if ((flags & COLLISION_PUSHED) && !(work->flags & PLAYER_HURT) &&
                (flags & COLLISION_HIT_SIDE) &&
                (collision->hit->shapes[collision->hit_shape].push &
                 PUSH_ALL)) {
                /* @hack spelling: the wall test as a switch of one case (CMP/EQ
                 * #4,R0 at 0x8c0f8d16), whose stores take ACTION_PUSH in R3
                 * and EGGMAN_PUSH in R2 (0x8c0f8d1c, 0x8c0f8d1e); as an if,
                 * the two trade and 4 of 687 words differ */
                switch (player_face_wall(work, motion)) {
                case PLAYER_WALL_HEAD_ON:
                    work->mode = EGGMAN_PUSH;
                    record->anim.action = ACTION_PUSH;
                    break;
                }
            }
        }
        if (player_stick_back_or_end_event(work, motion) &&
            record->speed.x >= record->consts.trot_speed) {
            work->mode = EGGMAN_STOP;
            record->anim.action = ACTION_SKID;
            /* @hack redundant: a redundant break ahead of the case's own, whose
             * jump (BRA at 0x8c0f8d46) takes the shared action store at
             * 0x8c0f919c, with ACTION_SKID set in its slot; without it, the
             * arm stores its own and 3 of 687 words differ */
            break;
        }
        break;
    case EGGMAN_PATH:
        work->flags &= ~PLAYER_ATTACKING;
        if (eggman_try_request(work, motion, record)) {
            work->flags &= ~PLAYER_ON_PATH;
            break;
        }
        if (eggman_try_fall(work, motion, record)) {
            work->flags &= ~PLAYER_ON_PATH;
            break;
        }
        if (eggman_try_jump(work, motion, record)) {
            work->flags &= ~PLAYER_ON_PATH;
            break;
        }
        if (record->speed.x < record->consts.trot_speed) {
            work->mode = EGGMAN_WALK;
            record->anim.action = ACTION_WALK;
            work->flags &= ~PLAYER_ON_PATH;
            break;
        }
        if (eggman_try_stop(work, motion, record)) {
            work->flags &= ~PLAYER_ON_PATH;
            /* @hack redundant: a redundant break ahead of the case's own, whose
             * jump (BRA at 0x8c0f8dda) takes the PLAYER_ON_PATH store that ends
             * EGGMAN_PATH_SPIN's arms (0x8c0f8e6a); without it, the arm
             * clears the flag itself and 13 of 687 words differ */
            break;
        }
        break;
    case EGGMAN_PATH_SPIN:
        if (eggman_try_request(work, motion, record)) {
            work->flags &= ~PLAYER_ATTACKING;
            work->flags &= ~PLAYER_ON_PATH;
            break;
        }
        if (eggman_try_fall(work, motion, record)) {
            record->anim.action = ACTION_JUMP;
            record->action_speed = 2.0F;
            work->ang.z = 0;
            work->ang.x = 0;
            work->flags |= PLAYER_ATTACKING;
            work->flags &= ~PLAYER_ON_PATH;
            break;
        }
        if (record->speed.x < PLAYER_SPIN_SPEED) {
            record->anim.action = ACTION_JUMP;
            record->action_speed = 2.0F;
            work->ang.z = 0;
            work->ang.x = 0;
            work->flags |= PLAYER_ATTACKING;
            work->flags &= ~PLAYER_ON_PATH;
            /* @hack redundant: a redundant break ahead of the case's own, which
             * ends this arm with the PLAYER_ON_PATH store that EGGMAN_PATH's
             * arms and this case's first two jump to (0x8c0f8e6a); without
             * it, the store ends the fall's arm instead and 99 of 687 words
             * differ */
            break;
        }
        break;
    case EGGMAN_JUMP:
        if (eggman_try_request(work, motion, record)) {
            break;
        }
        if (work->flags & PLAYER_ON_GROUND) {
            if (player_stick_back_or_end_event(work, motion) &&
                record->speed.x > 0.0F) {
                work->mode = EGGMAN_STOP;
                record->anim.action = ACTION_SKID;
            } else if (eggman_try_stop(work, motion, record)) {
                work->ang.x = motion->aim.x;
                work->ang.z = motion->aim.z;
                record->anim.action = ACTION_LAND;
            } else {
                work->ang.x = motion->aim.x;
                work->ang.z = motion->aim.z;
                work->mode = EGGMAN_WALK;
                eggman_set_walk_action(work, motion, record);
            }
            work->flags &= ~PLAYER_ATTACKING;
            break;
        }
        if (player_count_jump_hold(work, record)) {
            record->speed.y += record->consts.jump_hold_speed * 0.8F;
        }
        if (record->anim.action == ACTION_JUMP && motion->speed.y <= 0.0F) {
            record->anim.action = ACTION_FALL_START;
        }
        break;
    /* @hack spelling: EGGMAN_SPRING's and EGGMAN_LAUNCH's tests kept in
     * cases of their own, as the image holds the test twice (0x8c0f8f80,
     * 0x8c0f8f98). as one case, 153 of eggman_choose_mode's 687 words
     * differ */
    case EGGMAN_SPRING:
        if (motion->speed.y <= 0.0F) {
            work->mode = EGGMAN_FALL;
            record->anim.action = ACTION_FALL_START;
        }
        break;
    case EGGMAN_LAUNCH:
        if (motion->speed.y <= 0.0F) {
            work->mode = EGGMAN_FALL;
            record->anim.action = ACTION_FALL_START;
        }
        break;
    case EGGMAN_FALL:
        if (eggman_try_request(work, motion, record)) {
            break;
        }
        if (work->flags & PLAYER_ON_GROUND) {
            if (player_stick_back_or_end_event(work, motion) &&
                record->speed.x > 0.0F) {
                work->mode = EGGMAN_STOP;
                record->anim.action = ACTION_SKID;
            } else if (eggman_try_stop(work, motion, record)) {
                work->ang.x = motion->aim.x;
                work->ang.z = motion->aim.z;
                record->anim.action = ACTION_LAND;
            } else {
                work->ang.x = motion->aim.x;
                work->ang.z = motion->aim.z;
                work->mode = EGGMAN_WALK;
                eggman_set_walk_action(work, motion, record);
            }
        }
        break;
    case EGGMAN_STOP:
        if (eggman_try_request(work, motion, record)) {
            break;
        }
        if (eggman_try_fall(work, motion, record)) {
            break;
        }
        if (eggman_try_jump(work, motion, record)) {
            break;
        }
        if (record->anim.action != ACTION_SKID) {
            work->mode = EGGMAN_STAND;
            record->anim.action = ACTION_STAND;
        }
        break;
    case EGGMAN_PUSH:
        if (eggman_try_request(work, motion, record)) {
            break;
        }
        if (eggman_try_jump(work, motion, record)) {
            break;
        }
        if (eggman_try_fall(work, motion, record)) {
            break;
        }
        if (eggman_try_stop(work, motion, record)) {
            break;
        }
        switch (player_stick_into_wall(work, motion)) {
        case PLAYER_WALL_HEAD_ON:
            break;
        case PLAYER_WALL_ASKEW:
            if (!(work->flags & PLAYER_HOLDING)) {
                work->mode = EGGMAN_WALK;
                record->anim.action = ACTION_WALK;
            }
            break;
        default:
            work->mode = EGGMAN_WALK;
            record->anim.action = ACTION_WALK;
            break;
        }
        break;
    case EGGMAN_HURT:
        if (eggman_try_request(work, motion, record)) {
            break;
        }
        /* @hack type: the timer read as a Uint16, through TASK_TIMER, which
         * the image widens with EXTU.W ahead of its CMP/EQ #120 (0x8c0f9172).
         * an equality test reads either sign alike. as work->timer, the
         * Sint16 field, 45 of 687 words differ */
        if (TASK_TIMER(work) == PLAYER_HURT_STEPS) {
            break;
        }
        if (work->flags & PLAYER_ON_GROUND) {
            if (eggman_try_stop(work, motion, record)) {
                work->ang.x = motion->aim.x;
                work->ang.z = motion->aim.z;
                record->anim.action = ACTION_LAND;
            } else {
                work->ang.x = motion->aim.x;
                work->ang.z = motion->aim.z;
                work->mode = EGGMAN_WALK;
                eggman_set_walk_action(work, motion, record);
            }
        }
        break;
    case EGGMAN_BACK:
        if (eggman_try_request(work, motion, record)) {
            break;
        }
        if (eggman_try_fall(work, motion, record)) {
            break;
        }
        if (eggman_try_jump(work, motion, record)) {
            break;
        }
        if (eggman_try_stop(work, motion, record)) {
            break;
        }
        if (record->speed.x >= 0.0F) {
            work->mode = EGGMAN_WALK;
            eggman_set_walk_action(work, motion, record);
        }
        break;
    case EGGMAN_POSE:
        if (eggman_try_request(work, motion, record)) {
            break;
        }
        break;
    case EGGMAN_CRASH:
        if (eggman_try_fall(work, motion, record)) {
            break;
        }
        /* @hack spelling: the crash's own test and break ahead of the
         * landing's, which have the function keep EGGMAN_WALK's 2 in R10 and
         * ~PLAYER_ON_PATH in R11 (MOV #2,R10 at 0x8c0f8bc8, MOV.W at
         * 0x8c0f8bca); as one test with &&, R8 takes the 2, R10 the mask,
         * and 32 of 687 words differ */
        if (record->anim.action == ACTION_CRASH) {
            break;
        }
        if (record->anim.action != ACTION_CRASH_LAND) {
            work->mode = EGGMAN_STAND;
            record->anim.action = ACTION_STAND;
        }
        break;
    case EGGMAN_EVENT:
        if (work->event->paths == NULL) {
            work->ang.x = motion->aim.x;
            work->ang.z = motion->aim.z;
            work->mode = EGGMAN_STAND;
            record->anim.action = ACTION_STAND;
        }
        break;
    case EGGMAN_EDIT:
        if (pad_records[0].press & PDD_DGT_TA) {
            work->mode = EGGMAN_WALK;
            record->anim.action = ACTION_WALK;
        }
        break;
    }
}

static void eggman_destroy(struct task *task)
{
    /* @hack spelling: task->work read into a local, which the image stores in a
     * frame of its own (ADD #-4,R15 and MOV.L R3,@R15 at 0x8c0f9300,
     * 0x8c0f9306); read in place, 13 of 14 words differ */
    struct task_work *work = task->work;

    player_free(PLAYER_VARS(work)->player);
    texture_release_character(CHARACTER_EGGMAN);
}

/* draw him from his middle, and center his shape there. neither happens
 * while his timer is odd, so he blinks as it runs. in the edit mode,
 * holding X draws his model unposed */
static void eggman_draw(struct task_work *work, struct task_motion *motion,
                        struct player *record)
{
    NJS_VECTOR offset;
    NJS_POINT3 pos;
    struct collision_shape *shape;
    int action;

    action = record->anim.action;
    if (record->anim.state == PLAYER_ANIM_BLENDING) {
        action = record->anim.current;
    }
    /* @hack type: the timer read as a Uint16, through TASK_TIMER, which the
     * image widens with EXTU.W ahead of its TST #1 (0x8c0f9364). a bit test
     * reads either sign alike. as work->timer, the Sint16 field, 139 of 217
     * words differ */
    if (TASK_TIMER(work) & 1) {
        return;
    }
    draw_set_palette(DRAW_PLAYER_PALETTE);
    njSetTexture(&eggman_texlist);
    njPushMatrix(NULL);
    offset.x = offset.z = 0.0F;
    offset.y = record->consts.height * 0.5F;
    njPushMatrix(&_unit_matrix_);
    TURN(njRotateZ, work->ang.z);
    TURN(njRotateX, work->ang.x);
    TURN(njRotateY, -work->ang.y);
    njCalcVector(NULL, &offset, &pos);
    njPopMatrix(1);
    pos.x += work->pos.x;
    pos.y += work->pos.y;
    pos.z += work->pos.z;
    njTranslateV(NULL, &pos);
    shape = work->collision->shapes;
    shape->center.x = pos.x;
    shape->center.y = pos.y;
    shape->center.z = pos.z;
    TURN(njRotateZ, work->ang.z);
    TURN(njRotateX, work->ang.x);
    TURN(njRotateY, NJM_DEG_ANG(180) - work->ang.y);
    if (work->event->motions != NULL) {
        event_motions_draw(work);
    } else if ((pad_records[0].on & PDD_DGT_TX) && work->mode == EGGMAN_EDIT) {
        draw_tree(record->anim.actions[record->anim.action].action->object);
    } else if (record->anim.state != PLAYER_ANIM_BLENDING) {
        draw_action(record->anim.actions[action].action, record->anim.frame);
    } else {
        draw_action(record->anim.blend, record->anim.frame);
    }
    njPopMatrix(1);
    draw_set_palette(0);
}

/* the walk or the run his speed calls for, and how fast it plays */
static void eggman_set_walk_action(struct task_work *work,
                                   struct task_motion *motion,
                                   struct player *record)
{
    Float speed = record->speed.x + (motion->slip * 5.0F);

    if (speed > record->consts.full_speed) {
        record->anim.action = ACTION_FULL_SPEED;
    } else if (speed > record->consts.sprint_speed) {
        record->anim.action = ACTION_SPRINT;
    } else if (speed > record->consts.running_speed) {
        record->anim.action = ACTION_RUN;
    } else if (speed > record->consts.trot_speed) {
        record->anim.action = ACTION_TROT;
    } else {
        record->anim.action = ACTION_WALK;
    }
    record->action_speed = record->speed.x + (motion->slip * 20.0F);
}

/* fly him PLAYER_EDIT_SPEED a step the way the stick points, turned by the
 * camera's heading. while X is held, the stick moves him up and down
 * instead. B puts him back where the stage places him */
static void eggman_edit(struct task_work *work, struct task_motion *motion,
                        struct player *record)
{
    Float x;
    Float y;
    Float sign;
    Angle ang;

    if (pad_records[0].press & PDD_DGT_TB) {
        stage_place_player(work);
    }
    if (pad_records[0].on & PDD_DGT_TX) {
        x = (Float)(pad_records[PLAYER_VARS(work)->player].x1 << 8);
        y = (Float)(pad_records[PLAYER_VARS(work)->player].y1 << 8);
        if (x > PAD_DEAD_ZONE || x < -PAD_DEAD_ZONE || y > PAD_DEAD_ZONE ||
            y < -PAD_DEAD_ZONE) {
            sign = y != 0.0F ? y / njSqrt(y * y) : 0.0F;
            work->pos.y -= sign * PLAYER_EDIT_SPEED;
        }
        return;
    }
    x = (Float)(pad_records[PLAYER_VARS(work)->player].x1 << 8);
    y = (Float)(pad_records[PLAYER_VARS(work)->player].y1 << 8);
    if (x > PAD_DEAD_ZONE || x < -PAD_DEAD_ZONE || y > PAD_DEAD_ZONE ||
        y < -PAD_DEAD_ZONE) {
        ang = (Angle)(atan2f(y, x) * ANGLE_PER_RADIAN) - camera_work->ang.y;
        work->pos.x += njCos(ang) * PLAYER_EDIT_SPEED;
        work->pos.z += njSin(ang) * PLAYER_EDIT_SPEED;
    }
}

/* jump as player_jump_pressed says to: up at his jump speed, plus the
 * speed of the task he stands on. TRUE when he jumps */
static Bool eggman_try_jump(struct task_work *work, struct task_motion *motion,
                            struct player *record)
{
    if (player_jump_pressed(work, record)) {
        work->mode = EGGMAN_JUMP;
        if (record->ground_task != NULL) {
            struct task_force *force = record->ground_task->force;

            if (force != NULL) {
                player_set_accel(PLAYER_VARS(work)->player, force->speed.x,
                                 force->speed.y, force->speed.z);
            }
        }
        record->speed.y = record->consts.jump_speed;
        record->anim.action = ACTION_JUMP;
        record->action_speed = 2.0F;
        work->flags &= ~PLAYER_ON_TASK;
        work->flags |= PLAYER_ATTACKING;
        record->jump_steps = 0;
        sound_play(SE_JUMP, 0, 0, 0);
        return TRUE;
    }
    return FALSE;
}

/* follow the event's action: let it move him, or turn as it says. TRUE
 * for an action he takes. for any other, PLAYER_EVENT_DRIVEN clears, and
 * the result is FALSE */
static Bool eggman_follow_event(struct task_work *work,
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
                           event->move.ang.y + work->ang.y);
        break;
    default:
        work->flags &= ~PLAYER_EVENT_DRIVEN;
        return FALSE;
    }
    return TRUE;
}

/* TRUE while something outside holds his mode: an event's paths, which put
 * him in EGGMAN_EVENT, or PLAYER_REQUEST. while the event's record holds an
 * action, he follows it, and the result is FALSE */
static Bool eggman_try_request(struct task_work *work,
                               struct task_motion *motion,
                               struct player *record)
{
    if (work->event->action) {
        eggman_follow_event(work, motion, record);
        return FALSE;
    }
    if (work->event->paths != NULL) {
        work->mode = EGGMAN_EVENT;
        work->flags &= ~PLAYER_ATTACKING;
        work->flags &= ~PLAYER_ON_PATH;
        return TRUE;
    }
    if (!(work->flags & PLAYER_REQUEST)) {
        return FALSE;
    }
    return TRUE;
}

/* TRUE once he is off the ground, with his mode set to EGGMAN_FALL, or to
 * EGGMAN_JUMP while he's curled up */
static Bool eggman_try_fall(struct task_work *work, struct task_motion *motion,
                            struct player *record)
{
    if (!(work->flags & PLAYER_ON_GROUND)) {
        /* @hack redundant: an empty else, whose jump has the fall's stores take
         * ACTION_FALL in R3 and EGGMAN_FALL in R2 (0x8c0f9940, 0x8c0f9942);
         * without it, the two trade and 4 of 21 words differ */
        if (work->flags & PLAYER_BALL) {
            work->mode = EGGMAN_JUMP;
            return TRUE;
        } else {
        }
        work->mode = EGGMAN_FALL;
        record->anim.action = ACTION_FALL;
        return TRUE;
    }
    return FALSE;
}

/* TRUE once he is still with the stick let go, with his mode set to
 * EGGMAN_STAND */
static Bool eggman_try_stop(struct task_work *work, struct task_motion *motion,
                            struct player *record)
{
    if (!player_read_stick(work, NULL, NULL) && record->speed.x == 0.0F) {
        work->mode = EGGMAN_STAND;
        record->anim.action = ACTION_STAND;
        return TRUE;
    }
    return FALSE;
}
