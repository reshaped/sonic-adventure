/* the Egg Walker's leg copies and its close attack. the copies pose apart
 * from its motion, and the attack stomps with them */
#include <ninja.h>
#include <sg_xpt.h>
#include <shinobi.h>
#include <stdlib.h>

#include "b_egm2/camera.h"
#include "b_egm2/egg_walker.h"
#include "b_egm2/hints.h"
#include "b_egm2/leg_pose.h"
#include "b_egm2/legs.h"
#include "b_egm2/parts.h"
#include "collision/collision.h"
#include "draw/model.h"
#include "draw/motion.h"
#include "effect/bomb.h"
#include "player/player.h"
#include "sound/sound.h"
#ifdef GCC
#include "system/frame.h"
#endif
#include "system/pad.h"
#include "system/task.h"
#include "system/util.h"

/* the copies' last blend. raise_leg and lower_leg set POSE_RAISE and
 * POSE_LOWER, which stay set once the blend ends. POSE_ALL marks the walk or
 * rest blend of all four, and gives way to POSE_IDLE as it ends */
enum leg_pose_mode { POSE_IDLE, POSE_RAISE, POSE_LOWER, POSE_ALL };

/* the close attack's work->submode. CLOSE_START picks the legs to stomp
 * with, more as the boss's hits run out. CLOSE_PICK takes one of them at
 * random. the attack raises it, holds it, lowers it and stomps, which
 * leaves the leg in LEG_GLOW. CLOSE_WAIT_HITS gives the glows time to be
 * hit. once no leg glows or waits to stomp, CLOSE_FALL drops the boss with its
 * cockpit open. CLOSE_RECOVER stands it up again */
enum close_mode {
    CLOSE_START,
    CLOSE_PICK,
    CLOSE_RAISE,
    CLOSE_WAIT_RAISE,
    CLOSE_HOLD_RAISED,
    CLOSE_LOWER,
    CLOSE_WAIT_LOWER,
    CLOSE_WAIT_HITS,
    CLOSE_FALL,
    CLOSE_FALLING,
    CLOSE_EXPOSED,
    CLOSE_RECOVER,
    CLOSE_RECOVERING
};

/* the steps of a raise and of a lower, and of the blend that
 * egg_walker_legs_walk and egg_walker_legs_rest start */
#define RAISE_STEPS 40
#define LOWER_STEPS 20
#define BLEND_STEPS 15

/* how far a raised copy lifts its leg's shapes */
#define LEG_RISE 35.0F

/* RAISED_HOLD times a raised leg's hold, and HIT_WAIT the glows' wait for
 * hits. EXPOSED_WAIT times the fallen boss's time open to hits. each timer
 * counts down past 0, so it runs one step longer than its value */
#define RAISED_HOLD 5
#define HIT_WAIT 233
#define EXPOSED_WAIT 180

#define SE_FALL 0x1d6
#define SE_STOMP 0x1d8
/* SE_SETTLE plays as the boss's fall ends, and again as it starts to rise */
#define SE_SETTLE 0x1df

/* a node of a leg's copy, on the heap. it owns its transform and its links,
 * and shares its model with the boss's own leg */
struct copy_node {
    Float pos[3];
    Angle ang[3];
    Float scl[3];
    NJS_MODEL *model;
    struct copy_node *child;
    struct copy_node *sibling;
};

/* a copy's pose. ang0, ang1 and ang2 point at the angles of three nodes down
 * the copy. pos points at the position of the node below the third. rest_
 * holds their values as copied, and step_ what each step of a blend adds */
struct leg_pose {
    /* @hack type: three named pointers, not Angle *ang[3], which start
     * egg_walker_leg_copies_init's copy loop from the 0 already in R13 (MOV
     * R13,R4 at 0x0c904c64); as ang[3], the loop loads a 0 of its own and 1
     * of 161 words differ */
    Angle *ang0;
    Angle *ang1;
    Angle *ang2;
    Float *pos;
    Angle rest_ang0[3];
    Angle rest_ang1[3];
    Angle rest_ang2[3];
    Float rest_pos[3];
    Angle step_ang0[3];
    Angle step_ang1[3];
    Angle step_ang2[3];
    Float step_pos[3];
};

