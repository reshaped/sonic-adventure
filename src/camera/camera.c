/* the camera task: its modes and their levels, the act's collisions that
 * pick them, the overrides, and the tint under water */
#include <mathf.h>
#include <ninja.h>
#include <sg_xpt.h>
#include <shinobi.h>

#include "camera/camera.h"
#include "camera/editor.h"
#include "camera/layouts.h"
#include "collision/polygon.h"
#include "collision/world.h"
#include "draw/draw.h"
#include "draw/light.h"
#include "main.h"
#include "player/player.h"
#include "stage/stage.h"
#include "system/pad.h"
#include "system/task.h"
#include "system/util.h"

/* the field of view the camera starts at */
#define PERSPECTIVE NJM_DEG_ANG(70)
/* the most the FIXED modes move the field of view toward PERSPECTIVE in a
 * call */
#define PERSPECTIVE_STEP 18

/* the d-pad starts the look round; a face button ends it, as a stick does */
#define SURVEY_START (PDD_DGT_KU | PDD_DGT_KD | PDD_DGT_KL | PDD_DGT_KR)
#define SURVEY_END (PDD_DGT_TA | PDD_DGT_TB | PDD_DGT_TX | PDD_DGT_TY)
/* the pad starts the survey only once camera_restored_steps passes this */
#define SURVEY_DELAY 10

/* the adjust modes run only once camera_steps passes this */
#define ADJUST_DELAY 60

/* a change between two collisions of at least this priority takes adjust
 * mode NONE */
#define PRIORITY_HIGH 10

/* the steps player 0 loses control for when the view cuts */
#define CUT_STEPS 30

/* the collisions test player 0 at this height above his feet */
#define TEST_HEIGHT 4.0F

/* player 0 is still while he is on land and moves less than STILL_MOVE in
 * a step. the camera is still while it moves less than STILL_MOVE in a
 * step, and turns less than STILL_TURN about each axis */
#define STILL_MOVE 0.1F
#define STILL_TURN NJM_DEG_ANG(0.5)

/* the steps camera_ease takes */
#define EASE_STEPS 30

/* a height past any water, above the camera or below it */
#define NO_WATER 1000000.0F

/* the camera task's work mode */
enum camera_task_mode { CAMERA_TASK_INIT, CAMERA_TASK_START, CAMERA_TASK_RUN };

/* camera_action_state's values */
enum action_state {
    ACTION_NONE,
    ACTION_ASKED, /* camera_play_action gave one since the camera's last step */
    /* the camera's last step played one, which its next ends unless
     * camera_play_action gives one again */
    ACTION_PLAYED
};

struct camera_place camera_place;
Uint8 dat_8c6c0a30[4];
Bool camera_player_still;
NJS_VECTOR camera_moved;
struct task_work camera_last_record;
struct camera_state camera_state_record;
Uint8 dat_8c6c0ad8[12];
struct camera_control camera_control_record;
struct camera_view camera_step_view;
struct camera_view camera_override_view;
struct camera_adjust_work camera_adjust_record;
Sint16 camera_stage;
Sint16 camera_act;
Sint16 camera_mode_count;
struct camera_mode *camera_modes;
Sint16 camera_adjust_count;
struct camera_adjust_mode *camera_adjusts;
Sint32 *camera_collision_count;
struct camera_collision *camera_collisions;
Angle camera_perspective;
Sint32 camera_action_state;
NJS_CACTION *camera_action;
Float camera_action_frame;
Uint32 camera_steps;
Uint32 camera_restored_steps;
Sint32 camera_start_mode;
Sint32 camera_restore_mode;
Sint32 camera_restore_adjust;
Bool camera_cut;
Sint32 camera_step_count;
struct task_work *camera_take_work;
Uint8 dat_8c6c0c30[4];
NJS_POINT3 camera_player_last;
NJS_POINT3 camera_last_pos;
Bool camera_in_dry_box;

/* the camera's work as it was before the mode's step */
static struct task_work *const camera_last = &camera_last_record;
struct camera_state *const camera_state = &camera_state_record;
static struct camera_control *const camera_control = &camera_control_record;
static struct camera_adjust_work *const camera_adjust = &camera_adjust_record;
/* how far above its target the camera starts, before player 0's angles
 * turn it */
static const NJS_VECTOR start_above = {0.0F, 20.0F, 0.0F};
/* the name for a step no mode names, such as one camera_override gives.
 * nothing reads it */
/* NOLINTNEXTLINE(clang-diagnostic-unused-const-variable) */
static const char user_function_name[] = "USR FUNCTION";

struct task_work *camera_work = NULL;
static Bool camera_collisions_on = TRUE;
/* a plane collision, in its own frame: through its middle, facing z */
static NJS_PLANE collision_plane = {0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 1.0F};
/* the levels' names, which nothing reads */
static char camera_level_labels[CAMERA_LEVELS][16] = {
    "NORMAL:", "AREA  :", "EVENT :", "COMP  :", "COLI  :", "DEBUG :",
};
/* camera_ease's countdown, which nothing sets */
static Sint32 camera_ease_left = 0;
/* the tint's colors under water, one for each corner of its quad */
static NJS_COLOR tint_colors[4] = {
#include "camera/camera/tint_colors.inc"
};

static void camera_start_at_player(void);
static void camera_set_default_mode(void);
static void camera_task_destroy(struct task *task);
static void camera_task_draw(struct task *task);
static void camera_task_step(struct task *task);
static void camera_test_collisions(void);
static void camera_start_or_end_survey(void);
static void test_at_normal(Sint16 index, struct camera_collision *collision);
static void test_at_area(Sint16 index, struct camera_collision *collision);
static void test_at_event(Sint16 index, struct camera_collision *collision);
static void camera_run_mode(struct task_work *work);
static void camera_apply_view(struct task_work *work);
static Bool sphere_holds_player(struct camera_collision *collision);
static Bool plane_crossed_by_player(struct camera_collision *collision);
static Bool block_holds_player(struct camera_collision *collision);
static void camera_pick_next_mode(void);
static void camera_enter_collision(Sint8 mode, Sint16 index);
static void camera_leave_collision(void);
static void camera_override_mode_high(Sint16 mode, Uint8 adjust);
static void camera_view_from_collision(struct camera_collision *collision);
static void camera_mode_change_step(struct task *task);
static void camera_settle_adjust(struct task_work *work,
                                 struct task_work *last);
static void camera_note_player_still(void);
static void camera_measure_move(void);
static Bool sphere_holds(const struct camera_collision *collision,
                         const NJS_POINT3 *point);
static Bool push_out_of_sphere(struct camera_collision *collision,
                               NJS_POINT3 *pos);
static Bool push_out_of_block(struct camera_collision *collision,
                              NJS_POINT3 *pos);
static void camera_draw_tint(void);
static void camera_task_draw_tint(struct task *task);

void camera_set_start_mode(Sint8 mode)
{
    /* NOLINTNEXTLINE(bugprone-signed-char-misuse) */
    camera_start_mode = mode;
}

void camera_set_restore_mode(Sint8 mode, Sint8 adjust)
{
    /* NOLINTBEGIN(bugprone-signed-char-misuse) */
    camera_restore_mode = mode;
    camera_restore_adjust = adjust;
    /* NOLINTEND(bugprone-signed-char-misuse) */
}

Angle camera_get_perspective(void)
{
    return camera_perspective;
}

void camera_set_perspective(Angle ang)
{
    int adjust;

    switch (camera_control->mode) {
    case CAMERA_MODE_FIXED:
    case CAMERA_MODE_A_FIXED:
    case CAMERA_MODE_C_FIXED:
        /* NOLINTNEXTLINE(bugprone-signed-char-misuse) */
        adjust = camera_control->adjust;
        /* @hack redundant: the label ease, which starts a block that keeps
         * angle_approach's address in R2 (0x8c01a208, 0x8c01a20e); without it
         * and its jump, R1, and 2 of 34 words differ */
        goto ease;
    ease:
        if (adjust != CAMERA_ADJUST_NONE) {
            camera_perspective = angle_approach(camera_perspective, PERSPECTIVE,
                                                PERSPECTIVE_STEP);
        } else {
            camera_perspective = PERSPECTIVE;
        }
        break;
    default:
        camera_perspective = ang;
        break;
    }
    njSetPerspective(camera_perspective);
}

