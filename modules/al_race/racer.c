/* a Chao in the race, one to a lane: what the race keeps of it, and the
 * commands it gives it */
#include <ninja.h>
#include <sg_xpt.h>

#include "al_main/chao.h"
#include "al_race/ball.h"
#include "al_race/bugle.h"
#include "al_race/coconut_palm.h"
#include "al_race/course.h"
#include "al_race/game.h"
#include "al_race/item.h"
#include "al_race/palm_coconut.h"
#include "al_race/race_chao.h"
#include "al_race/race_path.h"
#include "al_race/racer.h"
#include "al_race/scoop.h"
#include "al_race/spray.h"
#include "al_race/watering_can.h"
#include "camera/camera.h"
#include "chao/chao.h"
#include "path/path.h"
#include "system/task.h"
#include "system/util.h"

/* @hack spelling: racer.c builds with -noinline, which keeps each call a
 * call. the image still expands racer_has_chao wherever racer.c calls it, in
 * racer_step (TST R13,R3 and TST R4,R4 at 0x0cb86ce8 and 0x0cb86d22) and
 * racer_face_target (TST #4,R0 and TST R4,R4 at 0x0cb87524 and 0x0cb8752e).
 * without the pragma, each call stays a call, and summed over the two, 211 of
 * 237 words differ */
#pragma inline(racer_has_chao)

/* a debug label's row, in bytes */
#define LABEL_SIZE 20
/* a Chao closer than this to the camera is hidden */
#define HIDE_RANGE 5.0F
/* a Chao heading less than this off its target has turned */
#define FACING_ANGLE NJM_DEG_ANG(10)
/* how far the weave phase advances each step a Chao follows its path */
#define WEAVE_STEP NJM_DEG_ANG(1)
/* racer_turn_to's timeout toward an item, which counts only for a Chao
 * without a task */
#define TURN_TIMEOUT 120
/* the count of effect_timer at which a dance or a speed boost ends */
#define EFFECT_STEPS 300
/* racer_do_command's result outside the active states */
#define PROGRESS_INACTIVE 3

static void racer_reset(struct racer *racer, int lane);
static void racer_step_effect(struct racer *racer);
static void racer_aim(struct racer *racer, NJS_POINT3 *target);
static void racer_update_shown(struct racer *racer);
static void racer_update_segment(struct racer *racer, int kind);
static void racer_update_path(struct racer *racer, struct race_path *path);
static void racer_follow_path(struct racer *racer, struct race_path *path);
static void racer_idle(struct racer *racer);
static void racer_clear_item(struct racer_reaction *reaction);
static void racer_take_up_item(struct racer *racer);
static void racer_swing(struct racer *racer);
static void racer_eat(struct racer *racer);
static void racer_push(struct racer *racer);
static void racer_play(struct racer *racer);
static void racer_go_to_coconut(struct racer *racer);
static void racer_go_to_lane_point(struct racer *racer,
                                   int (*get_point)(NJS_POINT3 *point,
                                                    int lane));
static void racer_hold_item(struct racer *racer, struct task_work *work,
                            Sint8 kind);

static const NJS_POINT3 racer_entry_places[RACE_LANES] = {
#include "al_race/racer/racer_entry_places.inc"
};

static const char racer_mode_labels[15][LABEL_SIZE] = {
    "WAIT", "FLOAT", "RUN",  "WALK",  "KOKE", "FLY",  "SWIM", "CLIMB",
    "PUSH", "GOAL",  "PLAY", "SWING", "EAT",  "REST", "DROWN"};

/* @bug memory: COMMAND_COMMENDATION fills its 20-byte row, at
 * 0x0cb9a0cc, with no null, so racer_print_motion prints COMMAND_DEBUG after
 * it */
/* NOLINTBEGIN(clang-diagnostic-unterminated-string-initialization) */
static const char racer_command_labels[COMMAND_DEBUG + 1][LABEL_SIZE] = {
    "COMMAND_NONE",        "COMMAND_WINDOW",       "COMMAND_BIRTH",
    "COMMAND_WAIT",        "COMMAND_RACE",         "COMMAND_WALK",
    "COMMAND_REST",        "COMMAND_HAPPY",        "COMMAND_SWING",
    "COMMAND_PUSH",        "COMMAND_EAT",          "COMMAND_PLAY_RAPPA",
    "COMMAND_PLAY_JYOURO", "COMMAND_PLAY_SCOOP",   "COMMAND_RETURN",
    "COMMAND_DANCE",       "COMMAND_COMMENDATION", "COMMAND_DEBUG"};
/* NOLINTEND(clang-diagnostic-unterminated-string-initialization) */

struct racer *racer_init(struct racer *racer, int lane)
{
    racer->flags |= RACER_INITIALIZED;
    if (racer->flags & RACER_ATTACHED) {
        racer->flags &= ~RACER_ATTACHED;
        racer->flags |= RACER_HAS_CHAO;
    } else {
        racer->chao = 0;
        racer->work = 0;
    }
    racer_reset(racer, lane);
    return racer;
}

void racer_attach(struct racer *racer, struct task_work *work,
                  struct alife_chao *chao)
{
    racer->flags |= RACER_ATTACHED;
    racer->chao = chao;
    racer->work = work;
    if (racer->flags & RACER_INITIALIZED) {
        racer->flags |= RACER_HAS_CHAO;
    }
}

void racer_detach(struct racer *racer)
{
    racer->flags &= ~RACER_INITIALIZED;
    racer->flags &= ~RACER_ATTACHED;
    racer->flags &= ~RACER_HAS_CHAO;
    racer->chao = 0;
    racer->work = 0;
}

void racer_destroy_chao(struct racer *racer)
{
    if (racer->chao) {
        task_destroy(racer->chao->task);
        racer->flags = 0;
    }
}

int racer_has_chao(struct racer *racer)
{
    if (racer->flags & RACER_HAS_CHAO) {
        return 1;
    }
    return 0;
}