/* a leg's copy. blending says its pose takes the blend's steps. shown says
 * it draws in place of the boss's own leg */
struct leg_copy {
    struct copy_node *root;
    struct leg_pose pose;
    int blending;
    int shown;
};

/* how far raise_leg turns a copy's nodes: the first two about z, the third
 * about x */
struct leg_fold {
    Angle first_z;
    Angle second_z;
    Angle third_x;
};

static const struct leg_fold fold_angles[LEGS] = {
#include "b_egm2/leg_pose/fold_angles.inc"
};

static const struct angles dat_0c908980[4][3] = {
#include "b_egm2/leg_pose/dat_0c908980.inc"
};

/* each leg's point, as a part. its foot is LEGS parts on, and the point fore
 * or aft of its foot LEGS more */
static const Uint8 leg_parts[LEGS] = {2, 3, 4, 5};

/* the leg choose_legs marks first. a coin flip picks between legs 0 and 1.
 * with the camera beyond the boss, it picks between legs 2 and 3. CLOSE_PICK
 * raises the marked legs in random order */
static const Uint8 first_legs[2][2] = {{0, 1}, {2, 3}};

/* each copy's walking pose: its three nodes' angles, then pos */
static const Float walk_poses[LEGS][4][3] = {
#include "b_egm2/leg_pose/walk_poses.inc"
};

extern NJS_OBJECT dat_0c910078, dat_0c90e934, dat_0c90d1f0, dat_0c90baac;

/* the boss's own legs, which egg_walker_leg_copies_init copies */
static NJS_OBJECT *leg_models[LEGS] = {&dat_0c910078, &dat_0c90e934,
                                       &dat_0c90d1f0, &dat_0c90baac};

/* the speed a hit on the cockpit gives player 0, 0.5 straight up in the
 * player's frame. player_turn_to_world turns it to the world in place. a hit
 * on a player tilted in x or z leaves it tilted. the next hit then turns it
 * again */
static NJS_VECTOR bounce = {0.0F, 0.5F, 0.0F};
static struct leg_copy leg_copies[LEGS];

/* the steps left in the copies' blend */
static int blend_left;
/* the leg the close attack moves */
static int active_leg;
static Uint8 pose_mode; /* a leg_pose_mode */
/* the picked legs still to stomp */
static int stomps_left;
/* whether the boss has fallen in this close attack */
static int fell;
/* whether its cockpit has taken a hit in this close attack */
static int cockpit_hit;
/* the leg pair that holds the last leg to stomp. pair 0 is legs 0 and 1,
 * and pair 1 legs 2 and 3 */
static int fall_pair;

static void copy_tree(struct copy_node *node, NJS_OBJECT *object, int top);
static void draw_copy(struct copy_node *node);
static void raise_leg(int index);
static void lower_leg(int index);
static void match_legs_to_copies(void);
static void set_hidden(NJS_OBJECT *object, int top, int hidden);
static void take_leg_hits(struct task *task);
static int take_cockpit_hit(struct task *task);

void egg_walker_leg_copies_init(void)
{
    int i;
    int j;
    struct leg_copy *copy;
    struct leg_pose *pose;

    for (i = 0; i < LEGS; i++) {
        copy = &leg_copies[i];
        copy->root = heap_alloc(sizeof(struct copy_node));
        copy_tree(copy->root, leg_models[i], 1);
        pose = &copy->pose;
        pose->ang0 = copy->root->child->ang;
        pose->ang1 = copy->root->child->child->ang;
        pose->ang2 = copy->root->child->child->child->sibling->child->ang;
        pose->pos = copy->root->child->child->child->sibling->child->child->pos;
        for (j = 0; j < 3; j++) {
            pose->rest_ang0[j] = pose->ang0[j];
            pose->rest_ang1[j] = pose->ang1[j];
            pose->rest_ang2[j] = pose->ang2[j];
            pose->rest_pos[j] = pose->pos[j];
        }
        copy->blending = 0;
        copy->shown = 0;
    }
    match_legs_to_copies();
    blend_left = 0;
    pose_mode = POSE_IDLE;
}

