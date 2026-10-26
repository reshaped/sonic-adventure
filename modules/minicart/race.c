/* the race: the lights, the laps the zones count, and the clock */
#include <ninja.h>
#include <sg_xpt.h>

#include "draw/state.h"
#include "minicart/cart_time.h"
#include "minicart/lap.h"
#include "minicart/race.h"
#include "minicart/score.h"
#include "minicart/times.h"
#include "sound/cue.h"
#include "stage/stage.h"
#include "system/frame.h"
#include "system/pad.h"
#include "system/task.h"
#include "system/task_desc.h"

#define CART_LAPS 3

/* the steps from one light to the next */
#define LIGHT_STEPS 60
/* the steps a lap's split, or at the finish the gap to the best, blinks */
#define SPLIT_STEPS 180
/* where each clock stops: 100 minutes at 60 steps a second */
#define CLOCK_STEPS 360000
/* the timer's bit that shows a blinking time, 4 steps in every 8 */
#define BLINK 4

/* the lights' beep, the beep as they go green, and the goal's */
#define SE_LIGHT 662
#define SE_GO 663
#define SE_GOAL 664

/* the race's record, in its task's motion */
struct cart_race {
    Sint8 mode; /* a race_mode */
    /* the lap the player is on, from 0. it is -1 until the player first
     * enters ZONE_GOAL, and CART_LAPS once the race is finished */
    Sint8 lap;
    /* the furthest lap begun, from 0 to CART_LAPS - 1. the hud shows it, and
     * lap_steps times it */
    Sint8 timed_lap;
    Sint8 dial; /* what cart_race_dial gives */
    Sint8 zone; /* the race_zone the player entered last */
    Sint16 timer; /* the steps to the next light, or a blink's steps left */
    int steps; /* the run's clock, from the green light */
    int lap_steps[CART_LAPS]; /* each lap's own clock */
    int splits[CART_LAPS]; /* the run's time as each lap ended, in hundredths */
    int best_time; /* the save's best time, in hundredths */
    int best_splits[CART_LAPS]; /* the best run's splits, in hundredths */
    Uint8 unk_34[12];
};

static void cart_race_init(struct task *task, void *arg);
static void cart_race_step(struct task *task);
static void cart_race_draw(struct task *task);
static void cart_race_load_bests(struct cart_race *race);
static void cart_race_count(struct cart_race *race);
static void cart_race_finish(struct cart_race *race);

static const struct sound_falloff cart_race_light_falloff = {127, -128, 20.0F,
                                                             600.0F};
static const struct sound_falloff cart_race_go_falloff = {127, -128, 20.0F,
                                                          600.0F};
static const struct sound_falloff cart_race_goal_falloff = {127, -128, 20.0F,
                                                            600.0F};
static const struct task_desc cart_race_desc = {
    sizeof(struct cart_race), 0,   cart_race_init, cart_race_step,
    cart_race_draw,           NULL};
static const NJS_ARGB cart_race_white = {1.0F, 1.0F, 1.0F, 1.0F};

static struct task *cart_race_task = NULL;

struct task *cart_race_create(void)
{
    /* @hack spelling: the result through task, which keeps the call a JSR
     * (0x0c90b008); returned straight, SHC jumps to task_create_desc and 9
     * of 9 words differ */
    struct task *task = task_create_desc(2, &cart_race_desc, NULL);

    return task;
}

void cart_race_kill(void)
{
    if (cart_race_task) {
        task_kill(cart_race_task);
        cart_race_task = NULL;
    }
}

void cart_race_destroy(void)
{
    if (cart_race_task) {
        task_destroy(cart_race_task);
        cart_race_task = NULL;
    }
}

struct task *cart_race_create_child(const struct task_desc *desc, void *arg)
{
    struct task *parent = cart_race_task;
    /* @hack spelling: the result through task, which takes both cases to one
     * return (BRA at 0x0c90b05c); returned straight, the null case returns
     * on its own and 14 of 22 words differ */
    struct task *task =
        parent == NULL ? NULL : task_create_desc_child(parent, desc, arg);

    return task;
}