struct alife_chao *racer_chao(struct racer *racer)
{
    if (racer->flags & RACER_HAS_CHAO) {
        return racer->chao;
    }
    return 0;
}

struct task_work *racer_work(struct racer *racer)
{
    if (racer->flags & RACER_HAS_CHAO) {
        return racer->work;
    }
    return 0;
}

/* reset the racer for a race in lane, leaving its Chao where it stands. its
 * course section, distance and rise go to 0, its weave phase to a random
 * one, and its command and effect to none. path_index goes to 2, state to
 * RACER_TO_WAIT and route to ROUTE_START. it takes the idle behavior, no item
 * and a new splash spray. its flags keep only RACER_INITIALIZED,
 * RACER_ATTACHED, RACER_HAS_CHAO, RACER_HOLD_PATH and RACER_SPRAYED */
static void racer_reset(struct racer *racer, int lane)
{
    racer->lane = lane;
    racer->segment = 0;
    racer->distance = 0.0F;
    racer->rise = 0.0F;
    racer->phase = RANDOM_ANGLE();
    racer->command = COMMAND_NONE;
    racer->path_index = 2;
    racer->effect_timer = 0;
    racer->effect = EFFECT_NONE;
    racer->state = RACER_TO_WAIT;
    racer->route = ROUTE_START;
    /* @hack spelling: RACER_AIMED kept by the first mask and cleared by a
     * second, which the image ANDs at 0x0cb86aaa and 0x0cb86ab2 with a store
     * between; as one mask, 38 of 61 words differ */
    racer->flags &= RACER_INITIALIZED | RACER_ATTACHED | RACER_HAS_CHAO |
                    RACER_AIMED | RACER_HOLD_PATH | RACER_SPRAYED;
    racer->flags &= ~RACER_AIMED;
    racer_clear_behavior(&racer->behavior);
    racer_clear_item(&racer->reaction);
    spray_init_splash(&racer->spray);
}

void racer_step(struct racer *racer, struct race_path *path, int kind)
{
    racer_update_shown(racer);
    switch (racer->state) {
    case RACER_TO_WAIT:
        racer->command = COMMAND_WAIT;
        racer->state = RACER_WAITING;
        break;
    case RACER_WAITING:
        break;
    case RACER_TO_REST:
        racer->command = COMMAND_REST;
        racer->state = RACER_RESTING;
        break;
    case RACER_RESTING:
        break;
    case RACER_TO_WALK:
        racer->command = COMMAND_WALK;
        racer->state = RACER_WALKING;
        break;
    case RACER_WALKING:
        racer->command = COMMAND_WALK;
        break;
    case RACER_TO_RACE:
        racer->command = COMMAND_RACE;
        racer->state = RACER_RACING;
        break;
    case RACER_RACING:
        break;
    case RACER_FINISHING:
        break;
    case RACER_TO_GOAL:
        racer->command = COMMAND_HAPPY;
        racer->state = RACER_AT_GOAL;
        break;
    case RACER_AT_GOAL:
        break;
    case RACER_TO_PODIUM:
        racer->command = COMMAND_COMMENDATION;
        racer->state = RACER_ON_PODIUM;
        racer->work->pos = race_course.targets[racer->place + 8];
        break;
    case RACER_ON_PODIUM:
        racer->command = COMMAND_COMMENDATION;
        break;
    case RACER_TO_RESTART:
        racer->state = RACER_RESTARTING;
    case RACER_RESTARTING:
        if (racer->flags & RACER_HAS_CHAO) {
            NJS_POINT3 *start = race_course.starts[kind];
            racer->work->pos = start[racer->lane];
            racer_update_path(racer, path);
            racer_reset(racer, racer->lane);
            if (racer->chao->task) {
                race_chao_reset(racer->chao->task);
            }
        }
        break;
    }
    if (racer_has_chao(racer)) {
        if (racer_is_active(racer) && racer->state != RACER_FINISHING) {
            racer_take_up_item(racer);
            racer->behavior.step(racer);
        }
        racer_step_effect(racer);
        racer_update_segment(racer, kind);
        racer_update_path(racer, path);
        if (racer->flags & RACER_SPRAYED) {
            racer->flags &= ~RACER_SPRAYED;
        } else {
            spray_step(&racer->spray, 0, 0);
        }
        spray_draw(&racer->spray);
    }
}

void racer_entry_step(struct racer *racer)
{
    struct task_work *work = racer->work;
    racer_update_shown(racer);
    switch (racer->state) {
    case RACER_TO_ENTRY:
        racer->command = COMMAND_WINDOW;
        racer->state = RACER_IN_ENTRY;
    case RACER_IN_ENTRY:
        work->pos = racer_entry_places[racer->lane];
        break;
    }
}

void racer_spray(struct racer *racer, NJS_POINT3 *pos, NJS_VECTOR *speed)
{
    racer->flags |= RACER_SPRAYED;
    spray_set_flags(&racer->spray, DROP_SPRAYING);
    spray_step(&racer->spray, pos, speed);
}

void racer_draw_spray(struct racer *racer)
{
    spray_draw(&racer->spray);
}

int racer_is_active(struct racer *racer)
{
    switch (racer->state) {
    case RACER_TO_REST:
    case RACER_RESTING:
    case RACER_TO_WALK:
    case RACER_WALKING:
    case RACER_TO_RACE:
    case RACER_RACING:
    case RACER_FINISHING:
        return 1;
    }
    return 0;
}

int racer_at_goal(struct racer *racer)
{
    switch (racer->state) {
    case RACER_TO_GOAL:
    case RACER_AT_GOAL:
        return 1;
    }
    return 0;
}

int racer_at_podium(struct racer *racer)
{
    switch (racer->state) {
    case RACER_TO_PODIUM:
    case RACER_ON_PODIUM:
        return 1;
    }
    return 0;
}