/* copy object's tree into node, giving each child and sibling a node of its
 * own. with top, object's siblings stay out */
static void copy_tree(struct copy_node *node, NJS_OBJECT *object, int top)
{
    int i;

    for (i = 0; i < 3; i++) {
        node->pos[i] = object->pos[i];
        node->ang[i] = object->ang[i];
        node->scl[i] = object->scl[i];
    }
    node->model = object->model;
    if (object->child) {
        node->child = heap_alloc(sizeof(struct copy_node));
        copy_tree(node->child, object->child, 0);
    } else {
        node->child = 0;
    }
    if (object->sibling && !top) {
        node->sibling = heap_alloc(sizeof(struct copy_node));
        copy_tree(node->sibling, object->sibling, 0);
    } else {
        node->sibling = 0;
    }
}

void egg_walker_leg_copies_draw(struct task *task)
{
    struct task_work *work = task->work;
    Angle angle;
    int i;
    struct leg_copy *copy;

    njPushMatrix(0);
    njTranslateV(0, &work->pos);
    TURN_VIA(njRotateY, 0x8000 - work->ang.y, angle);
    njPushMatrix(0);
    njMultiMatrix(0, egg_walker_body_matrix());
    /* @hack redundant: a never-run store of 0 to i, which keeps 0 in R10 across
     * both loops (0x0c904ecc); without it, the function saves one register
     * fewer, runs 6 bytes short, draw_copy after it moves, and 129 of 133 words
     * differ */
    if (0) {
        i = 0;
    }
    for (i = 0; i < LEGS; i++) {
        copy = &leg_copies[i];
        if (copy->shown) {
            draw_copy(copy->root);
        }
    }
    njPopMatrix(1);
    njPopMatrix(1);
#ifdef GCC
    /* the split frame's second picture reads no press. the pass's press
     * holds every step's, where 30 fps reads step 0's alone */
    if (frame_redrawing) {
        return;
    }
#endif
    if (per_info[0]->press & PDD_DGT_KU) {
        for (i = 0; i < LEGS; i++) {
            raise_leg(i);
        }
    }
    if (per_info[0]->press & PDD_DGT_KD) {
        for (i = 0; i < LEGS; i++) {
            lower_leg(i);
        }
    }
}

static void draw_copy(struct copy_node *node)
{
    Angle angle;

    njPushMatrix(0);
    njTranslate(0, node->pos[0], node->pos[1], node->pos[2]);
    TURN_VIA(njRotateZ, node->ang[2], angle);
    TURN_VIA(njRotateY, node->ang[1], angle);
    TURN_VIA(njRotateX, node->ang[0], angle);
    njScale(0, node->scl[0], node->scl[1], node->scl[2]);
    if (node->model) {
        draw_model(node->model);
    }
    if (node->child) {
        draw_copy(node->child);
    }
    njPopMatrix(1);
    if (node->sibling) {
        draw_copy(node->sibling);
    }
}

static void clear_steps(struct leg_pose *pose)
{
    int i;

    for (i = 0; i < 3; i++) {
        pose->step_ang0[i] = 0;
        pose->step_ang1[i] = 0;
        pose->step_ang2[i] = 0;
        pose->step_pos[i] = 0.0F;
    }
}

static void restore_pose(struct leg_pose *pose)
{
    int i;

    for (i = 0; i < 3; i++) {
        pose->ang0[i] = pose->rest_ang0[i];
        pose->ang1[i] = pose->rest_ang1[i];
        pose->ang2[i] = pose->rest_ang2[i];
        pose->pos[i] = pose->rest_pos[i];
    }
}

/* put leg index's copy back at rest, then fold it by fold_angles over
 * RAISE_STEPS */