Bool camera_collisions_enabled(void)
{
    return camera_collisions_on;
}

void camera_enable_collisions(void)
{
    camera_collisions_on = TRUE;
}

void camera_disable_collisions(void)
{
    camera_collisions_on = FALSE;
}

/* start the camera at player 0. it looks at a point 7.5 above his feet,
 * from 20 above that point along his up, moved 3 in x and in z */
static void camera_start_at_player(void)
{
    NJS_VECTOR up;

    camera_state->target.x = player_work[0]->pos.x;
    camera_state->target.y = player_work[0]->pos.y + 7.5F;
    camera_state->target.z = player_work[0]->pos.z;
    camera_state->ang_x = 0;
    camera_state->ang_y = 0;
    camera_state->ang_z = 0;
    camera_state->distance = 50.0F;
    up = start_above;
    njPushMatrix(&_unit_matrix_);
    TURN(njRotateZ, player_work[0]->ang.z);
    TURN(njRotateX, player_work[0]->ang.x);
    TURN(njRotateY, 0x4000 - player_work[0]->ang.y);
    njCalcVector(NULL, &up, &up);
    njPopMatrix(1);
    camera_state->pos.x = camera_state->target.x + up.x + 3.0F;
    camera_state->pos.y = camera_state->target.y + up.y;
    camera_state->pos.z = camera_state->target.z + up.z + 3.0F;
    camera_place_look_at(camera_work);
    camera_steps = 0;
    camera_restored_steps = 0;
}

/* keep what the camera runs, to go back to as a level's mode ends. slot is
 * that level. the slot keeps the collision's adjust mode, not the one
 * running */
#define SAVE_SLOT(slot)                                                        \
    do {                                                                       \
        /* NOLINTNEXTLINE(bugprone-signed-char-misuse) */                      \
        camera_control->saved_level[slot] = camera_control->level;             \
        camera_control->saved_collision[slot] = camera_control->collision;     \
        camera_control->saved_flags[slot] = camera_control->flags;             \
        camera_control->saved_mode[slot] = camera_control->mode;               \
        camera_control->saved_step[slot] = camera_control->step;               \
        /* @bug memory: with collision -1, as camera_set_mode and the          \
         * overrides leave it, the adjust mode is read from                    \
         * camera_collisions[-1] (MOV.B @(2,R0),R0 at 0x8c01b9f2 and           \
         * 0x8c01ba0c in camera_set_mode). in an act's layout that is the      \
         * entry holding the count, whose byte 2 is 0 for any count up to      \
         * CAMERA_COLLISIONS_MAX, so the slot keeps adjust mode NONE */        \
        camera_control->saved_adjust[slot] =                                   \
            camera_collisions[camera_control->collision].adjust;               \
        camera_control->saved_adjust_step[slot] =                              \
            camera_adjusts[camera_collisions[camera_control->collision]        \
                               .adjust]                                        \
                .step;                                                         \
    } while (0)

/* go back to what slot keeps */
#define RESTORE_SLOT(slot)                                                     \
    do {                                                                       \
        camera_control->collision = camera_control->saved_collision[slot];     \
        camera_control->flags = camera_control->saved_flags[slot];             \
        camera_control->mode = camera_control->saved_mode[slot];               \
        camera_control->step = camera_control->saved_step[slot];               \
        camera_control->adjust = camera_control->saved_adjust[slot];           \
        camera_control->adjust_step = camera_control->saved_adjust_step[slot]; \
        camera_control->level = camera_control->saved_level[slot];             \
    } while (0)

/* run the mode of the act's first collision at CAMERA_LEVEL_NORMAL, with
 * adjust mode NONE. it sets CAMERA_VIEW_GIVEN, so the mode's step reads
 * camera_override_view, not the collision's view */
static void camera_set_default_mode(void)
{
    camera_control->level = CAMERA_LEVEL_NORMAL;
    camera_control->collision = 0;
    camera_control->mode = camera_collisions->mode;
    camera_control->step = camera_modes[camera_control->mode].step;
    camera_control->adjust = CAMERA_ADJUST_NONE;
    camera_control->adjust_step = camera_adjusts[CAMERA_ADJUST_NONE].step;
    camera_adjust->steps = 0;
    camera_control->type = camera_modes[camera_control->mode].type;
    camera_control->changed = TRUE;
    camera_control->flags = CAMERA_VIEW_GIVEN;
}

void camera_create(void)
{
    struct task *task = task_create(TASK_WORK, 0, camera_task_step);

    camera_stage = stage_get_stage_act();
    camera_act = (Uint8)camera_stage;
    camera_stage >>= 8;
    camera_mode_count = camera_mode_table.mode_count;
    camera_modes = camera_mode_table.modes;
    camera_adjust_count = camera_mode_table.adjust_count;
    camera_adjusts = camera_mode_table.adjusts;
    camera_collision_count = (Sint32 *)camera_layouts[camera_stage][camera_act];
    camera_collisions = camera_layouts[camera_stage][camera_act] + 1;
    camera_set_default_mode();
    task->destructor = camera_task_destroy;
    task->draw = camera_task_draw;
    camera_action_state = ACTION_NONE;
    camera_start_mode = -1;
    camera_restore_mode = -1;
    /* @hack spelling: one statement, which keeps njSetPerspective a JSR ahead
     * of the return (0x8c01a526); as two, the call becomes a tail jump and
     * 12 of 73 words differ */
    camera_perspective = PERSPECTIVE, njSetPerspective(PERSPECTIVE);
}

void camera_create_advertise(void)
{
    struct task *task = task_create(TASK_WORK, 0, camera_task_step);

    camera_mode_count = camera_mode_table.mode_count;
    camera_modes = camera_mode_table.modes;
    camera_adjust_count = camera_mode_table.adjust_count;
    camera_adjusts = camera_mode_table.adjusts;
    camera_collision_count = &camera_advertise_layout.count;
    camera_collisions = camera_advertise_layout.collisions;
    camera_control->level = CAMERA_LEVEL_NORMAL;
    camera_control->collision = 0;
    camera_control->mode = CAMERA_MODE_ADVERTISE;
    camera_control->step = camera_modes[CAMERA_MODE_ADVERTISE].step;
    camera_control->adjust = CAMERA_ADJUST_NONE;
    camera_control->adjust_step = camera_adjusts[CAMERA_ADJUST_NONE].step;
    camera_adjust->steps = 0;
    camera_control->type = CAMERA_FROM_TARGET;
    camera_control->changed = TRUE;
    camera_control->flags = 0;
    task->destructor = camera_task_destroy;
    task->draw = camera_task_draw;
    /* @hack spelling: one statement, which keeps njSetPerspective a JSR ahead
     * of the return (0x8c01a62e); as two, the call becomes a tail jump, 23
     * of 76 words differ and camera.c's code runs 32 bytes short */
    camera_perspective = PERSPECTIVE, njSetPerspective(PERSPECTIVE);
}

static void camera_task_destroy(struct task *task)
{
    camera_work = NULL;
}

static void camera_task_draw(struct task *task)
{
    struct task_work *work = task->work;

    camera_apply_view(work);
}

void camera_change_act(Sint16 by)
{
    camera_act += by;
    camera_collision_count = (Sint32 *)camera_layouts[camera_stage][camera_act];
    camera_collisions = camera_layouts[camera_stage][camera_act] + 1;
    camera_set_default_mode();
    camera_steps = 0;
    camera_restored_steps = 0;
}

/* the first step takes the work, the next starts the camera at player 0,
 * and each step after runs the collisions, the mode and the view */