int racer_take_command(struct racer *racer, int command)
{
    if (racer->command == command) {
        racer->command = COMMAND_NONE;
        return 1;
    }
    return 0;
}

int racer_is_followed(struct racer *racer)
{
    int lane;
    if (race_get_followed_lane(&lane) && lane == racer->lane) {
        return 1;
    }
    return 0;
}

void racer_clear_effect(struct racer *racer)
{
    if (racer->effect != EFFECT_NONE) {
        racer->command = COMMAND_RACE;
        racer->effect = EFFECT_NONE;
        racer->effect_timer = 0;
    }
}

/* choose EFFECT_NONE, EFFECT_DANCE or EFFECT_BOOST, each about a third of
 * the time */
static void racer_choose_effect(struct racer *racer)
{
    racer->effect = RANDOM(0.0F, 3.0F);
}

/* run the racer's effect a step, and count the step in effect_timer. a
 * dance issues COMMAND_DANCE at timer 0. it ends without a count when the
 * racer's command is COMMAND_RETURN or COMMAND_RACE. it also ends at timer
 * EFFECT_STEPS, and then issues COMMAND_RETURN. a speed boost sets
 * RACER_CHEERED at timer 0, and clears it at timer EFFECT_STEPS.
 *
 * an effect that ends at EFFECT_STEPS clears the timer and still counts the
 * step, which leaves the timer at 1. unless racer_reset clears it first, the
 * next effect starts at 1 and ends a step sooner. such a dance issues no
 * COMMAND_DANCE, and such a boost sets no RACER_CHEERED */
static void racer_step_effect(struct racer *racer)
{
    switch (racer->effect) {
    case EFFECT_NONE:
        break;
    case EFFECT_DANCE:
        if (racer->effect_timer == 0) {
            racer->command = COMMAND_DANCE;
        }
        /* @hack redundant: an else after the break, which has the timer's load
         * take R3 at 0x0cb87082; as its own if, 3 of 77 words differ */
        if (racer->command == COMMAND_RETURN ||
            racer->command == COMMAND_RACE) {
            racer->effect = EFFECT_NONE;
            racer->effect_timer = 0;
            break;
        } else if (racer->effect_timer >= EFFECT_STEPS) {
            racer->effect = EFFECT_NONE;
            racer->effect_timer = 0;
            racer->command = COMMAND_RETURN;
        }
        racer->effect_timer++;
        break;
    case EFFECT_BOOST:
        if (racer->effect_timer >= EFFECT_STEPS) {
            racer->effect = EFFECT_NONE;
            racer->effect_timer = 0;
            racer->flags &= ~RACER_CHEERED;
        } else if (racer->effect_timer == 0) {
            racer->flags |= RACER_CHEERED;
        }
        racer->effect_timer++;
        break;
    }
}

void racer_print_motion(struct racer *racer, int x, int y)
{
    if (!(racer->flags & RACER_HAS_CHAO)) {
        return;
    }
    njPrintC(NJM_LOCATION(x, y), "ID:");
    njPrintD(NJM_LOCATION(x + 3, y), racer->lane, 2);
    njPrintC(NJM_LOCATION(x + 7, y), "WIN:");
    njPrintD(NJM_LOCATION(x + 11, y), racer->chao->record->race_wins, 4);
    /* @bug memory: the Chao's mode, read at 0x0cb87178, indexes 15
     * labels, but race_chao.h's modes run to 22. a mode past 14 prints a row
     * of racer_command_labels, which follows at 0x0cb99f8c. the labels also
     * stop naming race_chao.h's modes at mode 5: MODE_FLY, 8, prints PUSH */
    njPrintC(NJM_LOCATION(x, y + 1), racer_mode_labels[racer->work->mode]);
    njPrintD(NJM_LOCATION(x + 6, y + 1), racer->work->submode, 2);
    njPrintC(NJM_LOCATION(x + 9, y + 1), racer_command_labels[racer->command]);
    njPrintF(NJM_LOCATION(x, y + 2), racer->distance, 4);
    njPrintC(NJM_LOCATION(x, y + 3), "X:");
    njPrintF(NJM_LOCATION(x + 2, y + 3), racer->work->pos.x, 4);
    njPrintF(NJM_LOCATION(x + 13, y + 3), racer->chao->speed.x, 1);
    njPrintC(NJM_LOCATION(x, y + 4), "Y:");
    njPrintF(NJM_LOCATION(x + 2, y + 4), racer->work->pos.y, 4);
    njPrintF(NJM_LOCATION(x + 13, y + 4), racer->chao->speed.y, 1);
    njPrintC(NJM_LOCATION(x, y + 5), "Z:");
    njPrintF(NJM_LOCATION(x + 2, y + 5), racer->work->pos.z, 4);
    njPrintF(NJM_LOCATION(x + 13, y + 5), racer->chao->speed.z, 1);
}

/* each chao_type's label, then PARTNER for 9, a type chao_type doesn't
 * name */
static const char racer_type_labels[10][LABEL_SIZE] = {
    "CHILD", "STANDARD", "SWIM", "FLY",     "RUN",
    "POWER", "CHAOS",    "EGG",  "NOTHING", "PARTNER"};