static void raise_leg(int index)
{
    struct leg_copy *copy = &leg_copies[index];
    struct leg_pose *pose;

    blend_left = RAISE_STEPS;
    pose = &copy->pose;
    copy->blending = 1;
    clear_steps(pose);
    restore_pose(pose);
    pose->step_ang0[2] = fold_angles[index].first_z / blend_left;
    pose->step_ang1[2] = fold_angles[index].second_z / blend_left;
    pose->step_ang2[0] = fold_angles[index].third_x / blend_left;
    pose_mode = POSE_RAISE;
}

/* blend leg index's copy from where it is back to rest over LOWER_STEPS */
static void lower_leg(int index)
{
    struct leg_copy *copy = &leg_copies[index];
    struct leg_pose *pose;
    int i;

    blend_left = LOWER_STEPS;
    pose = &copy->pose;
    copy->blending = 1;
    clear_steps(pose);
    for (i = 0; i < 3; i++) {
        pose->step_ang0[i] = (pose->rest_ang0[i] - pose->ang0[i]) / blend_left;
        pose->step_ang1[i] = (pose->rest_ang1[i] - pose->ang1[i]) / blend_left;
        pose->step_ang2[i] = (pose->rest_ang2[i] - pose->ang2[i]) / blend_left;
        pose->step_pos[i] = (pose->rest_pos[i] - pose->pos[i]) / blend_left;
    }
    pose_mode = POSE_LOWER;
}

void egg_walker_leg_pose_step(void)
{
    int i;
    int j;
    struct leg_copy *copy;
    struct leg_pose *pose;

    if (blend_left > 0) {
        for (i = 0; i < LEGS; i++) {
            copy = &leg_copies[i];
            if (copy->blending) {
                pose = &copy->pose;
                for (j = 0; j < 3; j++) {
                    pose->ang0[j] += pose->step_ang0[j];
                    pose->ang1[j] += pose->step_ang1[j];
                    pose->ang2[j] += pose->step_ang2[j];
                    pose->pos[j] += pose->step_pos[j];
                }
            }
        }
        if (--blend_left <= 0) {
            for (i = 0; i < LEGS; i++) {
                copy = &leg_copies[i];
                copy->blending = 0;
                if (pose_mode == POSE_ALL) {
                    copy->shown = 0;
                }
            }
            if (pose_mode == POSE_ALL) {
                match_legs_to_copies();
                pose_mode = POSE_IDLE;
            }
        }
    }
}

void egg_walker_leg_shapes_lift(struct task_work *work)
{
    int remaining = blend_left;
    Float rise;
    int i;
    int j;
    struct leg_copy *copy;

#ifdef GCC
    /* GCC can't see that rise is set wherever it is read: pose_mode holds
     * only the four modes the switch has cases for, and each case that
     * doesn't return sets it */
    rise = 0.0F;
#endif
    switch (pose_mode) {
    case POSE_RAISE:
        /* each rise multiplies by an inverse step count. SHC folds this
         * product to the image's 0.87499994, and LEG_RISE / RAISE_STEPS to
         * 0.875 */
        if (remaining > 0) {
            rise =
                (RAISE_STEPS - remaining) * (LEG_RISE * (1.0F / RAISE_STEPS));
        } else {
            rise = LEG_RISE;
        }
        break;
    case POSE_LOWER:
        if (remaining > 0) {
            rise = remaining * (LEG_RISE * (1.0F / LOWER_STEPS));
        } else {
            rise = 0.0F;
        }
        break;
    case POSE_IDLE:
        return;
    case POSE_ALL:
        return;
    }
    for (i = 0; i < LEGS; i++) {
        copy = &leg_copies[i];
        if (copy->shown) {
            for (j = 0; j < 3; j++) {
                work->collision->shapes[leg_parts[i] + (j * LEGS)].center.y +=
                    rise;
            }
        }
    }
}

/* hide each of the boss's own legs whose copy is shown, and show the
 * others */