static void camera_task_step(struct task *task)
{
    struct task_work *work = task->work;
    int adjust;

    draw_set_material(0.0F, 0.0F, 0.0F, 0.0F);
    switch (work->mode) {
    case CAMERA_TASK_INIT:
        camera_work = work;
        work->mode = CAMERA_TASK_START;
        camera_collisions_on = TRUE;
        break;
    case CAMERA_TASK_START:
        if (camera_start_mode != -1) {
            camera_set_mode(camera_start_mode, CAMERA_ADJUST_NONE);
        }
        camera_start_at_player();
        camera_run_mode(work);
        camera_apply_view(work);
        work->mode = CAMERA_TASK_RUN;
        task->draw = camera_task_draw_tint;
        break;
    case CAMERA_TASK_RUN:
        camera_step_count++;
        if (camera_action_state == ACTION_PLAYED) {
            camera_action_state = ACTION_NONE;
            camera_restore();
        }
        camera_note_player_still();
        camera_measure_move();
        camera_test_collisions();
        if (!camera_control->changed) {
            camera_control->steps++;
        } else {
            camera_control->steps = 0;
            camera_control->changed = FALSE;
            /* @hack spelling: the adjust mode held in adjust, where the test
             * could read camera_control->adjust. the local keeps
             * camera_action_state's address in R12 and 0 in R13 (0x8c01a76a,
             * 0x8c01a778), and has the test take a copy in R4 (MOV R0,R4 at
             * 0x8c01a7ac); read in the test, 61 of 137 words differ */
            /* NOLINTNEXTLINE(bugprone-signed-char-misuse) */
            adjust = camera_control->adjust;
            /* @hack redundant: the jump to cut, where control goes anyway,
             * which puts camera_cut's address in R2 and player_get's in R3
             * (0x8c01a7b2, 0x8c01a7ba); without it, the two trade and 5 of
             * 137 words differ */
            goto cut;
        cut:
            if (adjust == CAMERA_ADJUST_NONE && camera_cut) {
                struct player *player = player_get(0);

                player->launch_steps = CUT_STEPS;
            }
        }
        camera_cut = FALSE;
        camera_run_mode(work);
        camera_apply_view(work);
        camera_draw_tint();
        camera_settle_adjust(work, camera_last);
        camera_pick_next_mode();
        if (camera_action_state == ACTION_ASKED) {
            camera_action_state = ACTION_PLAYED;
        }
        break;
    }
    camera_steps++;
    camera_restored_steps++;
}

/* give the camera the mode of a collision player 0 is in, by the rules of
 * the level the camera runs at. the collisions are tested from the last
 * down to the second, and again from the last whenever the level drops */
static void camera_test_collisions(void)
{
    struct camera_collision *collision;
    Uint16 stage;
    Sint16 i;
    Sint8 level;

    if (!camera_collisions_on) {
        return;
    }
    camera_start_or_end_survey();
    if (camera_control->mode == CAMERA_MODE_SURVEY) {
        return;
    }
    stage = stage_get_stage_act();
    if (!camera_layouts[stage >> 8][stage & 0xff]) {
        return;
    }
restart:
    collision = &camera_collisions[*camera_collision_count - 1];
    level = camera_control->level;
    for (i = *camera_collision_count - 1; i > 0; i--, collision--) {
        if (camera_modes[collision->mode].level >= camera_control->level) {
            switch (camera_control->level) {
            case CAMERA_LEVEL_NORMAL:
                if (camera_modes[collision->mode].level != CAMERA_LEVEL_COLI) {
                    test_at_normal(i, collision);
                }
                break;
            case CAMERA_LEVEL_AREA:
            case CAMERA_LEVEL_COMP:
                if (camera_modes[collision->mode].level != CAMERA_LEVEL_COLI) {
                    test_at_area(i, collision);
                }
                break;
            case CAMERA_LEVEL_EVENT:
                test_at_event(i, collision);
                break;
            }
            if (camera_control->level < level) {
                goto restart;
            }
        }
    }
}

/* start or end the survey, the pad's look round. the d-pad starts it at
 * NORMAL, AREA or COMP while camera_player_still is set, once
 * camera_restored_steps passes SURVEY_DELAY. it doesn't in the three
 * ASHLAND_I modes or the three FIXED modes. a face button, the stick, or
 * camera_player_still clear ends it, with adjust mode TIME. each call counts
 * camera_restored_steps once more */
static void camera_start_or_end_survey(void)
{
    camera_restored_steps++;
    /* @hack spelling: the end tested in two arms of one chain, which SHC builds
     * twice (0x8c01a9c4, 0x8c01aa60); tested in one, 103 of 108 words differ
     * and the code runs 64 bytes short */
    if (!(pad_records[0].on & SURVEY_START)) {
        if (camera_control->mode == CAMERA_MODE_SURVEY) {
            if ((pad_records[0].on & SURVEY_END) ||
                ((pad_records[0].x1 << 8) != 0) ||
                ((pad_records[0].y1 << 8) != 0) || !camera_player_still) {
                camera_restore_high();
                camera_set_adjust(CAMERA_ADJUST_TIME);
            }
        }
    } else if (camera_control->mode != CAMERA_MODE_SURVEY &&
               camera_control->level != CAMERA_LEVEL_EVENT &&
               camera_player_still) {
        switch (camera_control->mode) {
        case CAMERA_MODE_ASHLAND_I:
        case CAMERA_MODE_A_ASHLAND_I:
        case CAMERA_MODE_C_ASHLAND_I:
        case CAMERA_MODE_FIXED:
        case CAMERA_MODE_A_FIXED:
        case CAMERA_MODE_C_FIXED:
            break;
        default:
            if (camera_restored_steps > SURVEY_DELAY &&
                camera_control->level != CAMERA_LEVEL_COLI) {
                camera_override_mode_high(CAMERA_MODE_SURVEY,
                                          CAMERA_ADJUST_TIME);
            }
            break;
        }
    } else if (camera_control->mode == CAMERA_MODE_SURVEY) {
        if ((pad_records[0].on & SURVEY_END) ||
            ((pad_records[0].x1 << 8) != 0) ||
            ((pad_records[0].y1 << 8) != 0) || !camera_player_still) {
            camera_restore_high();
            camera_set_adjust(CAMERA_ADJUST_TIME);
        }
    }
}

/* at CAMERA_LEVEL_NORMAL: take the collision's mode once player 0 is in it,
 * or crosses its plane */
static void test_at_normal(Sint16 index, struct camera_collision *collision)
{
    Bool inside = FALSE;

    if (camera_control->collision == index || !player_work[0]) {
        return;
    }
    switch (collision->type) {
    case CAMCOL_SPHERE:
        inside = sphere_holds_player(collision);
        break;
    case CAMCOL_PLANE:
        inside = plane_crossed_by_player(collision);
        break;
    case CAMCOL_BLOCK:
        inside = block_holds_player(collision);
        break;
    }
    if (!inside) {
        return;
    }
    camera_enter_collision(collision->mode, index);
    task_create(TASK_EXTRA, 6, camera_mode_change_step);
}

/* set inside to whether player 0 is in the collision's sphere or block. a
 * plane never holds him */
/* @hack redundant: the switch in a loop of one pass, which keeps
 * test_at_area's 0 in R13, with R11 among the registers its prologue saves
 * (0x8c01ab22, 0x8c01ab0e); as the switch alone, summed over test_at_area
 * and test_at_event, 182 of 312 words differ */
#define PLAYER_INSIDE(inside, collision)                                       \
    do {                                                                       \
        switch ((collision)->type) {                                           \
        case CAMCOL_SPHERE:                                                    \
            (inside) = sphere_holds_player(collision);                         \
            break;                                                             \
        case CAMCOL_BLOCK:                                                     \
            (inside) = block_holds_player(collision);                          \
            break;                                                             \
        default:                                                               \
            (inside) = FALSE;                                                  \
        }                                                                      \
    } while (0)

/* at CAMERA_LEVEL_AREA or COMP: leave the camera's collision once player 0
 * is out of it. take a collision player 0 is in whose mode runs at a higher
 * level, or at the same level with a higher priority */
static void test_at_area(Sint16 index, struct camera_collision *collision)
{
    Bool inside;

    if (camera_control->collision == index) {
        if (!player_work[0]) {
            return;
        }
        PLAYER_INSIDE(inside, collision);
        if (!inside) {
            camera_leave_collision();
            task_create(TASK_EXTRA, 6, camera_mode_change_step);
        }
    } else if (camera_modes[collision->mode].level > camera_control->level) {
        PLAYER_INSIDE(inside, collision);
        if (inside) {
            camera_enter_collision(collision->mode, index);
            task_create(TASK_EXTRA, 6, camera_mode_change_step);
        }
    } else if (camera_modes[collision->mode].level == camera_control->level &&
               player_work[0] &&
               camera_collisions[camera_control->collision].priority <
                   collision->priority) {
        PLAYER_INSIDE(inside, collision);
        if (inside) {
            camera_leave_collision();
            camera_enter_collision(collision->mode, index);
            task_create(TASK_EXTRA, 6, camera_mode_change_step);
        }
    }
}