void racer_print_record(struct racer *racer, int x, int y)
{
    struct chao *record;
    int kind;
    if (!(racer->flags & RACER_HAS_CHAO)) {
        return;
    }
    record = racer->chao->record;
    njPrintC(NJM_LOCATION(x, y), "STATUS:");
    if (record->type <= 9 && record->type >= 0) {
        njPrintC(NJM_LOCATION(x + 8, y), racer_type_labels[record->type]);
    } else {
        njPrintH(NJM_LOCATION(x + 8, y), record->type, 2);
    }
    njPrintC(NJM_LOCATION(x + 7, y + 1), "WIN:");
    njPrintD(NJM_LOCATION(x + 11, y + 1), record->race_wins, 4);
    njPrintC(NJM_LOCATION(x, y + 2), "SWIM  :");
    njPrintD(NJM_LOCATION(x + 7, y + 2), record->skills.swim, 6);
    njPrintC(NJM_LOCATION(x, y + 3), "RUN   :");
    njPrintD(NJM_LOCATION(x + 7, y + 3), record->skills.run, 6);
    njPrintC(NJM_LOCATION(x, y + 4), "FLY   :");
    njPrintD(NJM_LOCATION(x + 7, y + 4), record->skills.fly, 6);
    njPrintC(NJM_LOCATION(x, y + 5), "POWER :");
    njPrintD(NJM_LOCATION(x + 7, y + 5), record->skills.power, 6);
    njPrintC(NJM_LOCATION(x, y + 6), "SLEEP :");
    njPrintD(NJM_LOCATION(x + 7, y + 6), record->sleep, 6);
    njPrintC(NJM_LOCATION(x, y + 7), "STRESS:");
    njPrintD(NJM_LOCATION(x + 7, y + 7), record->unk_65, 6);
    if (race_get_kind(&kind)) {
        /* @hack spelling: each time indexed from the first time's address,
         * which adds kind's offset at its full width (0x0cb8747e); as
         * race_times[kind], SHC narrows it to a byte with EXTS.B, the function
         * grows, and 60 of 210 words differ */
        njPrintD(NJM_LOCATION(x + 7, y + 8),
                 (&record->race_times[0])[kind].minutes, 2);
        njPrintD(NJM_LOCATION(x + 10, y + 8),
                 (&record->race_times[0])[kind].seconds, 2);
        njPrintD(NJM_LOCATION(x + 13, y + 8),
                 (&record->race_times[0])[kind].steps, 2);
    }
}

int racer_place(struct racer *racer)
{
    return racer->place;
}

/* aim the Chao at target: set the Chao's target, clear the racer's weave
 * phase and set RACER_AIMED */
static void racer_aim(struct racer *racer, NJS_POINT3 *target)
{
    struct alife_chao *chao = racer->chao;
    racer->flags |= RACER_AIMED;
    VECTOR_COPY(&chao->target, target);
    racer->phase = 0;
}

void racer_face_target(struct racer *racer, NJS_POINT3 *target)
{
    racer_aim(racer, target);
    if (racer_has_chao(racer)) {
        race_chao_face_target(racer->work, racer->chao);
    }
}

/* with its Chao, clear RACER_SHOWN while the Chao's work is closer than
 * HIDE_RANGE to camera_place's pos, and set it otherwise */
static void racer_update_shown(struct racer *racer)
{
    NJS_VECTOR delta;
    if (racer->flags & RACER_HAS_CHAO) {
        VECTOR_DIFF(&delta, &camera_place.pos, &racer->work->pos);
        if (VECTOR_SQUARED(&delta) < SQUARE(HIDE_RANGE)) {
            racer->flags &= ~RACER_SHOWN;
        } else {
            racer->flags |= RACER_SHOWN;
        }
    }
}

/* step the course section down or up until it holds the distance. section
 * n runs from race_course.segments[n - 1].distance, or from 0 for section 0,
 * up to its own distance. the sections stop at 0 and 16. kind is unread */
static void racer_update_segment(struct racer *racer, int kind)
{
    int done = 0;
    Float start;
    Float end;
    struct race_segment *segments = race_course.segments;
    do {
        if (racer->segment == 0) {
            start = 0.0F;
        } else {
            start = segments[racer->segment - 1].distance;
        }
        end = segments[racer->segment].distance;
        if (racer->distance < start) {
            if (racer->segment == 0) {
                done = 1;
            } else {
                racer->segment--;
            }
        } else if (racer->distance >= end) {
            if (racer->segment == 16) {
                done = 1;
            } else {
                racer->segment++;
            }
        } else {
            done = 1;
        }
    } while (!done);
}

void racer_hold_path(struct racer *racer)
{
    racer->flags |= RACER_HOLD_PATH;
}

/* update the racer's path, which first sets path_index to 2. of the cases
 * below, only the first that holds runs.
 *
 * - in RACER_FINISHING, while the path has a target, the Chao takes that
 *   target and the weave phase goes to 0
 * - with RACER_AIMED, the flag clears, and the racer takes only the distance
 *   of where the work stands
 * - with RACER_HOLD_PATH, the flag clears
 * - otherwise the racer follows its path */
static void racer_update_path(struct racer *racer, struct race_path *path)
{
    racer->path_index = 2;
    if (racer->state == RACER_FINISHING) {
        NJS_POINT3 target;
        if (race_path_get_target(path, &target)) {
            VECTOR_COPY(&racer->chao->target, &target);
            racer->phase = 0;
            return;
        }
    }
    if (racer->flags & RACER_AIMED) {
        struct race_path_position position;
        struct task_work *work = racer->work;
        racer->flags &= ~RACER_AIMED;
        race_path_locate(path, &work->pos, &position, LOCATE_SKIP_FIRST);
        racer->distance = position.distance;
    } else if (racer->flags & RACER_HOLD_PATH) {
        racer->flags &= ~RACER_HOLD_PATH;
    } else {
        racer_follow_path(racer, path);
    }
}

/* with the Chao and its work, locate the work on the paths and take its
 * distance. while RACER_SKIPS_FIRST_PATH is set and the Chao doesn't swim,
 * the first other path is skipped. the Chao's target goes 25 ahead on the
 * path found, where that path gives a point there. the weave phase advances
 * by WEAVE_STEP, and path_index takes the path's index in race_paths, where
 * it has one */