Bool cart_race_reset(void)
{
    Bool reset = FALSE;

    if (cart_race_task) {
        struct cart_race *race = (struct cart_race *)cart_race_task->motion;
        Sint8 i;
        int *split;

        race->mode = RACE_READY;
        race->lap = -1;
        race->timed_lap = 0;
        race->dial = 0;
        /* race_enter_zone then reads the first entry to ZONE_GOAL as one
         * from ZONE_NEXT, which begins lap 0 */
        race->zone = ZONE_NEXT;
        race->timer = LIGHT_STEPS;
        race->steps = 0;
        /* @bug logic: split never moves on, so each pass clears the
         * first split, and the others keep what they held (MOV.L R5,@R1 at
         * 0x0c90b0ac, 0x0c90b0b4 and 0x0c90b0bc). nothing reads a split
         * before its lap's end writes it */
        for (i = 0, split = race->splits; i < CART_LAPS; i++) {
            *split = 0;
            race->lap_steps[i] = 0;
        }
        cart_race_load_bests(race);
        reset = TRUE;
    }
    return reset;
}

Bool cart_race_start_lights(void)
{
    Bool started = FALSE;

    if (cart_race_task) {
        struct cart_race *race = (struct cart_race *)cart_race_task->motion;

        if (race->mode == RACE_READY) {
            race->mode = RACE_COUNTDOWN;
            race->timer = LIGHT_STEPS;
            started = TRUE;
        }
    }
    return started;
}

Sint8 cart_race_mode(void)
{
    Sint8 mode = RACE_NONE;

    if (cart_race_task) {
        struct cart_race *race = (struct cart_race *)cart_race_task->motion;

        mode = race->mode;
    }
    return mode;
}

Sint8 cart_race_dial(void)
{
    Sint8 dial = 0;

    if (cart_race_task) {
        struct cart_race *race = (struct cart_race *)cart_race_task->motion;

        dial = race->dial;
    }
    return dial;
}

void race_enter_zone(Sint8 zone)
{
    struct cart_race *race;
    Sint8 last;
    Sint8 lap;
    Sint8 timed_lap;
    Sint8 dial;

    if (cart_race_task) {
        race = (struct cart_race *)cart_race_task->motion;
        last = race->zone;
        lap = race->lap;
        timed_lap = race->timed_lap;
        dial = race->dial;
        /* @hack redundant: the cases that do nothing, which SHC tests one by
         * one before each branch to the end (CMP/EQ #0,R0 at 0x0c90b18e).
         * without them, race_enter_zone runs 80 bytes short and 124 of 159
         * words differ */
        switch (zone) {
        case ZONE_GOAL:
            switch (last) {
            case ZONE_GOAL:
                break;
            case ZONE_PREVIOUS:
                break;
            case ZONE_NEXT:
                if (lap < CART_LAPS) {
                    int time = cart_hundredths_of(race->steps);

                    lap++;
                    if (lap >= CART_LAPS) {
                        race->splits[timed_lap] = time;
                        timed_lap = CART_LAPS - 1;
                        cart_race_finish(race);
                    } else if (lap > timed_lap) {
                        race->splits[timed_lap] = time;
                        timed_lap = lap;
                        race->timer = SPLIT_STEPS;
                        race->mode = RACE_LAP;
                    }
                }
                break;
            }
            break;
        case ZONE_PREVIOUS:
            switch (last) {
            case ZONE_GOAL:
                break;
            case ZONE_PREVIOUS:
                break;
            case ZONE_NEXT:
                if (timed_lap == lap && lap > -1 && lap < CART_LAPS) {
                    dial--;
                    if (dial < DIAL_GO) {
                        dial = DIAL_GO;
                    }
                }
                break;
            }
            break;
        case ZONE_NEXT:
            switch (last) {
            case ZONE_GOAL:
                /* @hack spelling: race->lap read again, MOV.B @(1,R14),R0 at
                 * 0x0c90b280. as lap, which holds the same, race_enter_zone
                 * runs 12 bytes short and 74 of 159 words differ */
                if (race->lap > -1) {
                    lap--;
                }
                break;
            case ZONE_PREVIOUS:
                if (timed_lap == lap && lap > -1 && lap < CART_LAPS) {
                    if (dial < DIAL_GO) {
                        dial = DIAL_GO;
                    }
                    dial++;
                    if (dial > DIAL_HOME_STRETCH) {
                        dial = DIAL_HOME_STRETCH;
                    }
                }
                break;
            case ZONE_NEXT:
                break;
            }
            break;
        }
        race->zone = zone;
        race->lap = lap;
        race->timed_lap = timed_lap;
        race->dial = dial;
    }
}

static void cart_race_init(struct task *task, void *arg)
{
    cart_race_task = task;
    cart_race_reset();
}