/* at CAMERA_LEVEL_EVENT: player 0 crossing a plane whose mode runs at COLI
 * sets the mode and adjust mode the camera goes back to at NORMAL. a
 * collision of higher priority that player 0 is in ends the override and
 * takes its own mode */
static void test_at_event(Sint16 index, struct camera_collision *collision)
{
    Bool inside = FALSE;
    int level;

    if (camera_modes[collision->mode].level == CAMERA_LEVEL_COLI) {
        if (collision->type == CAMCOL_PLANE) {
            inside = plane_crossed_by_player(collision);
        }
        if (inside) {
            level = CAMERA_LEVEL_EVENT;
            while (camera_control->saved_level[level] != CAMERA_LEVEL_NORMAL) {
                level = camera_control->saved_level[level];
            }
            if (camera_collisions[index].mode != CAMERA_MODE_COLLI) {
                camera_control->saved_mode[level] =
                    camera_collisions[index].mode;
            }
            camera_control->saved_adjust[level] =
                camera_collisions[index].adjust;
            if (camera_collisions[index].mode != CAMERA_MODE_COLLI) {
                camera_control->saved_step[level] =
                    camera_modes[collision->mode].step;
            }
            camera_control->saved_adjust_step[level] =
                camera_adjusts[camera_collisions[index].adjust].step;
            task_create(TASK_EXTRA, 6, camera_mode_change_step);
        }
    } else if (player_work[0] &&
               /* @bug memory: after an override, collision is -1,
                * and the priority is read from camera_collisions[-1], as in
                * SAVE_SLOT (MOV.B @(1,R0),R0 at 0x8c01ad66): byte 1 of an
                * act's count, 0 below 256 collisions, so player 0 inside a
                * collision of any priority above 0 ends the override */
               camera_collisions[camera_control->collision].priority <
                   collision->priority) {
        PLAYER_INSIDE(inside, collision);
        if (inside) {
            camera_restore();
            camera_enter_collision(collision->mode, index);
            task_create(TASK_EXTRA, 6, camera_mode_change_step);
        }
    }
}

/* run the mode's step, and place work as camera_control's type says. the
 * adjust mode's step follows once camera_steps passes ADJUST_DELAY, but
 * under CAMERA_ADJUST_WAITS not in the mode's first step */
static void camera_run_mode(struct task_work *work)
{
    if (!camera_control->step) {
        return;
    }
    camera_last_record = *work;
    camera_state->pos.x = work->pos.x;
    camera_state->pos.y = work->pos.y;
    camera_state->pos.z = work->pos.z;
    if (camera_control->level == CAMERA_LEVEL_DEBUG) {
        camera_step_editor(&camera_step_view);
        camera_control->step(&camera_step_view);
    } else if (camera_control->flags & CAMERA_VIEW_GIVEN) {
        camera_step_view = camera_override_view;
        camera_step_view.steps = camera_control->steps;
        camera_control->step(&camera_step_view);
    } else {
        struct camera_collision *collision =
            &camera_collisions[camera_control->collision];

        camera_view_from_collision(collision);
        camera_control->step(&camera_step_view);
    }
    switch (camera_control->type) {
    case CAMERA_AT_ANGLES:
        camera_place_at_angles(work);
        break;
    case CAMERA_LOOK_AT:
        camera_place_look_at(work);
        break;
    case CAMERA_FROM_TARGET:
        camera_place_from_target(work);
        break;
    }
    if (camera_control->steps ||
        !(camera_control->flags & CAMERA_ADJUST_WAITS)) {
        if (camera_steps > ADJUST_DELAY) {
            camera_control->adjust_step(work, camera_last, camera_adjust);
            camera_adjust->steps++;
        }
    }
}

/* set Ninja's view, then the light in it. while a camera action plays, the
 * action gives the view. otherwise the view is work's, moved by the offsets
 * camera_add_offset gave, which this zeroes */
static void camera_apply_view(struct task_work *work)
{
    if (camera_action_state != ACTION_NONE) {
        njCameraAction(camera_action, camera_action_frame);
    } else {
        njRotateViewZAbsolute(&view, work->ang.z);
        njRotateViewXAbsolute(&view, work->ang.x);
        njRotateViewYRelative(&view, work->ang.y);
        njTranslateViewAbsolute(&view, work->pos.x + camera_control->offset.x,
                                work->pos.y + camera_control->offset.y,
                                work->pos.z + camera_control->offset.z);
        njSetView(&view);
        camera_control->offset.x = 0.0F;
        camera_control->offset.y = 0.0F;
        camera_control->offset.z = 0.0F;
    }
    light_apply();
}

/* whether player 0, at TEST_HEIGHT, is in the sphere */
static Bool sphere_holds_player(struct camera_collision *collision)
{
    Float dx = player_work[0]->pos.x - collision->col_pos.x;
    Float dy = player_work[0]->pos.y - collision->col_pos.y + TEST_HEIGHT;
    Float dz = player_work[0]->pos.z - collision->col_pos.z;
    Float distance = (dx * dx) + (dy * dy) + (dz * dz);

    if (collision->col_scl.z > distance) {
        return TRUE;
    }
    return FALSE;
}

/* whether player 0, at TEST_HEIGHT, crossed the plane between the last two
 * steps, within its width and height */
static Bool plane_crossed_by_player(struct camera_collision *collision)
{
    NJS_MATRIX matrix;
    NJS_POINT3 point;
    NJS_POINT3 before;
    NJS_POINT3 after;
    NJS_POINT3 hit;
    NJS_LINE line;
    Float x = collision->col_pos.x;
    Float y = collision->col_pos.y;
    Float z = collision->col_pos.z;
    Float sx = collision->col_scl.x;
    Float sy = collision->col_scl.y;
    Float dx = x - player_work[0]->pos.x;
    /* @bug logic: dy adds 4 to the plane's height less player 0's, so
     * the range test measures from 4 below his feet, not above (FSUB at
     * 0x8c01b122, FADD at 0x8c01b12a) */
    Float dy = y - player_work[0]->pos.y + TEST_HEIGHT;
    Float dz = z - player_work[0]->pos.z;
    Float distance = (dx * dx) + (dy * dy) + (dz * dz);
    Float range = (sx * sx) + (sy * sy);
    Angle ang;

    if (distance > range) {
        return FALSE;
    }
    player_get_history(0, 1, &after, NULL);
    player_get_history(0, 2, &before, NULL);
    after.y += TEST_HEIGHT;
    before.y += TEST_HEIGHT;
    njUnitMatrix(&matrix);
    njTranslate(&matrix, x, y, z);
    TURN_MATRIX_VIA(njRotateX, &matrix, collision->col_ang_x, ang);
    TURN_MATRIX_VIA(njRotateY, &matrix, collision->col_ang_y, ang);
    njInvertMatrix(&matrix);
    njCalcPoint(&matrix, &before, &point);
    before = point;
    njCalcPoint(&matrix, &after, &point);
    after = point;
    if (before.z < 0.0F && after.z < 0.0F) {
        return FALSE;
    }
    if (before.z > 0.0F && after.z > 0.0F) {
        return FALSE;
    }
    line.px = before.x;
    line.py = before.y;
    line.pz = before.z;
    line.vx = after.x - before.x;
    line.vy = after.y - before.y;
    line.vz = after.z - before.z;
    njDistanceL2PL(&line, &collision_plane, &hit);
    if (hit.x > sx) {
        return FALSE;
    }
    if (hit.x < -sx) {
        return FALSE;
    }
    if (hit.y > sy) {
        return FALSE;
    }
    if (hit.y < -sy) {
        return FALSE;
    }
    return TRUE;
}