static void racer_follow_path(struct racer *racer, struct race_path *path)
{
    struct alife_chao *chao = racer->chao;
    struct task_work *work = racer->work;
    struct path_sample sample;
    struct race_path_position position;
    int mode;
    int i;
    if (chao && work) {
        if ((racer->flags & RACER_SKIPS_FIRST_PATH) &&
            work->mode != MODE_SWIM) {
            mode = LOCATE_SKIP_FIRST;
        } else {
            mode = LOCATE_ALL;
        }
        race_path_locate(path, &work->pos, &position, mode);
        racer->distance = position.distance;
        sample.distance = position.along + 25.0F;
        racer->phase += WEAVE_STEP;
        if (position.path) {
            if (path_sample(position.path, &sample)) {
                VECTOR_COPY(&chao->target, &sample.pos);
            }
        }
        for (i = 0; i < 3; i++) {
            if (position.path == race_paths[i]) {
                racer->path_index = i;
            }
        }
    }
}

int racer_path_index(struct racer *racer)
{
    return racer->path_index;
}

int racer_set_behavior(struct racer_behavior *behavior,
                       void (*step)(struct racer *), int priority)
{
    if (behavior->priority < priority) {
        behavior->priority = priority;
        behavior->state = 0;
        behavior->timer = 0;
        behavior->step = step;
        return 1;
    }
    return 0;
}

void racer_override_behavior(struct racer_behavior *behavior,
                             void (*step)(struct racer *))
{
    behavior->priority++;
    behavior->state = 0;
    behavior->timer = 0;
    behavior->step = step;
}

void racer_clear_behavior(struct racer_behavior *behavior)
{
    behavior->priority = 0;
    behavior->state = 0;
    behavior->timer = 0;
    behavior->step = racer_idle;
}

int racer_go_to(struct racer *racer, NJS_POINT3 *target, Float radius,
                int timeout)
{
    struct racer_behavior *behavior = &racer->behavior;
    int arrived = 0;
    NJS_VECTOR delta;
    if (behavior->timer == 0) {
        racer->command = COMMAND_RETURN;
    }
    VECTOR_DIFF(&delta, &racer->work->pos, target);
    if (ACROSS_SQUARED(&delta) > SQUARE(radius)) {
        racer_aim(racer, target);
    } else {
        arrived = 1;
    }
    behavior->timer++;
    if (behavior->timer > timeout) {
        return PROGRESS_TIMED_OUT;
    }
    if (arrived) {
        racer->command = COMMAND_RETURN;
        racer->phase = RANDOM_ANGLE();
        return PROGRESS_DONE;
    }
    return PROGRESS_GOING;
}

/* turn the Chao in place toward target, from a behavior's step, with
 * RACER_TURNING set. at timer 0, issue COMMAND_RETURN. each call aims the
 * Chao at target and counts a step. the Chao has turned once
 * race_chao_angle_to_target puts its heading less than FACING_ANGLE off
 * target, or, for a null task, once the count passes timeout. then clear
 * RACER_TURNING, issue COMMAND_RETURN and take a new weave phase.
 *
 * return PROGRESS_DONE once the Chao has turned, and PROGRESS_GOING until
 * then */
static int racer_turn_to(struct racer *racer, NJS_POINT3 *target, int timeout)
{
    struct racer_behavior *behavior = &racer->behavior;
    int turned = 0;
    Angle angle;
    if (behavior->timer == 0) {
        racer->command = COMMAND_RETURN;
    }
    racer->flags |= RACER_TURNING;
    racer_aim(racer, target);
    behavior->timer++;
    if (race_chao_angle_to_target(racer->chao->task, &angle)) {
        if (angle > -FACING_ANGLE && angle < FACING_ANGLE) {
            turned = 1;
        }
    } else if (behavior->timer > timeout) {
        turned = 1;
    }
    if (turned) {
        racer->flags &= ~RACER_TURNING;
        racer->command = COMMAND_RETURN;
        racer->phase = RANDOM_ANGLE();
        return PROGRESS_DONE;
    }
    return PROGRESS_GOING;
}

/* in the active states, issue command at timer 0 and count the step. once
 * the count passes timeout, issue COMMAND_RETURN and take a new weave phase.
 *
 * return PROGRESS_INACTIVE outside the active states, doing nothing.
 * otherwise return PROGRESS_DONE once the count passes timeout, and
 * PROGRESS_GOING until then */
static int racer_do_command(struct racer *racer, int timeout, int command)
{
    struct racer_behavior *behavior = &racer->behavior;
    if (!racer_is_active(racer)) {
        return PROGRESS_INACTIVE;
    }
    if (behavior->timer == 0) {
        racer->command = command;
    }
    behavior->timer++;
    if (behavior->timer > timeout) {
        racer->command = COMMAND_RETURN;
        racer->phase = RANDOM_ANGLE();
        return PROGRESS_DONE;
    }
    return PROGRESS_GOING;
}

static void racer_idle(struct racer *racer)
{
}

static void racer_clear_item(struct racer_reaction *reaction)
{
    reaction->status = REACTION_PENDING;
    reaction->source = 0;
    reaction->kind = -1;
    reaction->mode = -1;
}

/* 1 while a mode is bound. without one, set status back to REACTION_PENDING
 * and return 0 */
static int racer_item_bound_or_pending(struct racer_reaction *reaction)
{
    if (reaction->mode == -1) {
        reaction->status = REACTION_PENDING;
        return 0;
    }
    return 1;
}

int racer_tell(struct racer *racer, Sint8 mode, struct task *source, Sint8 kind)
{
    struct racer_reaction *reaction = &racer->reaction;
    reaction->status = REACTION_PENDING;
    switch (mode) {
    case ITEM_LET_GO:
        if (reaction->source == source && reaction->kind == kind) {
            reaction->source = 0;
            reaction->kind = -1;
            reaction->mode = -1;
        }
        break;
    case ITEM_GO_TO:
        reaction->mode = mode;
        break;
    default:
        reaction->mode = mode;
        reaction->source = source;
        reaction->kind = kind;
        /* @hack spelling: a return where the function's own follows, which
         * keeps MOV #1,R0 ahead of the RTS at 0x0cb87c88; with a break, it
         * moves into the delay slot, and 3 of 36 words differ */
        return 1;
    }
    return 1;
}