static void match_legs_to_copies(void)
{
    int i;
    struct leg_copy *copy;

    for (i = 0; i < LEGS; i++) {
        copy = &leg_copies[i];
        if (copy->shown) {
            if (!(leg_models[i]->evalflags & NJD_EVAL_HIDE)) {
                set_hidden(leg_models[i], 1, 1);
            }
        } else {
            if (leg_models[i]->evalflags & NJD_EVAL_HIDE) {
                set_hidden(leg_models[i], 1, 0);
            }
        }
    }
}

/* set NJD_EVAL_HIDE through object's tree when hidden is nonzero, and clear
 * it otherwise. with top, object's siblings stay as they are */
static void set_hidden(NJS_OBJECT *object, int top, int hidden)
{
    if (object) {
        if (hidden) {
            object->evalflags |= NJD_EVAL_HIDE;
        } else {
            object->evalflags &= ~NJD_EVAL_HIDE;
        }
        if (object->child) {
            set_hidden(object->child, 0, hidden);
        }
        if (object->sibling && !top) {
            set_hidden(object->sibling, 0, hidden);
        }
    }
}

/* mark the legs a close attack stomps with LEG_SELECTED, and return how
 * many. it takes one with 4 hits left or more, two with 2 or 3, and three
 * with fewer. the first comes from first_legs, and the rest at random */
static int choose_legs(struct egg_walker *boss)
{
    int count;
    int total;
    int i;
    int side;
    int index;

    switch ((int)boss->health) {
    default:
    case 5:
    case 4:
        total = count = 1;
        break;
    case 2:
    case 3:
        total = count = 2;
        break;
    case 0:
    case 1:
        total = count = 3;
        break;
    }
    for (i = 0; i < LEGS; i++) {
        boss->legs[i] = LEG_NONE;
    }
    side = rand() * RAND_SCALE < 0.5F ? 1 : 0;
    index = egg_walker_camera_beyond_boss() ? 1 : 0;
    index = first_legs[index][side];
    boss->legs[index] = LEG_SELECTED;
    --count;
    while (count > 0) {
        index = (int)(rand() * (4.0F * RAND_SCALE));
        if (boss->legs[index]) {
            continue;
        }
        boss->legs[index] = LEG_SELECTED;
        --count;
    }
    return total;
}