/* whether player 0, at TEST_HEIGHT, is in the block */
static Bool block_holds_player(struct camera_collision *collision)
{
    NJS_MATRIX matrix;
    NJS_POINT3 local;
    NJS_POINT3 point;
    Float x = collision->col_pos.x;
    Float y = collision->col_pos.y;
    Float z = collision->col_pos.z;
    Float sx = collision->col_scl.x;
    Float sy = collision->col_scl.y;
    Float sz = collision->col_scl.z;
    Float dx = x - player_work[0]->pos.x;
    /* @bug logic: dy adds 4 to the block's height less player 0's, so
     * the range test measures from 4 below his feet, not above (FSUB at
     * 0x8c01b386, FADD at 0x8c01b38c) */
    Float dy = y - player_work[0]->pos.y + TEST_HEIGHT;
    Float dz = z - player_work[0]->pos.z;
    Float distance = (dx * dx) + (dy * dy) + (dz * dz);
    Float range = (sx * sx) + (sy * sy) + (sz * sz);
    Angle ang;

    if (distance > range) {
        return FALSE;
    }
    player_get_history(0, 1, &point, NULL);
    point.y += TEST_HEIGHT;
    njUnitMatrix(&matrix);
    njTranslate(&matrix, x, y, z);
    TURN_MATRIX_VIA(njRotateX, &matrix, collision->col_ang_x, ang);
    TURN_MATRIX_VIA(njRotateY, &matrix, collision->col_ang_y, ang);
    njInvertMatrix(&matrix);
    njCalcPoint(&matrix, &point, &local);
    if (local.x > sx) {
        return FALSE;
    }
    if (local.x < -sx) {
        return FALSE;
    }
    if (local.y > sy) {
        return FALSE;
    }
    if (local.y < -sy) {
        return FALSE;
    }
    if (local.z > sz) {
        return FALSE;
    }
    if (local.z < -sz) {
        return FALSE;
    }
    return TRUE;
}

/* at CAMERA_LEVEL_NORMAL, a press of one of pad_camera_buttons moves the
 * camera on to the next pickable mode, round to mode 0 after the last */
static void camera_pick_next_mode(void)
{
    if (camera_control->level != CAMERA_LEVEL_NORMAL) {
        return;
    }
    if (!(pad_records[0].press & pad_camera_buttons)) {
        return;
    }
    /* @hack redundant: the jump to next, which loads camera_control into R2 at
     * the loop's head (MOV.L @R4,R2 at 0x8c01b48e); without it and its
     * label, R3, and 3 of 47 words differ */
    goto next;
next:
    for (;;) {
        camera_control->mode++;
        if (camera_control->mode >= camera_mode_count) {
            camera_control->mode = 0;
        }
        if (camera_modes[camera_control->mode].pickable == TRUE &&
            camera_control->level == CAMERA_LEVEL_NORMAL) {
            camera_set_mode(camera_control->mode, CAMERA_ADJUST_NONE);
            break;
        }
    }
}

void camera_place_at_angles(struct task_work *work)
{
    work->pos.x = camera_state->pos.x;
    work->pos.y = camera_state->pos.y;
    work->pos.z = camera_state->pos.z;
    work->ang.x = camera_state->ang_x;
    work->ang.y = camera_state->ang_y;
    work->ang.z = camera_state->ang_z;
}

void camera_place_look_at(struct task_work *work)
{
    Float dx;
    Float dz;

    work->pos.x = camera_state->pos.x;
    work->pos.y = camera_state->pos.y;
    work->pos.z = camera_state->pos.z;
    dx = camera_state->pos.x - camera_state->target.x;
    dz = camera_state->pos.z - camera_state->target.z;
    work->ang.y = (Angle)(atan2f(dx, dz) * ANGLE_PER_RADIAN);
    work->ang.x = (Angle)(atan2f(camera_state->target.y - camera_state->pos.y,
                                 njSqrt((dx * dx) + (dz * dz))) *
                          ANGLE_PER_RADIAN);
    work->ang.z = camera_state->ang_z;
}

void camera_place_from_target(struct task_work *work)
{
    Float across = camera_state->distance * njCos(camera_state->ang_x);

    work->pos.y = camera_state->target.y -
                  (camera_state->distance * njSin(camera_state->ang_x));
    work->pos.x =
        camera_state->target.x + (across * njSin(camera_state->ang_y));
    work->pos.z =
        camera_state->target.z + (across * njCos(camera_state->ang_y));
    work->ang.x = camera_state->ang_x;
    work->ang.y = camera_state->ang_y;
    work->ang.z = camera_state->ang_z;
}

void camera_add_offset(NJS_VECTOR *offset)
{
    camera_control->offset.x += offset->x;
    camera_control->offset.y += offset->y;
    camera_control->offset.z += offset->z;
}

/* give the camera mode, the mode of collision index, at the mode's level.
 * that level's slot keeps what the camera ran */
static void camera_enter_collision(Sint8 mode, Sint16 index)
{
    Sint8 level = camera_modes[mode].level;
    /* NOLINTBEGIN(bugprone-signed-char-misuse) */
    /* @bug memory: with collision -1, old_priority is read from
     * camera_collisions[-1], as in SAVE_SLOT (MOV.B @(1,R11),R0 at
     * 0x8c01b742): byte 1 of an act's count, 0 below 256 collisions */
    int old_priority = camera_collisions[camera_control->collision].priority;
    int new_priority = camera_collisions[index].priority;
    /* NOLINTEND(bugprone-signed-char-misuse) */

    SAVE_SLOT(level);
    camera_control->level = level;
    camera_control->collision = index;
    camera_control->mode = mode;
    camera_control->step = camera_modes[mode].step;
    if (old_priority < new_priority) {
        camera_cut = TRUE;
    }
    if (old_priority >= PRIORITY_HIGH && new_priority >= PRIORITY_HIGH) {
        camera_control->adjust = CAMERA_ADJUST_NONE;
    } else {
        camera_control->adjust = camera_collisions[index].adjust;
    }
    camera_control->adjust_step = camera_adjusts[camera_control->adjust].step;
    camera_adjust->steps = 0;
    /* a waiting adjust mode adds CAMERA_ADJUST_WAITS to the flags the
     * camera had. a CAMERA_VIEW_GIVEN from before then stays, and the step
     * reads camera_override_view, not this collision's view. any other
     * adjust mode clears the flags */
    if (camera_adjusts[camera_control->adjust].waits) {
        camera_control->flags |= CAMERA_ADJUST_WAITS;
    } else {
        camera_control->flags = 0;
    }
    camera_control->type = camera_modes[camera_control->mode].type;
    camera_control->changed = TRUE;
    task_create(TASK_EXTRA, 6, camera_mode_change_step);
}

/* go back to what the slot of the camera's level keeps */
static void camera_leave_collision(void)
{
    Sint8 level = camera_control->level;
    /* NOLINTBEGIN(bugprone-signed-char-misuse) */
    int old_priority = camera_collisions[camera_control->collision].priority;
    /* @bug memory: with the slot's collision -1, new_priority is read
     * from camera_collisions[-1], as in SAVE_SLOT (MOV.B @(1,R1),R0 at
     * 0x8c01b8ba): byte 1 of an act's count, 0 below 256 collisions */
    int new_priority =
        camera_collisions[camera_control->saved_collision[level]].priority;
    /* NOLINTEND(bugprone-signed-char-misuse) */

    camera_control->collision = camera_control->saved_collision[level];
    camera_control->flags = camera_control->saved_flags[level];
    camera_control->mode = camera_control->saved_mode[level];
    camera_control->step = camera_control->saved_step[level];
    camera_control->level = camera_control->saved_level[level];
    if (old_priority >= PRIORITY_HIGH && new_priority >= PRIORITY_HIGH) {
        camera_control->adjust = CAMERA_ADJUST_NONE;
    } else {
        camera_control->adjust = camera_control->saved_adjust[level];
    }
    if (old_priority < new_priority) {
        camera_cut = TRUE;
    }
    if (old_priority >= PRIORITY_HIGH && new_priority >= PRIORITY_HIGH) {
        camera_control->adjust_step = camera_adjusts[CAMERA_ADJUST_NONE].step;
    } else {
        camera_control->adjust_step = camera_control->saved_adjust_step[level];
    }
    camera_control->type = camera_modes[camera_control->mode].type;
    camera_control->changed = TRUE;
    camera_control->steps = 0;
    camera_adjust->steps = 0;
    task_create(TASK_EXTRA, 6, camera_mode_change_step);
}

void camera_set_mode(Sint16 mode, Uint8 adjust)
{
    if (camera_control->level != CAMERA_LEVEL_NORMAL) {
        return;
    }
    SAVE_SLOT(CAMERA_LEVEL_NORMAL);
    camera_control->collision = -1;
    camera_control->level = CAMERA_LEVEL_NORMAL;
    camera_control->mode = mode;
    camera_control->step = camera_modes[mode].step;
    camera_control->adjust = adjust;
    camera_control->adjust_step = camera_adjusts[adjust].step;
    camera_adjust->steps = 0;
    if (camera_adjusts[camera_control->adjust].waits) {
        camera_control->flags |= CAMERA_ADJUST_WAITS;
    }
    camera_control->type = camera_modes[camera_control->mode].type;
    camera_control->changed = TRUE;
    task_create(TASK_EXTRA, 6, camera_mode_change_step);
}