/* tell a source that is a watering can, a bugle or a scoop that the play is
 * done. then, or without a source, clear the reaction and the behavior. a
 * source of another kind changes nothing */
static void racer_release_item(struct racer *racer)
{
    struct racer_reaction *reaction = &racer->reaction;
    int command;
    /* @bug uninitialized: without a source, no path sets command, and the
     * call passes the caller's R13 (MOV R13,R6 at 0x0cb87cf2). item_tell,
     * given a null task, never reads it */
    /* NOLINTNEXTLINE(clang-diagnostic-sometimes-uninitialized) */
    if (reaction->source) {
        switch (reaction->kind) {
        case WATERING_CAN_KIND:
            command = WATERING_CAN_DONE;
            break;
        case BUGLE_KIND:
            command = BUGLE_DONE;
            break;
        case SCOOP_KIND:
            command = SCOOP_DONE;
            break;
        default:
            return;
        }
    }
    item_tell(reaction->source, racer->chao->task, command, reaction->kind);
    racer_clear_item(reaction);
    racer_clear_behavior(&racer->behavior);
}

void racer_cancel_item(struct racer *racer)
{
    struct racer_reaction *reaction = &racer->reaction;
    if (reaction->source) {
        item_tell(reaction->source, racer->chao->task, ITEM_LET_GO,
                  reaction->kind);
        racer_clear_item(reaction);
    }
    racer_clear_behavior(&racer->behavior);
}

int racer_item_behavior(struct racer *racer)
{
    if (racer->behavior.step == racer_play) {
        return 1;
    } else if (racer->behavior.step == racer_eat) {
        return 2;
    } else if (racer->behavior.step == racer_swing) {
        return 3;
    } else if (racer->behavior.step == racer_go_to_coconut) {
        return 4;
    } else if (racer->behavior.step == racer_push) {
        return 5;
    }
    return 0;
}

/* while status is REACTION_PENDING, take up the bound mode's behavior, one
 * priority higher, and set status to REACTION_TAKEN. for ITEM_GO_TO, first
 * tell the bound source COCONUT_PALM_STOP through the coconut palm's
 * handler, whatever the source's kind */
static void racer_take_up_item(struct racer *racer)
{
    struct racer_reaction *reaction = &racer->reaction;
    switch (reaction->mode) {
    case -1:
        break;
    case ITEM_SWING:
        if (reaction->status == REACTION_PENDING) {
            racer_override_behavior(&racer->behavior, racer_swing);
            reaction->status = REACTION_TAKEN;
        }
        break;
    case ITEM_GO_TO:
        if (reaction->status == REACTION_PENDING) {
            item_tell(reaction->source, racer->chao->task, COCONUT_PALM_STOP,
                      COCONUT_PALM_KIND);
            racer_override_behavior(&racer->behavior, racer_go_to_coconut);
            reaction->status = REACTION_TAKEN;
        }
        break;
    case ITEM_EAT:
        if (reaction->status == REACTION_PENDING) {
            racer_override_behavior(&racer->behavior, racer_eat);
            reaction->status = REACTION_TAKEN;
        }
        break;
    case ITEM_PUSH:
        if (reaction->status == REACTION_PENDING) {
            racer_override_behavior(&racer->behavior, racer_push);
            reaction->status = REACTION_TAKEN;
        }
        break;
    case ITEM_TAKE:
        if (reaction->status == REACTION_PENDING) {
            racer_override_behavior(&racer->behavior, racer_play);
            reaction->status = REACTION_TAKEN;
        }
        break;
    }
}

/* swing the coconut palm: turn to it, then issue COMMAND_SWING for 420
 * steps, aimed at it. at its 60th step, tell it COCONUT_PALM_SWAY, and end
 * unless it answers 1. the end, or leaving the active states, drops the
 * source and sets status to REACTION_DONE. an unbound mode ends it at once */
static void racer_swing(struct racer *racer)
{
    int result;
    struct racer_behavior *behavior = &racer->behavior;
    struct racer_reaction *reaction = &racer->reaction;
    if (racer_item_bound_or_pending(reaction)) {
        switch (behavior->state) {
        case 0:
            result = racer_turn_to(racer, &reaction->source->work->pos,
                                   TURN_TIMEOUT);
            switch (result) {
            case PROGRESS_GOING:
                break;
            case PROGRESS_DONE:
                behavior->state++;
                behavior->timer = 0;
                break;
            default:
                racer_clear_behavior(behavior);
                reaction->status = REACTION_DONE;
                break;
            }
            break;
        case 1:
            result = racer_do_command(racer, 420, COMMAND_SWING);
            racer_aim(racer, &reaction->source->work->pos);
            /* @hack spelling: the timer read through racer, not behavior, which
             * loads it from R12 + 0xb6 at 0x0cb87fd4; through behavior, 49 of
             * 102 words differ */
            if (racer->behavior.timer == 60) {
                if (item_tell(reaction->source, racer->chao->task,
                              COCONUT_PALM_SWAY, COCONUT_PALM_KIND) != 1) {
                    result = PROGRESS_DONE;
                }
            }
            /* @hack redundant: a case for PROGRESS_DONE with the default's
             * body, whose CMP/EQ #1 the image keeps at 0x0cb87ffc; without it,
             * 75 of 102 words differ */
            switch (result) {
            case PROGRESS_GOING:
                break;
            case PROGRESS_DONE:
                racer_clear_behavior(behavior);
                reaction->source = 0;
                reaction->status = REACTION_DONE;
                break;
            default:
                racer_clear_behavior(behavior);
                reaction->source = 0;
                reaction->status = REACTION_DONE;
                break;
            }
            break;
        }
    } else {
        racer_clear_behavior(behavior);
    }
}