void egg_walker_close(struct task *task)
{
    struct task_work *work = task->work;
    struct egg_walker *boss = task->extra;
    int current_action;
    int i;

    switch (work->submode) {
    case CLOSE_START:
        cockpit_hit = 0;
        fell = 0;
        stomps_left = choose_legs(boss);
        work->submode = CLOSE_PICK;
        /* fall through */
    case CLOSE_PICK:
        do {
            active_leg = (int)(rand() * (4.0F * RAND_SCALE));
        } while (boss->legs[active_leg] != LEG_SELECTED);
        work->submode = CLOSE_RAISE;
        break;
    case CLOSE_RAISE:
        raise_leg(active_leg);
        leg_copies[active_leg].shown = 1;
        match_legs_to_copies();
        work->submode = CLOSE_WAIT_RAISE;
        break;
    case CLOSE_WAIT_RAISE:
        if (blend_left <= 0) {
            EGG_WALKER_VARS(work)->timer = RAISED_HOLD;
            work->submode = CLOSE_HOLD_RAISED;
        }
        break;
    case CLOSE_HOLD_RAISED:
        if (--EGG_WALKER_VARS(work)->timer < 0) {
            work->submode = CLOSE_LOWER;
        }
        break;
    case CLOSE_LOWER:
        egg_walker_voice(0);
        lower_leg(active_leg);
        work->submode = CLOSE_WAIT_LOWER;
        break;
    case CLOSE_WAIT_LOWER:
        if (blend_left <= 0) {
            sound_play(SE_STOMP, 0, 0, 0);
            egg_walker_camera_shake(3);
            leg_copies[active_leg].shown = 0;
            egg_walker_stomp_create(active_leg);
            match_legs_to_copies();
            boss->legs[active_leg] = LEG_GLOW;
            if (--stomps_left <= 0) {
                EGG_WALKER_VARS(work)->timer = HIT_WAIT;
                work->submode = CLOSE_WAIT_HITS;
            } else {
                work->submode = CLOSE_PICK;
            }
        }
        break;
    case CLOSE_WAIT_HITS:
        if (--EGG_WALKER_VARS(work)->timer < 0) {
            for (i = 0; i < LEGS; i++) {
                boss->legs[i] = LEG_NONE;
            }
            work->submode = MODE_DONE;
        }
        break;
    case CLOSE_FALL:
        fell = 1;
        switch (active_leg) {
        case 0:
        case 1:
            fall_pair = 0;
            break;
        case 2:
        case 3:
            fall_pair = 1;
            break;
        }
        boss->motion.wanted =
            !fall_pair ? EGG_WALKER_ACTION_FALL_01 : EGG_WALKER_ACTION_FALL_23;
        work->collision->shapes[PART_COCKPIT].levels &= ~SHAPE_GUARD;
        sound_play(SE_FALL, 0, 0, 0);
        egg_walker_voice(1);
        egg_walker_count_fall();
        work->submode = CLOSE_FALLING;
        break;
    case CLOSE_FALLING: {
        Uint16 motion = boss->motion.current;
        NJS_ACTION *action = boss->motion.actions[motion].action;
        struct collision_shape *body = work->collision->shapes;
        egg_walker_camera_look_past();
        /* the body's shape attacks while it is above player 0 */
        if (egg_walker_part_pos(PART_BODY)->y > player_work[0]->pos.y) {
            body->levels &= ~SHAPE_ATTACK;
            body->levels |= SHAPE_ATTACK;
        } else {
            body->levels &= ~SHAPE_ATTACK;
        }
        if (take_cockpit_hit(task)) {
            cockpit_hit = 1;
        }
        if (motion == (!fall_pair ? EGG_WALKER_ACTION_FALL_01
                                  : EGG_WALKER_ACTION_FALL_23)) {
            if (boss->motion.frame >= action->motion->nbFrame - 1.0F) {
                body->levels &= ~SHAPE_ATTACK;
                sound_play(SE_SETTLE, 0, 0, 0);
                if (cockpit_hit) {
                    EGG_WALKER_VARS(work)->timer = 0;
                } else {
                    EGG_WALKER_VARS(work)->timer = EXPOSED_WAIT;
                }
                if (boss->health <= 0.0F) {
                    work->submode = MODE_DONE;
                } else {
                    work->submode = CLOSE_EXPOSED;
                }
            }
        }
    } break;
    case CLOSE_EXPOSED:
        egg_walker_camera_look_past();
        if (--EGG_WALKER_VARS(work)->timer < 0) {
            work->submode = CLOSE_RECOVER;
        } else if (take_cockpit_hit(task)) {
            cockpit_hit = 1;
            if (boss->health <= 0.0F) {
                work->submode = MODE_DONE;
            } else {
                work->submode = CLOSE_RECOVER;
            }
        }
        break;
    case CLOSE_RECOVER:
        work->collision->shapes[PART_COCKPIT].levels &= ~SHAPE_GUARD;
        work->collision->shapes[PART_COCKPIT].levels |= SHAPE_GUARD;
        boss->motion.wanted =
            !fall_pair ? EGG_WALKER_ACTION_RISE_01 : EGG_WALKER_ACTION_RISE_23;
        sound_play(SE_SETTLE, 0, 0, 0);
        work->submode = CLOSE_RECOVERING;
        break;
    case CLOSE_RECOVERING:
        /* @hack redundant: a never-run store of 1 to cockpit_hit, which gives
         * the constant 1, not 4, the image's R10 (0x0c9059c2, 0x0c9059c6);
         * without it, the two trade and 17 of 528 words differ */
        if (0) {
            cockpit_hit = 1;
        }
        current_action = boss->motion.current;
        if (current_action == EGG_WALKER_ACTION_STAND) {
            work->submode = MODE_DONE;
        }
        break;
    }
    switch (work->submode) {
    case CLOSE_PICK:
    case CLOSE_RAISE:
    case CLOSE_WAIT_RAISE:
    case CLOSE_HOLD_RAISED:
    case CLOSE_LOWER:
    case CLOSE_WAIT_LOWER:
    case CLOSE_WAIT_HITS:
        egg_walker_aim_turret(task);
        take_leg_hits(task);
        if (egg_walker_legs_spent(task)) {
            work->submode = CLOSE_FALL;
        }
        break;
    }
    if (work->submode == MODE_DONE) {
        for (i = 0; i < LEGS; i++) {
            boss->legs[i] = LEG_NONE;
        }
        if (cockpit_hit) {
            if (boss->health <= 0.0F) {
                if (work->mode != EGG_WALKER_DEFEAT) {
                    /* @hack spelling: boss_next_mode set inside the expression
                     * that sets boss_last_mode, which puts boss_next_mode's
                     * address in R11 and boss_last_mode's in R12, and their
                     * pool words in that order at 0x0c905ecc. as two
                     * statements, the two trade and 10 of 528 words differ */
                    boss_last_mode =
                        (boss_next_mode = EGG_WALKER_DEFEAT, work->mode);
                }
            } else {
                if (boss->health == 1.0F) {
                    egg_walker_voice(4);
                }
                if (work->mode != EGG_WALKER_JUMP) {
                    boss_next_mode = EGG_WALKER_JUMP;
                    boss_last_mode = work->mode;
                }
            }
        } else if (fell) {
            if (work->mode != EGG_WALKER_JUMP) {
                boss_next_mode = EGG_WALKER_JUMP;
                boss_last_mode = work->mode;
            }
        } else {
            /* the boss didn't fall. unless alternate_target_x is set
             * already, it targets the street's far end and turns to its
             * bombs. otherwise it walks on */
            if (boss->alternate_target_x == 0) {
                boss->target = *egg_walker_part_pos(PART_BODY);
                if (work->pos.z < WALK_Z_MIDDLE) {
                    boss->target.z = WALK_Z_MAX;
                } else {
                    boss->target.z = WALK_Z_MIN;
                }
                boss->alternate_target_x = 1;
                boss->last_alternate_z = work->pos.z;
                if (boss->walk == WALK_STOPPED) {
                    boss->walk = WALK_START;
                }
                if (work->mode != EGG_WALKER_BOMBS) {
                    boss_next_mode = EGG_WALKER_BOMBS;
                    boss_last_mode = work->mode;
                }
            } else if (work->mode != EGG_WALKER_WALK) {
                boss_next_mode = EGG_WALKER_WALK;
                boss_last_mode = work->mode;
            }
        }
    }
}