void camera_override_mode(Sint16 mode, Uint8 adjust)
{
    if (camera_control->level <= CAMERA_LEVEL_EVENT) {
        if (camera_control->level != CAMERA_LEVEL_EVENT) {
            SAVE_SLOT(CAMERA_LEVEL_EVENT);
            camera_control->level = CAMERA_LEVEL_EVENT;
        }
        camera_control->collision = -1;
        camera_control->mode = mode;
        camera_control->step = camera_modes[mode].step;
        camera_control->adjust = adjust;
        camera_control->adjust_step = camera_adjusts[adjust].step;
        camera_adjust->steps = 0;
        camera_control->flags = CAMERA_VIEW_GIVEN;
        if (camera_adjusts[camera_control->adjust].waits) {
            camera_control->flags |= CAMERA_ADJUST_WAITS;
        }
        camera_control->type = camera_modes[camera_control->mode].type;
        camera_control->changed = TRUE;
        task_create(TASK_EXTRA, 6, camera_mode_change_step);
    }
}

void camera_override(void (*step)(struct camera_view *view), Uint8 adjust,
                     Sint8 type)
{
    if (camera_control->level <= CAMERA_LEVEL_EVENT) {
        if (camera_control->level != CAMERA_LEVEL_EVENT) {
            SAVE_SLOT(CAMERA_LEVEL_EVENT);
            camera_control->level = CAMERA_LEVEL_EVENT;
        }
        camera_control->collision = -1;
        camera_control->mode = -1;
        camera_control->step = step;
        camera_control->adjust = adjust;
        camera_control->adjust_step =
            camera_adjusts[camera_control->adjust].step;
        camera_adjust->steps = 0;
        camera_control->flags = CAMERA_VIEW_GIVEN;
        if (camera_adjusts[camera_control->adjust].waits) {
            camera_control->flags |= CAMERA_ADJUST_WAITS;
        }
        camera_control->type = type;
        camera_control->changed = TRUE;
        task_create(TASK_EXTRA, 6, camera_mode_change_step);
    }
}

void camera_restore(void)
{
    if (camera_control->level == CAMERA_LEVEL_EVENT) {
        RESTORE_SLOT(CAMERA_LEVEL_EVENT);
        camera_control->type = camera_modes[camera_control->mode].type;
        camera_control->changed = TRUE;
        camera_control->steps = 0;
        camera_adjust->steps = 0;
        task_create(TASK_EXTRA, 6, camera_mode_change_step);
        if (camera_control->level == CAMERA_LEVEL_NORMAL &&
            camera_restore_mode != -1) {
            camera_set_mode(camera_restore_mode, camera_restore_adjust);
        }
    }
    camera_restored_steps = 0;
}

void camera_override_high(void (*step)(struct camera_view *view), Uint8 adjust,
                          Sint8 type)
{
    if (camera_control->level <= CAMERA_LEVEL_COLI) {
        if (camera_control->level != CAMERA_LEVEL_COLI) {
            SAVE_SLOT(CAMERA_LEVEL_COLI);
            camera_control->level = CAMERA_LEVEL_COLI;
        }
        camera_control->collision = -1;
        camera_control->mode = -1;
        camera_control->step = step;
        camera_control->adjust = adjust;
        camera_control->adjust_step =
            camera_adjusts[camera_control->adjust].step;
        camera_adjust->steps = 0;
        camera_control->flags = CAMERA_VIEW_GIVEN;
        if (camera_adjusts[camera_control->adjust].waits) {
            camera_control->flags |= CAMERA_ADJUST_WAITS;
        }
        camera_control->type = type;
        camera_control->changed = TRUE;
        task_create(TASK_EXTRA, 6, camera_mode_change_step);
    }
}

static void camera_override_mode_high(Sint16 mode, Uint8 adjust)
{
    if (camera_control->level <= CAMERA_LEVEL_COLI) {
        if (camera_control->level != CAMERA_LEVEL_COLI) {
            SAVE_SLOT(CAMERA_LEVEL_COLI);
            camera_control->level = CAMERA_LEVEL_COLI;
        }
        camera_control->collision = -1;
        camera_control->mode = mode;
        camera_control->step = camera_modes[mode].step;
        camera_control->adjust = adjust;
        camera_control->adjust_step = camera_adjusts[adjust].step;
        camera_adjust->steps = 0;
        camera_control->flags = CAMERA_VIEW_GIVEN;
        if (camera_adjusts[camera_control->adjust].waits) {
            camera_control->flags |= CAMERA_ADJUST_WAITS;
        }
        camera_control->type = camera_modes[camera_control->mode].type;
        camera_control->changed = TRUE;
        task_create(TASK_EXTRA, 6, camera_mode_change_step);
    }
}

void camera_restore_high(void)
{
    if (camera_control->level != CAMERA_LEVEL_COLI) {
        return;
    }
    RESTORE_SLOT(CAMERA_LEVEL_COLI);
    camera_control->type = camera_modes[camera_control->mode].type;
    camera_control->changed = TRUE;
    camera_control->steps = 0;
    camera_adjust->steps = 0;
    task_create(TASK_EXTRA, 6, camera_mode_change_step);
    if (camera_control->level == CAMERA_LEVEL_NORMAL &&
        camera_restore_mode != -1) {
        camera_set_mode(camera_restore_mode, camera_restore_adjust);
    }
}

void camera_take(struct task_work *work, void (*step)(struct camera_view *view))
{
    if (camera_control->level != CAMERA_LEVEL_DEBUG) {
        /* NOLINTNEXTLINE(bugprone-signed-char-misuse) */
        camera_control->saved_level[CAMERA_LEVEL_DEBUG] = camera_control->level;
        camera_control->saved_collision[CAMERA_LEVEL_DEBUG] =
            camera_control->collision;
        camera_control->saved_flags[CAMERA_LEVEL_DEBUG] = camera_control->flags;
        camera_control->saved_mode[CAMERA_LEVEL_DEBUG] = camera_control->mode;
        camera_control->saved_step[CAMERA_LEVEL_DEBUG] = camera_control->step;
        camera_control->saved_adjust[CAMERA_LEVEL_DEBUG] =
            camera_control->adjust;
        camera_control->saved_adjust_step[CAMERA_LEVEL_DEBUG] =
            camera_control->adjust_step;
        camera_control->saved_type[CAMERA_LEVEL_DEBUG] = camera_control->type;
        /* NOLINTNEXTLINE(bugprone-signed-char-misuse) */
        camera_control->unk_86[CAMERA_LEVEL_DEBUG] = camera_control->level;
        camera_control->level = CAMERA_LEVEL_DEBUG;
    }
    camera_take_work = work;
    camera_control->collision = -1;
    camera_control->mode = -1;
    camera_control->step = step;
    camera_control->adjust = CAMERA_ADJUST_NONE;
    camera_control->adjust_step = camera_adjusts[CAMERA_ADJUST_NONE].step;
    camera_control->flags = CAMERA_ADJUST_WAITS | CAMERA_VIEW_GIVEN;
    camera_control->type = CAMERA_BY_STEP;
    camera_control->changed = TRUE;
    task_create(TASK_EXTRA, 6, camera_mode_change_step);
}

void camera_release(void)
{
    if (camera_control->level == CAMERA_LEVEL_DEBUG) {
        camera_set_default_mode();
    }
}

/* set camera_step_view to the view the collision gives its mode's step */
static void camera_view_from_collision(struct camera_collision *collision)
{
    camera_step_view.pos.x = collision->cam_pos.x;
    camera_step_view.pos.y = collision->cam_pos.y;
    camera_step_view.pos.z = collision->cam_pos.z;
    camera_step_view.target.x = collision->dir_pos.x;
    camera_step_view.target.y = collision->dir_pos.y;
    camera_step_view.target.z = collision->dir_pos.z;
    camera_step_view.ang_x = collision->cam_ang_x;
    camera_step_view.ang_y = collision->cam_ang_y;
    camera_step_view.distance = collision->distance;
    camera_step_view.steps = camera_control->steps;
}