/* eat the coconut: turn to it and tell it PALM_COCONUT_EAT. without an
 * answer of 1, end with status REACTION_DONE. then issue COMMAND_EAT for 60
 * steps, aimed at it. at the end, tell it PALM_COCONUT_EATEN and choose
 * an effect; leaving the active states ends it without either. both ends
 * drop the source and set status to REACTION_DONE. an unbound mode ends it at
 * once */
static void racer_eat(struct racer *racer)
{
    int result;
    struct racer_behavior *behavior = &racer->behavior;
    struct racer_reaction *reaction = &racer->reaction;
    if (racer_item_bound_or_pending(reaction)) {
        switch (behavior->state) {
        case 0:
            result = racer_turn_to(racer, &reaction->source->work->pos,
                                   TURN_TIMEOUT);
            switch (result) {
            case PROGRESS_GOING:
                break;
            case PROGRESS_DONE:
                if (item_tell(reaction->source, racer->chao->task,
                              PALM_COCONUT_EAT, PALM_COCONUT_KIND) == 1) {
                    behavior->state++;
                    behavior->timer = 0;
                } else {
                    racer_clear_behavior(behavior);
                    reaction->status = REACTION_DONE;
                }
                break;
            default:
                racer_clear_behavior(behavior);
                reaction->status = REACTION_DONE;
                break;
            }
            break;
        case 1:
            result = racer_do_command(racer, 60, COMMAND_EAT);
            racer_aim(racer, &reaction->source->work->pos);
            switch (result) {
            case PROGRESS_GOING:
                break;
            case PROGRESS_DONE:
                item_tell(reaction->source, racer->chao->task,
                          PALM_COCONUT_EATEN, PALM_COCONUT_KIND);
                racer_choose_effect(racer);
                racer_clear_behavior(behavior);
                reaction->source = 0;
                reaction->status = REACTION_DONE;
                break;
            default:
                racer_clear_behavior(behavior);
                reaction->source = 0;
                reaction->status = REACTION_DONE;
                break;
            }
            break;
        }
    } else {
        racer_clear_behavior(behavior);
    }
}

/* push the ball: turn to it and tell it BALL_PUSH. without an answer of 1,
 * end with status REACTION_DONE. then each step, issue COMMAND_PUSH, which
 * racer_go_to overwrites with COMMAND_RETURN at timer 0, and head within 10
 * of race_course.targets[lane]. once the Chao arrives or the racer's distance
 * passes 216, tell the ball BALL_STOP, drop the source, issue COMMAND_RACE
 * and set status to REACTION_DONE. past racer_go_to's 1800 steps, it times
 * out even on arriving, and only the distance ends the push. an unbound mode
 * issues COMMAND_RACE and ends it */
static void racer_push(struct racer *racer)
{
    int result;
    struct racer_behavior *behavior = &racer->behavior;
    struct racer_reaction *reaction = &racer->reaction;
    NJS_POINT3 *target = &race_course.targets[racer->lane];
    if (racer_item_bound_or_pending(reaction)) {
        switch (behavior->state) {
        case 0:
            result = racer_turn_to(racer, &reaction->source->work->pos,
                                   TURN_TIMEOUT);
            switch (result) {
            case PROGRESS_GOING:
                break;
            case PROGRESS_DONE:
                if (item_tell(reaction->source, racer->chao->task, BALL_PUSH,
                              BALL_KIND) == 1) {
                    behavior->state++;
                    behavior->timer = 0;
                } else {
                    racer_clear_behavior(behavior);
                    reaction->status = REACTION_DONE;
                }
                break;
            default:
                racer_clear_behavior(behavior);
                reaction->status = REACTION_DONE;
                break;
            }
            break;
        case 1:
            racer->command = COMMAND_PUSH;
            result = racer_go_to(racer, target, 10.0F, 1800);
            if (result == PROGRESS_DONE || racer->distance > 216.0F) {
                item_tell(reaction->source, racer->chao->task, BALL_STOP,
                          BALL_KIND);
                racer_clear_behavior(behavior);
                reaction->status = REACTION_DONE;
                reaction->source = 0;
                racer->command = COMMAND_RACE;
                /* @hack redundant: a break ahead of the switch's own, which
                 * puts COMMAND_RACE in R1 for the unbound path's store at
                 * 0x0cb882c0; without it, 2 of 132 words differ */
                break;
            }
            break;
        }
    } else {
        racer->command = COMMAND_RACE;
        racer_clear_behavior(behavior);
    }
}

/* play the item the Chao took. with RACER_CHEERED, end at once with status
 * REACTION_DONE. with a source bound, turn to it, then issue its kind's
 * command, holding the item at the Chao each step. the bugle issues
 * COMMAND_PLAY_RAPPA for 540 steps, and the watering can COMMAND_PLAY_JYOURO
 * for 480. any other kind issues COMMAND_PLAY_SCOOP for 420. the play ends once
 * its steps run out or on leaving the active states. then release the item,
 * drop the source, issue COMMAND_RACE and set status to REACTION_DONE. with no
 * mode or no source bound, end it at once */