/* turn each glowing leg whose shape was hurt this step to LEG_FADE. each
 * makes a puff at the shape and keeps the smoke's sound on */
static void take_leg_hits(struct task *task)
{
    struct motion *motion;
    struct task_work *work;
    int i;
    struct egg_walker *boss;
    struct collision_shape *hit;
    work = task->work;
    /* @hack spelling: the record read as a motion, then cast to the boss's,
     * which keeps its pointer in a frame slot (MOV.L R3,@R15 at 0x0c905fc2).
     * read as the boss's alone, the frame loses the slot and 75 of 76 words
     * differ */
    motion = task->extra;
    boss = (struct egg_walker *)motion;
    for (i = 0; i < LEGS; i++) {
        hit = work->collision->shapes + i + PART_LEGS;
        switch (boss->legs[i]) {
        case LEG_NONE:
            break;
        case LEG_GLOW:
            if (hit->levels & SHAPE_HURT) {
                egg_walker_smoke_hold();
                /* @hack spelling: the shape's address spelled again, not hit,
                 * which SHC keeps as a cursor of its own in R11, 52 bytes on
                 * each pass (0x0c90601e). as &hit->center, 61 of 76 words
                 * differ */
                bomb_create(&(work->collision->shapes + i + PART_LEGS)->center,
                            0.25F);
                boss->legs[i] = LEG_FADE;
            }
            break;
        case LEG_SELECTED:
            break;
        case LEG_FADE:
            break;
        }
    }
}