/* the task made at each change of mode, which only ends itself */
static void camera_mode_change_step(struct task *task)
{
    task_kill(task);
}

Bool camera_at_event_level(void)
{
    if (camera_control->level == CAMERA_LEVEL_EVENT) {
        return TRUE;
    }
    return FALSE;
}

Bool camera_at_comp_level(void)
{
    if (camera_control->level == CAMERA_LEVEL_COMP) {
        return TRUE;
    }
    return FALSE;
}

Uint32 camera_get_steps(void)
{
    return camera_steps;
}

void camera_reset_steps(void)
{
    camera_steps = 0;
}

void camera_set_adjust(int adjust)
{
    camera_control->adjust = adjust;
    camera_control->adjust_step = camera_adjusts[camera_control->adjust].step;
    camera_adjust->steps = 0;
}

int camera_get_adjust(void)
{
    return camera_control->adjust;
}

int camera_get_mode(void)
{
    return camera_control->mode;
}

/* if the camera is still from last to work, adjust mode NORMAL becomes
 * SLOW. NONE, NORMAL_S, SLOW and SLOW_S become NONE. camera_set_adjust
 * zeroes the adjust mode's steps, so a still camera at NONE zeroes them
 * each step */
static void camera_settle_adjust(struct task_work *work, struct task_work *last)
{
    Float dx = work->pos.x - last->pos.x;
    Float dy = work->pos.y - last->pos.y;
    Float dz = work->pos.z - last->pos.z;
    Float distance = (dx * dx) + (dy * dy) + (dz * dz);

    if (distance < STILL_MOVE * STILL_MOVE &&
        angle_distance(work->ang.x, last->ang.x) < STILL_TURN &&
        angle_distance(work->ang.y, last->ang.y) < STILL_TURN &&
        angle_distance(work->ang.z, last->ang.z) < STILL_TURN) {
        switch (camera_control->adjust) {
        case CAMERA_ADJUST_NORMAL:
            camera_set_adjust(CAMERA_ADJUST_SLOW);
            break;
        case CAMERA_ADJUST_NONE:
        case CAMERA_ADJUST_NORMAL_S:
        case CAMERA_ADJUST_SLOW:
        case CAMERA_ADJUST_SLOW_S:
            camera_set_adjust(CAMERA_ADJUST_NONE);
            break;
        }
    }
}

static void camera_note_player_still(void)
{
    njSubVector(&camera_player_last, &player_work[0]->pos);
    if (njScalor(&camera_player_last) < STILL_MOVE &&
        (player_work[0]->flags & PLAYER_ON_LAND)) {
        camera_player_still = TRUE;
    } else {
        camera_player_still = FALSE;
    }
    camera_player_last = player_work[0]->pos;
}

static void camera_measure_move(void)
{
    camera_moved.x = camera_work->pos.x - camera_last_pos.x;
    camera_moved.y = camera_work->pos.y - camera_last_pos.y;
    camera_moved.z = camera_work->pos.z - camera_last_pos.z;
    camera_last_pos = camera_work->pos;
}

Bool camera_adjust_is_c(void)
{
    /* NOLINTNEXTLINE(bugprone-signed-char-misuse) */
    int adjust = camera_control->adjust;

    switch (adjust) {
    case CAMERA_ADJUST_TIME:
    case CAMERA_ADJUST_THREE1C:
    case CAMERA_ADJUST_THREE2C:
    case CAMERA_ADJUST_THREE3C:
    case CAMERA_ADJUST_THREE4C:
    case CAMERA_ADJUST_THREE5C:
    case CAMERA_ADJUST_RELATIVE1C:
    case CAMERA_ADJUST_RELATIVE2C:
    case CAMERA_ADJUST_RELATIVE3C:
    case CAMERA_ADJUST_RELATIVE4C:
    case CAMERA_ADJUST_RELATIVE5C:
    case CAMERA_ADJUST_RELATIVE6C:
        return TRUE;
    default:
        return FALSE;
    }
}

void camera_set_type(int type)
{
    camera_control->type = type;
}

Bool camera_play_action(NJS_CACTION *action, Float frame)
{
    switch (camera_action_state) {
    case ACTION_NONE:
        camera_override(camera_step_none, CAMERA_ADJUST_NONE, CAMERA_BY_STEP);
        /* fall through */
    case ACTION_PLAYED:
        camera_action_state = ACTION_ASKED;
        camera_action = action;
        camera_action_frame = frame;
        return FALSE;
    }
    return TRUE;
}

int camera_push_out(NJS_POINT3 *pos)
{
    struct camera_collision *collision;
    Uint16 stage;
    int count = 0;
    Sint16 i;

    stage = stage_get_stage_act();
    if (!camera_layouts[stage >> 8][stage & 0xff]) {
        return 0;
    }
    collision = &camera_collisions[*camera_collision_count - 1];
    for (i = *camera_collision_count - 1; i > 0; i--, collision--) {
        if (camera_modes[collision->mode].level == CAMERA_LEVEL_COLI) {
            switch (collision->type) {
            case CAMCOL_SPHERE:
                count += push_out_of_sphere(collision, pos);
                break;
            case CAMCOL_BLOCK:
                count += push_out_of_block(collision, pos);
                break;
            }
        }
    }
    return count;
}

static Bool sphere_holds(const struct camera_collision *collision,
                         const NJS_POINT3 *point)
{
    Float dx = point->x - collision->col_pos.x;
    Float dy = point->y - collision->col_pos.y;
    Float dz = point->z - collision->col_pos.z;
    Float distance = (dx * dx) + (dy * dy) + (dz * dz);

    if (collision->col_scl.z > distance) {
        return TRUE;
    }
    return FALSE;
}

/* push pos out to the sphere's surface, away from its middle, if the
 * sphere holds it. TRUE if it did */
static Bool push_out_of_sphere(struct camera_collision *collision,
                               NJS_POINT3 *pos)
{
    NJS_VECTOR away;

    if (sphere_holds(collision, pos) == TRUE) {
        away.x = pos->x - collision->col_pos.x;
        away.y = pos->y - collision->col_pos.y;
        away.z = pos->z - collision->col_pos.z;
        njUnitVector(&away);
        pos->x = collision->col_pos.x + (away.x * collision->col_scl.x);
        pos->y = collision->col_pos.y + (away.y * collision->col_scl.x);
        pos->z = collision->col_pos.z + (away.z * collision->col_scl.x);
        return TRUE;
    }
    return FALSE;
}

/* push pos out through the block's nearest face, if the block holds it.
 * TRUE if it did */
static Bool push_out_of_block(struct camera_collision *collision,
                              NJS_POINT3 *pos)
{
    NJS_MATRIX matrix;
    NJS_POINT3 local;
    Float depth[3];
    Angle ang;
    int axis;

    njUnitMatrix(&matrix);
    njTranslate(&matrix, collision->col_pos.x, collision->col_pos.y,
                collision->col_pos.z);
    TURN_MATRIX_VIA(njRotateX, &matrix, collision->col_ang_x, ang);
    TURN_MATRIX_VIA(njRotateY, &matrix, collision->col_ang_y, ang);
    njInvertMatrix(&matrix);
    njCalcPoint(&matrix, pos, &local);
    /* @hack spelling: each depth set inside its test, which stores it through
     * R4 (FMOV.S FR3,@R4 at 0x8c01c70a); set ahead of the tests, depth's
     * address takes R13 and 63 of 186 words differ */
    if ((depth[0] = njAbs(local.x)) > collision->col_scl.x) {
        return FALSE;
    }
    if ((depth[1] = njAbs(local.y)) > collision->col_scl.y) {
        return FALSE;
    }
    if ((depth[2] = njAbs(local.z)) > collision->col_scl.z) {
        return FALSE;
    }
    depth[0] = collision->col_scl.x - depth[0];
    depth[1] = collision->col_scl.y - depth[1];
    depth[2] = collision->col_scl.z - depth[2];
    axis = 0;
    if (depth[1] < depth[0]) {
        axis = 1;
        if (depth[2] < depth[1]) {
            axis = 2;
        }
    } else if (depth[2] < depth[0]) {
        axis = 2;
    }
    switch (axis) {
    case 0:
        if (local.x > 0.0F) {
            local.x = collision->col_scl.x;
        } else {
            local.x = -collision->col_scl.x;
        }
        break;
    case 1:
        if (local.y > 0.0F) {
            local.y = collision->col_scl.y;
        } else {
            local.y = -collision->col_scl.y;
        }
        break;
    case 2:
        if (local.z > 0.0F) {
            local.z = collision->col_scl.z;
        } else {
            local.z = -collision->col_scl.z;
        }
        break;
    }
    njInvertMatrix(&matrix);
    njCalcPoint(&matrix, &local, pos);
    return TRUE;
}