static void racer_play(struct racer *racer)
{
    int result;
    struct racer_behavior *behavior = &racer->behavior;
    struct racer_reaction *reaction = &racer->reaction;
    if (racer->flags & RACER_CHEERED) {
        racer_clear_behavior(behavior);
        reaction->status = REACTION_DONE;
        return;
    }
    if (racer_item_bound_or_pending(reaction) && reaction->source) {
        switch (behavior->state) {
        case 0:
            result = racer_turn_to(racer, &reaction->source->work->pos,
                                   TURN_TIMEOUT);
            switch (result) {
            case PROGRESS_GOING:
                break;
            case PROGRESS_DONE:
                behavior->state++;
                behavior->timer = 0;
                break;
            default:
                racer_clear_behavior(behavior);
                reaction->status = REACTION_DONE;
                break;
            }
            break;
        case 1:
            /* @hack redundant: a case for SCOOP_KIND, the default's own, whose
             * CMP/EQ #5 the image keeps at 0x0cb883d6; without it, 59 of 120
             * words differ */
            switch (reaction->kind) {
            case BUGLE_KIND:
                result = racer_do_command(racer, 540, COMMAND_PLAY_RAPPA);
                break;
            case WATERING_CAN_KIND:
                result = racer_do_command(racer, 480, COMMAND_PLAY_JYOURO);
                break;
            case SCOOP_KIND:
            default:
                result = racer_do_command(racer, 420, COMMAND_PLAY_SCOOP);
                break;
            }
            racer_hold_item(racer, reaction->source->work, reaction->kind);
            switch (result) {
            case PROGRESS_GOING:
                break;
            default:
                racer_release_item(racer);
                racer_clear_behavior(behavior);
                reaction->source = 0;
                reaction->status = REACTION_DONE;
                racer->command = COMMAND_RACE;
                break;
            }
            break;
        }
    } else {
        racer_clear_behavior(behavior);
    }
}

/* walk to where the lane's last coconut came to rest. racer_go_to_lane_point's
 * code comes right after it, so SHC drops the tail call's branch. the image's
 * racer_go_to_coconut is then only the load of palm_coconut_get_rest at
 * 0x0cb884a0, which runs on into racer_go_to_lane_point */
static void racer_go_to_coconut(struct racer *racer)
{
    racer_go_to_lane_point(racer, palm_coconut_get_rest);
}

/* head within 3 of the point get_point gives for the lane, for up to 120
 * steps, then end with status REACTION_DONE. when get_point gives none, end
 * at once with REACTION_DONE. an unbound mode ends it at once */
static void racer_go_to_lane_point(struct racer *racer,
                                   int (*get_point)(NJS_POINT3 *point,
                                                    int lane))
{
    struct racer_behavior *behavior = &racer->behavior;
    struct racer_reaction *reaction = &racer->reaction;
    int result;
    NJS_POINT3 point;
    if (racer_item_bound_or_pending(reaction)) {
        /* @hack redundant: a switch on the mode whose one case, ITEM_GO_TO,
         * shares the default's body. it keeps the image's CMP/EQ #2 at
         * 0x0cb884c4, whose branch goes to the next instruction either way;
         * without it, 46 of 65 words differ */
        switch (reaction->mode) {
        case ITEM_GO_TO:
        default:
            if (get_point(&point, racer->lane) != 1) {
                racer_clear_behavior(behavior);
                reaction->status = REACTION_DONE;
            } else {
                result = racer_go_to(racer, &point, 3.0F, 120);
                /* @hack redundant: a case for PROGRESS_DONE with the default's
                 * body, whose call of racer_clear_behavior the image copies at
                 * 0x0cb88500 and 0x0cb88508; without it, the function is 18
                 * bytes shorter and 62 of 65 words differ */
                switch (result) {
                case PROGRESS_GOING:
                    break;
                case PROGRESS_DONE:
                    racer_clear_behavior(behavior);
                    reaction->status = REACTION_DONE;
                    break;
                default:
                    racer_clear_behavior(behavior);
                    reaction->status = REACTION_DONE;
                    break;
                }
            }
            break;
        }
    } else {
        racer_clear_behavior(behavior);
    }
}

/* hold the item at the Chao, where work and kind are the item's. work goes
 * to the Chao's work's pos, offset by where a node lies in the Chao's model's
 * space. the node is 4 for the bugle and 7 for any other kind. the item
 * takes the Chao's turn about y, negated. the scoop also turns 90 degrees
 * about x and z and sits 0.2 less in x. a kind other than the watering can,
 * the bugle and the scoop turns -100 degrees about x */
static void racer_hold_item(struct racer *racer, struct task_work *work,
                            Sint8 kind)
{
    NJS_VECTOR offset;
    int node;
    VECTOR_CLEAR(&offset);
    /* @hack redundant: cases for WATERING_CAN_KIND and SCOOP_KIND with the
     * default's node, whose CMP/EQ #3 and #5 the image keeps at 0x0cb88572 and
     * 0x0cb88580; without them, the function shrinks and 108 of 120 words
     * differ */
    switch (kind) {
    case BUGLE_KIND:
        node = 4;
        break;
    case WATERING_CAN_KIND:
        node = 7;
        break;
    case SCOOP_KIND:
        node = 7;
        break;
    default:
        node = 7;
        break;
    }
    /* @bug memory: a null work skips race_chao_node_pos, but the turn and the
     * position are written through it all the same. TST R14,R14 at 0x0cb8858e
     * branches to the stores from 0x0cb885a4 */
    if (!work ||
        race_chao_node_pos(&offset, racer->work, racer->chao, node) == 1) {
        /* @hack spelling: WATERING_CAN_KIND's case a copy of BUGLE_KIND's,
         * which the image holds twice, at 0x0cb885b8 and 0x0cb885c4; as a
         * second label on BUGLE_KIND's case, 25 of 120 words differ */
        switch (kind) {
        case WATERING_CAN_KIND:
            work->ang.y = -racer->work->ang.y;
            work->ang.x = 0;
            work->ang.z = 0;
            break;
        case BUGLE_KIND:
            work->ang.y = -racer->work->ang.y;
            work->ang.x = 0;
            work->ang.z = 0;
            break;
        case SCOOP_KIND:
            work->ang.y = -racer->work->ang.y;
            work->ang.x = NJM_DEG_ANG(90);
            work->ang.z = NJM_DEG_ANG(90);
            offset.x -= 0.2F;
            break;
        default:
            work->ang.y = -racer->work->ang.y;
            work->ang.x = NJM_DEG_ANG(-100);
            work->ang.z = 0;
            break;
        }
    }
    VECTOR_SUM(&work->pos, &offset, &racer->work->pos);
}