/* take a hit on the cockpit, part PART_COCKPIT, when its shape was hurt
 * this step. the boss loses a hit, a puff bursts at player 0 with
 * SE_WALKER_BLAST, and the player bounces up. the cockpit then guards fully
 * again. it returns 1 for a hit, else 0 */
static int take_cockpit_hit(struct task *task)
{
    struct task_work *work = task->work;
    struct egg_walker *boss = task->extra;

    struct collision_shape *cockpit = &work->collision->shapes[PART_COCKPIT];

    if (cockpit->levels & SHAPE_HURT) {
        boss->health -= 1.0F;
        egg_walker_voice(2);
        if (boss->health <= 0.0F) {
            boss->health = 0.0F;
        }
        sound_play(SE_WALKER_BLAST, 0, 0, 0);
        bomb_create(&player_work[0]->pos, 0.5F);
        player_turn_to_world(player_work[0], &bounce);

        /* @hack spelling: x read through bounce's address, which gives each
         * component a pool word of its own (0x0c906118 to 0x0c906120). read
         * as bounce.x, SHC holds the vector's address in R13 and 7 of 75
         * words differ */
        player_set_speed(0, (&bounce)->x, bounce.y, bounce.z);
        work->collision->shapes[PART_COCKPIT].levels &= ~SHAPE_GUARD;
        work->collision->shapes[PART_COCKPIT].levels |= SHAPE_GUARD;
        return 1;
    }
    return 0;
}

void egg_walker_legs_walk(void)
{
    int i;
    int j;
    struct leg_copy *copy;
    struct leg_pose *pose;

    for (i = 0; i < LEGS; i++) {
        copy = &leg_copies[i];
        pose = &copy->pose;
        blend_left = BLEND_STEPS;
        copy->blending = 1;
        clear_steps(pose);
        /* each angle turns the short way round */
        for (j = 0; j < 3; j++) {
            int delta;
            pose->step_ang0[j] =
                ((delta = ((Angle)walk_poses[i][0][j] - pose->ang0[j]) &
                          0xffff) <= 0x8000
                     ? delta
                     : delta - 0x10000) /
                blend_left;
            pose->step_ang1[j] =
                ((delta = ((Angle)walk_poses[i][1][j] - pose->ang1[j]) &
                          0xffff) <= 0x8000
                     ? delta
                     : delta - 0x10000) /
                blend_left;
            pose->step_ang2[j] =
                ((delta = ((Angle)walk_poses[i][2][j] - pose->ang2[j]) &
                          0xffff) <= 0x8000
                     ? delta
                     : delta - 0x10000) /
                blend_left;
            pose->step_pos[j] =
                (walk_poses[i][3][j] - pose->pos[j]) / blend_left;
        }
        copy->shown = 1;
    }
    match_legs_to_copies();
    pose_mode = POSE_ALL;
}

void egg_walker_legs_rest(void)
{
    int i;
    int j;
    struct leg_copy *copy;
    struct leg_pose *pose;

    blend_left = BLEND_STEPS;
    for (i = 0; i < LEGS; i++) {
        copy = &leg_copies[i];
        copy->shown = 1;
        pose = &copy->pose;
        copy->blending = 1;
        clear_steps(pose);
        for (j = 0; j < 3; j++) {
            pose->step_ang0[j] =
                (pose->rest_ang0[j] - pose->ang0[j]) / blend_left;
            pose->step_ang1[j] =
                (pose->rest_ang1[j] - pose->ang1[j]) / blend_left;
            pose->step_ang2[j] =
                (pose->rest_ang2[j] - pose->ang2[j]) / blend_left;
            pose->step_pos[j] = (pose->rest_pos[j] - pose->pos[j]) / blend_left;
        }
    }
    match_legs_to_copies();
    pose_mode = POSE_ALL;
}

int egg_walker_all_legs_blending(void)
{
    return pose_mode == POSE_ALL;
}