void camera_ease(NJS_POINT3 *from, NJS_POINT3 *pos)
{
    if (!camera_ease_left) {
        return;
    }
    pos->x =
        from->x + ((pos->x - from->x) * (Float)(EASE_STEPS - camera_ease_left) *
                   (1.0F / EASE_STEPS));
    pos->y =
        from->y + ((pos->y - from->y) * (Float)(EASE_STEPS - camera_ease_left) *
                   (1.0F / EASE_STEPS));
    pos->z =
        from->z + ((pos->z - from->z) * (Float)(EASE_STEPS - camera_ease_left) *
                   (1.0F / EASE_STEPS));
    camera_ease_left--;
}

/* tint the screen while the eye point, 2 in front of the camera, is under
 * water. a step that finds camera_in_dry_box set draws no tint, and clears
 * the flag. the facing plane runs through the eye point, square to the
 * camera's view. the water line is where it meets the nearest water surface
 * above the eye point. the tint covers the screen from the row that line
 * projects to, or all of it where that row is off the screen */
static void camera_draw_tint(void)
{
    /* @hack spelling: the locals in this order, which puts facing 100 to 120
     * bytes above R15, in reach of MOV #imm,R0 (0x8c01c948 to 0x8c01c96c);
     * declared as first used, 251 of 486 words differ and the code runs 32
     * bytes short */
    struct height_query query;
    NJS_POINT3 above;
    NJS_POINT3 below;
    NJS_POINT3 eye;
    NJS_POINT2 screen;
    union plane_view facing;
    NJS_PLANE above_plane;
    NJS_PLANE below_plane;
    struct task_work *work = camera_work;
    struct world_object *objects;
    NJS_OBJECT *object;
    Uint16 i;
    Sint16 y;
    Bool dry;

    dry = camera_in_dry_box;
    camera_in_dry_box = FALSE;
    if (dry == TRUE) {
        return;
    }
    eye.x = 0.0F;
    eye.y = 0.0F;
    eye.z = -2.0F;
    njPushMatrix(&_unit_matrix_);
    njTranslateV(NULL, &work->pos);
    TURN(njRotateY, work->ang.y);
    TURN(njRotateX, work->ang.x);
    njCalcPoint(NULL, &eye, &eye);
    facing.plane.px = eye.x;
    facing.plane.py = eye.y;
    facing.plane.pz = eye.z;
    facing.plane.vx = 0.0F;
    facing.plane.vy = 0.0F;
    facing.plane.vz = -1.0F;
    njCalcVector(NULL, &facing.split.normal, &facing.split.normal);
    njPopMatrix(1);

    query.pos = eye;
    objects = world_objects;
    world_list_near(eye.x, eye.y, eye.z, 200.0F);
    above.y = NO_WATER;
    below.y = -NO_WATER;
    for (i = 0; i < world_count; i++) {
        object = objects[i].object;
        if (objects[i].owner) {
            NJS_VECTOR away;
            Float scale;
            Float distance;

            away.x = object->pos[0] - eye.x;
            away.y = 0.0F;
            away.z = object->pos[2] - eye.z;
            distance = njScalor(&away);
            scale = object->scl[0];
            if (object->scl[1] > scale) {
                scale = object->scl[1];
            }
            if (object->scl[2] > scale) {
                scale = object->scl[2];
            }
            if (distance > scale * object->model->r) {
                continue;
            }
        }
        if (objects[i].flags & WORLD_LIQUID) {
            if (polygon_find_model_heights(&query, objects[i].object)) {
                if (query.above.found == TRUE &&
                    query.pos.y < query.above.level &&
                    query.above.level < above.y) {
                    above.y = query.above.level;
                } else if (query.below.found == TRUE &&
                           query.pos.y > query.below.level &&
                           query.below.level < below.y) {
                    /* @bug logic: below.y starts at -NO_WATER, so FCMP/GT
                     * at 0x8c01cb1c passes only a level under -NO_WATER, and
                     * no water below the point is kept */
                    below.y = query.below.level;
                }
            }
        }
    }
    if (above.y == NO_WATER && below.y == -NO_WATER) {
        return;
    }

    /* below_plane lies at above.y, as above_plane does, so below takes the
     * point above takes. where above projects off the screen, the code below
     * projects that same point again. where the planes don't meet, each else
     * sets above.y to -NO_WATER */
    above_plane.px = eye.x;
    above_plane.py = above.y;
    above_plane.pz = eye.z;
    above_plane.vx = 0.0F;
    above_plane.vy = 1.0F;
    above_plane.vz = 0.0F;
    below_plane.px = eye.x;
    below_plane.py = above.y;
    below_plane.pz = eye.z;
    below_plane.vx = 0.0F;
    below_plane.vy = 1.0F;
    below_plane.vz = 0.0F;
    {
        NJS_LINE line;

        /* @hack type: the union's own address as the NJS_PLANE, which SHC
         * forms from R15 at each use; as &facing.plane here too, it keeps
         * the address in a saved register, the function runs 4 bytes long
         * and 274 of 486 words differ */
        if (njDistancePL2PL((NJS_PLANE *)&facing, &above_plane, &line) ==
            0.0F) {
            above.x = line.px;
            above.y = line.py;
            above.z = line.pz;
        } else {
            above.y = -NO_WATER;
        }
        if (njDistancePL2PL(&facing.plane, &below_plane, &line) == 0.0F) {
            below.x = line.px;
            below.y = line.py;
            below.z = line.pz;
        } else {
            above.y = -NO_WATER;
        }
    }
    njPushMatrix(NULL);
    njProjectScreen(NULL, &above, &screen);
    njPopMatrix(1);
    if (screen.y < 0.0F || screen.y > SCREEN_HEIGHT) {
        njPushMatrix(NULL);
        njProjectScreen(NULL, &below, &screen);
        njPopMatrix(1);
    }

    {
        NJS_POINT2COL polygon;

        {
            NJS_POINT2 line[2];
            NJS_COLOR colors[2];

            /* draw a line along row 490, below the screen's 480 rows and
             * out of sight */
            polygon.p = line;
            polygon.col = colors;
            polygon.tex = NULL;
            polygon.num = 2;
            polygon.p[0].x = 0.0F;
            polygon.p[1].x = SCREEN_WIDTH;
            polygon.p[0].y = 490.0F;
            polygon.p[1].y = 490.0F;
            polygon.col[0].color = 0xff88;
            polygon.col[1].color = 0xff88;
            njDrawLine2D(&polygon, 2, -2.0F, NJD_TRANSPARENT);
        }
        {
            NJS_POINT2 quad[4];

            y = (Sint16)screen.y;
            if (y < 0 || y > 480) {
                y = 0;
            }
            polygon.p = quad;
            polygon.col = tint_colors;
            polygon.tex = NULL;
            polygon.num = 4;
            quad[0].x = 0.0F;
            quad[0].y = (Float)y;
            quad[1].x = 0.0F;
            quad[1].y = SCREEN_HEIGHT;
            quad[2].x = SCREEN_WIDTH;
            quad[2].y = SCREEN_HEIGHT;
            quad[3].x = SCREEN_WIDTH;
            quad[3].y = (Float)y;
            njColorBlendingMode(NJD_SOURCE_COLOR, NJD_COLOR_BLENDING_SRCALPHA);
            njColorBlendingMode(NJD_DESTINATION_COLOR, NJD_COLOR_BLENDING_ONE);
            draw_set_material(0.0F, 0.0F, 0.0F, 0.0F);
            njDrawPolygon2D(&polygon, 4, 0.0F, NJD_FILL | NJD_TRANSPARENT);
        }
    }
}

Bool camera_sweep_hits(NJS_POINT3 *pos, NJS_VECTOR *speed, struct angles *ang,
                       Float radius)
{
    return world_sweep_ball_hit(pos, speed, ang, WORLD_LIQUID, radius);
}

static void camera_task_draw_tint(struct task *task)
{
    camera_draw_tint();
}