/* run the race's timer and clocks, draw the race and step its children. the
 * lights beep LIGHT_STEPS apart, three times. at the fourth they go green,
 * and player 0's pad drives the player */
static void cart_race_step(struct task *task)
{
    struct cart_race *race = (struct cart_race *)task->motion;

    switch (race->mode) {
    case RACE_READY:
        break;
    case RACE_COUNTDOWN:
        race->timer--;
        if (race->timer <= 0) {
            race->dial++;
            if (race->dial < DIAL_GO) {
                sound_play_near(SE_LIGHT, NULL, &cart_race_light_falloff);
                race->timer = LIGHT_STEPS;
            } else {
                sound_play_near(SE_GO, NULL, &cart_race_go_falloff);
                pad_enable_player(0);
                race->mode = RACE_GO;
            }
        }
        break;
    case RACE_LAP:
        race->timer--;
        if (race->timer <= 0) {
            race->mode = RACE_GO;
        }
        /* falls through: the clocks count on while the split blinks */
    case RACE_GO:
        cart_race_count(race);
        break;
    case RACE_FINISH:
        race->timer--;
        if (race->timer <= 0) {
            race->mode = RACE_OVER;
        }
        break;
    case RACE_OVER:
        break;
    }
    cart_race_draw(task);
    task_step_children(task);
}

/* on a frame's first step, unless hud_hidden is below 0, draw the lap and
 * the clock. the clock shows the run's time. in RACE_LAP, the finished
 * lap's split and its gap to the best blink in its place. once the race is
 * finished, the run's time stays, and in RACE_FINISH its gap to the best
 * blinks */
static void cart_race_draw(struct task *task)
{
    struct cart_race *race;
    int time;

    if (frame_step || hud_hidden < 0) {
        return;
    }
    race = (struct cart_race *)task->motion;
    time = cart_hundredths_of(race->steps);
    draw_state_save();
    njColorBlendingMode(NJD_SOURCE_COLOR, NJD_COLOR_BLENDING_BOTHSRCALPHA);
    njTextureFilterMode(NJD_TEXTUREFILTER_POINT_SAMPLE);
    njSetConstantMaterial((NJS_ARGB *)&cart_race_white);
    lap_draw(race->timed_lap);
    cart_time_draw_label();
    switch (race->mode) {
    case RACE_READY:
    case RACE_COUNTDOWN:
    case RACE_GO:
    default:
        cart_time_draw(time);
        break;
    case RACE_LAP:
        if (race->timer & BLINK) {
            Sint8 lap = race->timed_lap - 1;

            cart_time_draw(race->splits[lap]);
            cart_time_draw_gap(race->splits[lap], race->best_splits[lap]);
        }
        break;
    case RACE_FINISH:
        if (race->timer & BLINK) {
            cart_time_draw_gap(time, race->best_time);
        }
    case RACE_OVER:
        cart_time_draw_final(time, race->best_time);
        break;
    }
    njTextureFilterMode(NJD_TEXTUREFILTER_BILINEAR);
    draw_state_restore();
}

static void cart_race_load_bests(struct cart_race *race)
{
    Sint8 i;
    int *best;

    race->best_time = cart_best_time();
    for (i = 0, best = race->best_splits; i < CART_LAPS; i++, best++) {
        *best = cart_best_split(i);
    }
}

/* count a step on the run's clock and the lap's, each up to CLOCK_STEPS */
static void cart_race_count(struct cart_race *race)
{
    if (race->steps < CLOCK_STEPS) {
        race->steps++;
    }
    if (race->lap_steps[race->timed_lap] < CLOCK_STEPS) {
        race->lap_steps[race->timed_lap]++;
    }
}

/* beep at the goal, and blink the gap to the best for SPLIT_STEPS. hand
 * cart_set_split and cart_set_lap each lap's split and own time, in
 * hundredths, then start the score */
static void cart_race_finish(struct cart_race *race)
{
    Sint8 i;
    int *split;
    int *steps;

    sound_play_near(SE_GOAL, NULL, &cart_race_goal_falloff);
    race->timer = SPLIT_STEPS;
    race->mode = RACE_FINISH;
    for (i = 0, split = race->splits, steps = race->lap_steps; i < CART_LAPS;
         i++) {
        int time;

        cart_set_split(i, *split);
        split++;
        time = cart_hundredths_of(*steps);
        steps++;
        cart_set_lap(i, time);
    }
    cart_score_start();
}
