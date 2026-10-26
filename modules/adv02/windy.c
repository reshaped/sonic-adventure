/* the Mystic Ruins' winds. O CHG WINDY is the whirlwind into Windy Valley, and
 * O CHG CAVE and O CHG CAVE2 are the cave's two updrafts. O W LEAF1, O W LEAF2
 * and O W BOARD are leaves and a board that drift on the wind */
#include <ninja.h>
#include <sg_xpt.h>
#include <stdlib.h>

#include "adv02/mystic_ruins.h"
#include "adv02/windy.h"
#include "debug/editor.h"
#include "draw/model.h"
#include "effect/dust.h"
#include "effect/particle.h"
#include "effect/pools.h"
#include "object/placement.h"
#include "player/player.h"
#include "progress/progress.h"
#include "stage/stage.h"
#include "system/task.h"
#include "system/texture.h"
#include "system/util.h"

/* @hack type: the board's and the leaves' objects declared as arrays, and read
 * through ->. SHC then loads each object's address and reads its model 4 bytes
 * past it, as windy_leaf_2_draw does (MOV.L @(4,R4),R4 at 0x0c9106e6). as
 * NJS_OBJECTs read through ., 2 of windy_leaf_2_draw's 44 words differ, 2 of
 * windy_board_draw's 58, 2 of windy_leaf_1_draw's 58 and 3 of
 * windy_debris_leaves' 89 */
extern NJS_OBJECT windy_board_object[];
extern NJS_OBJECT windy_twig_object;
extern NJS_OBJECT windy_leaf_1_object[];
extern NJS_OBJECT windy_leaf_2_object[];

/* the radius of a wind's cylinder, and how far it reaches above and below the
 * wind. the still whirlwind reaches STILL_HEIGHT, and a blowing wind
 * BLOWING_HEIGHT */
#define WIND_RADIUS 30.0F
#define STILL_HEIGHT 10.0F
#define BLOWING_HEIGHT 100.0F

/* a blowing wind's big leaves start within BIG_LEAF_RADIUS of it in x and z.
 * its big leaves and bits start DEBRIS_DROP below it */
#define BIG_LEAF_RADIUS 40.0F
#define DEBRIS_DROP 20.0F
/* a leaf the still whirlwind throws up bobs about LEAF_RISE above the
 * whirlwind */
#define LEAF_RISE 20.0F

/* a blowing wind's lift falls by LIFT_FALL for each unit player 0 is above the
 * wind. it never falls below LIFT_LEAST */
#define LIFT_FALL 0.02F
#define LIFT_LEAST 0.01F

/* the most lift the whirlwind builds up to, from 0, while it stands still. it
 * blows with the lift it built */
#define WHIRL_LIFT 0.5F
/* the still whirlwind's phase, in work->ang.y, moves WHIRL_PHASE_STEP a step.
 * each step the whirlwind builds (sin(phase) + 1) * WHIRL_BUILD of lift */
#define WHIRL_PHASE_STEP 0x800
#define WHIRL_BUILD 0.05F
/* a cave updraft's lift, and what the one down to the cave's mouth loses each
 * step once it has sent player 0 on */
#define CAVE_LIFT 0.7F
#define CAVE_FADE 0.02F

/* a blowing wind sends player 0 where it leads once player 0 has been in it
 * more than RIDE_STEPS steps in a row */
#define RIDE_STEPS 20

/* where the updraft up from the cave sends player 0: the island's entrance at
 * the top of the cave */
#define CAVE_TOP_ENTRANCE 0
/* where the updraft down sends player 0: the station's entrance at the cave's
 * mouth */
#define CAVE_MOUTH_ENTRANCE 1

/* windy_leaf_2_step's leaf counts down LEAF_STEPS steps before it ends, and a
 * gust GUST_STEPS. a big leaf counts down BIG_LEAF_STEPS, and a bit of board,
 * twig or leaf BIT_STEPS */
#define LEAF_STEPS 180
#define GUST_STEPS 60
#define BIG_LEAF_STEPS 40
#define BIT_STEPS 80

/* a wind's vars. while a wind blows, work->timer counts the steps in a row
 * player 0 has been in it */
struct updraft_vars {
    Uint8 unk_00[8];
    Float lift; /* the lift the wind gives player 0 at the wind's height */
};

#define UPDRAFT(work) ((struct updraft_vars *)&(work)->vars)
/* the steps the whirlwind or windy_cave_up_step's updraft has run, held in
 * work->ang.x. they time the leaves, gusts and bits the wind raises */
#define STEPS(work) ((work)->ang.x)

/* a leaf the whirlwind throws up. it keeps its scale in work->tick, in
 * sixteenths, its steps left in work->timer, and its speed in work->scl */
struct leaf_vars {
    /* its turn a step about x, and about y */
    Uint16 spin_x;
    Uint16 spin_y;
    Sint8 kind; /* 0, 1 or 2 at random, which nothing reads */
    Uint8 unk_05[3];
    Float top; /* the height it bobs about */
};

#define LEAF(work) ((struct leaf_vars *)&(work)->vars)

/* a drifting leaf or board. work->scl.x holds its scale, from its set entry.
 * work->scl.y starts as how far above its position it bobs, and then holds its
 * upward speed. work->timer holds a phase that shapes its rise */
struct drift_vars {
    /* how far the phases of its tilts about x and z move a step */
    Uint16 sway_speed_x;
    Uint16 sway_speed_z;
    /* the phases of its tilts about x and z, whose sines set the tilts */
    Sint16 sway_x;
    Sint16 sway_z;
    Float top; /* the height it bobs about */
};

#define DRIFT(work) ((struct drift_vars *)&(work)->vars)
/* how far a rising drift's phase in work->timer moves a step */
#define RISE_PHASE_STEP 0x800

/* a big leaf, or a bit of board, twig or leaf, that the wind blows.
 * work->timer holds its steps left, work->scl.x its scale and work->scl.y its
 * upward speed */
struct debris_vars {
    /* its turn a step about x, and about z */
    Uint16 spin_x;
    Uint16 spin_z;
    Uint8 unk_04[4];
    NJS_MODEL *model;
};

#define DEBRIS(work) ((struct debris_vars *)&(work)->vars)

/* a gust's steps left, in the particle's vars */
struct gust_vars {
    Sint32 steps;
};

#define GUST_LEFT(particle) (((struct gust_vars *)&(particle)->vars)->steps)

enum windy_mode { WINDY_INIT, WINDY_STILL, WINDY_BLOWING, WINDY_GONE };
enum cave_mode { CAVE_INIT, CAVE_BLOWING, CAVE_GONE };
enum leaf_mode { LEAF_INIT, LEAF_RISING, LEAF_FALLING };
enum debris_mode { DEBRIS_INIT, DEBRIS_FLYING };

/* a drift's modes. once the Wind Stone is set, a drift that has started blows
 * away, and one yet to start is gone */
enum drift_mode {
    DRIFT_INIT,
    DRIFT_RISING,
    DRIFT_FALLING,
    DRIFT_AWAY,
    DRIFT_GONE
};

static void windy_entrance_destroy(struct task *task);
static void windy_entrance_draw(struct task *task);
static Bool windy_stone_set(void);
static void windy_leaf_2_draw(struct task *task);
static void windy_board_destroy(struct task *task);
static void windy_board_draw(struct task *task);
static void windy_leaf_1_destroy(struct task *task);
static void windy_leaf_1_draw(struct task *task);
static void windy_gust_step(struct particle *particle,
                            struct particle_pool *pool);
static void windy_gust_create(struct task_work *work, Float radius);
static void windy_debris_step(struct task *task);
static void windy_debris_draw(struct task *task);
static void windy_debris_leaves(Float x, Float y, Float z, Float radius);
static void windy_debris_scatter(Float x, Float y, Float z, Float radius);
static void windy_entrance_open(void);

/* the work of the whirlwind that runs, or null. another whirlwind waits in
 * WINDY_INIT until that one is destroyed */
static struct task_work *windy_entrance_work = NULL;

/* the objects whose models the wind blows about. windy_debris_scatter picks one
 * of the first three at random, and windy_debris_leaves names the fourth
 * itself */
static NJS_OBJECT *windy_debris_objects[4] = {
    windy_board_object,
    &windy_twig_object,
    windy_leaf_1_object,
    windy_leaf_2_object,
};

/* the whirlwind stands still until the Wind Stone is set. a still whirlwind
 * throws up a leaf every 8 steps, and lifts player 0 within STILL_HEIGHT. once
 * the stone is set, it opens the character's way into Windy Valley and blows.
 * a blowing whirlwind lifts player 0 within BLOWING_HEIGHT, and sends player 0
 * into Windy Valley: Tails to act 2, and the others to act 0 */
void windy_entrance_step(struct task *task)
{
    struct task_work *work = task->work;
    Float lift;
    Float height;

    if (placement_try_leave(task)) {
        return;
    }
    switch (work->mode) {
    case WINDY_INIT:
        if (windy_entrance_work != NULL) {
            return;
        }
        windy_entrance_work = work;
        STEPS(work) = 0;
        work->ang.y = 0;
        task->destructor = windy_entrance_destroy;
        task->draw = windy_entrance_draw;
        work->mode = WINDY_STILL;
        break;
    case WINDY_STILL:
        if (!(STEPS(work) & 7)) {
            struct task *leaf = task_create(TASK_WORK, 3, windy_leaf_2_step);

            if (leaf != NULL) {
                struct task_work *leaf_work = leaf->work;

                leaf_work->pos.x =
                    work->pos.x +
                    (((rand() * RAND_SCALE) - 0.5F) * WIND_RADIUS);
                leaf_work->pos.y = work->pos.y;
                leaf_work->pos.z =
                    work->pos.z +
                    (((rand() * RAND_SCALE) - 0.5F) * WIND_RADIUS);
                LEAF(leaf_work)->top = work->pos.y + LEAF_RISE;
            }
        }
        work->ang.y += WHIRL_PHASE_STEP;
        UPDRAFT(work)->lift += (njSin(work->ang.y) + 1.0F) * WHIRL_BUILD;
        if (UPDRAFT(work)->lift > WHIRL_LIFT) {
            UPDRAFT(work)->lift = WHIRL_LIFT;
        }
        lift = UPDRAFT(work)->lift;
        if (player_in_cylinder(&work->pos, WIND_RADIUS, STILL_HEIGHT) == 1) {
            player_updraft(0, 0.0F, lift, 0.0F);
        }
        /* @bug logic: player_in_cylinder returns 1 for player 0 and
         * otherwise 0, never 2, so player 1 is never lifted (CMP/EQ #2,R0 at
         * 0x0c90ffc2) */
        if (player_in_cylinder(&work->pos, WIND_RADIUS, STILL_HEIGHT) == 2) {
            player_updraft(1, 0.0F, lift, 0.0F);
        }
        if (windy_stone_set()) {
            windy_entrance_open();
            work->mode = WINDY_BLOWING;
        }
        break;
    case WINDY_BLOWING:
        windy_debris_leaves(work->pos.x, work->pos.y - DEBRIS_DROP, work->pos.z,
                            BIG_LEAF_RADIUS);
        if (!(STEPS(work) & 1)) {
            windy_gust_create(work, WIND_RADIUS);
        }
        if (!(STEPS(work) & 3)) {
            windy_debris_scatter(work->pos.x, work->pos.y - DEBRIS_DROP,
                                 work->pos.z, WIND_RADIUS);
        }
        if (player_in_cylinder(&work->pos, WIND_RADIUS, BLOWING_HEIGHT)) {
            height = player_work[0]->pos.y - work->pos.y;
            /* @hack spelling: lift assigned in the test, which loads LIFT_LEAST
             * before it works out lift (FMOV.S @R0,FR6 at 0x0c9100a0);
             * assigned ahead of it, 13 of 283 words differ */
            if ((lift = UPDRAFT(work)->lift - (height * LIFT_FALL)) <
                LIFT_LEAST) {
                lift = LIFT_LEAST;
            }
            player_updraft(0, 0.0F, lift, 0.0F);
            if (TASK_TIMER(work)++ == RIDE_STEPS) {
                if (get_character() == CHARACTER_TAILS) {
                    story_go_to(STAGE_WINDY, 2);
                } else {
                    story_go_to(STAGE_WINDY, 0);
                }
            }
        } else {
            TASK_TIMER(work) = 0;
        }
        break;
    case WINDY_GONE:
        placement_kill(task);
        return;
    }
    STEPS(work)++;
}

static void windy_entrance_destroy(struct task *task)
{
    windy_entrance_work = NULL;
}

static void windy_entrance_draw(struct task *task)
{
}

/* TRUE while the Wind Stone sits in the current character's lock, else FALSE.
 * only Sonic, Tails and E-102 have a lock */
static Bool windy_stone_set(void)
{
    switch (get_character()) {
    case CHARACTER_SONIC:
        return event_flags[FLAG_SONIC_MR_WINDY_STONE];
    case CHARACTER_TAILS:
        return event_flags[FLAG_TAILS_MR_WINDY_STONE];
    /* @hack redundant: the empty cases for Knuckles, Amy and Big, which keep
     * the image's CMP/EQ #3, #5 and #7 (0x0c910194 to 0x0c9101a2); without
     * them, 34 of 36 words differ */
    case CHARACTER_KNUCKLES:
        break;
    case CHARACTER_AMY:
        break;
    case CHARACTER_E102:
        return event_flags[FLAG_E102_MR_WINDY_STONE];
    case CHARACTER_BIG:
        break;
    }
    return FALSE;
}

/* the updraft up from the cave. it lifts player 0 within BLOWING_HEIGHT, and
 * sends player 0 up to the island act. while the set editor holds the task, the
 * step does nothing */
void windy_cave_up_step(struct task *task)
{
    struct task_work *work = task->work;
    Float lift;
    Float height;

    if (placement_try_leave(task)) {
        return;
    }
    if (placement_editing(task)) {
        return;
    }
    switch (work->mode) {
    case CAVE_INIT:
        UPDRAFT(work)->lift = CAVE_LIFT;
        work->mode = CAVE_BLOWING;
        break;
    case CAVE_BLOWING:
        STEPS(work)++;
        windy_debris_leaves(work->pos.x, work->pos.y - DEBRIS_DROP, work->pos.z,
                            BIG_LEAF_RADIUS);
        if (!(STEPS(work) & 1)) {
            windy_gust_create(work, WIND_RADIUS);
        }
        if (!(STEPS(work) & 3)) {
            windy_debris_scatter(work->pos.x, work->pos.y - DEBRIS_DROP,
                                 work->pos.z, WIND_RADIUS);
        }
        if (player_in_cylinder(&work->pos, WIND_RADIUS, BLOWING_HEIGHT)) {
            height = player_work[0]->pos.y - work->pos.y;
            /* @hack spelling: lift assigned in the test, which loads LIFT_LEAST
             * before it works out lift (FMOV.S @R0,FR4 at 0x0c910294);
             * assigned ahead of it, 9 of 135 words and 2 of 37 other words
             * differ */
            if ((lift = UPDRAFT(work)->lift - (height * LIFT_FALL)) <
                LIFT_LEAST) {
                lift = LIFT_LEAST;
            }
            player_updraft(0, 0.0F, lift, 0.0F);
            if (TASK_TIMER(work)++ == RIDE_STEPS) {
                stage_set_entrance(CAVE_TOP_ENTRANCE);
                story_go_to(STAGE_MYSTIC_RUINS, MYSTIC_RUINS_ISLAND);
            }
        } else {
            TASK_TIMER(work) = 0;
        }
        break;
    case CAVE_GONE:
        placement_kill(task);
        break;
    }
}

/* the updraft down from the island act to the cave's mouth. it lifts player 0
 * within BLOWING_HEIGHT, and sends player 0 down to the station act. from then
 * on its lift weakens by CAVE_FADE a step, until player 0 leaves it and it
 * lifts by CAVE_LIFT again. while the set editor holds the task, the step does
 * nothing */
void windy_cave_down_step(struct task *task)
{
    struct task_work *work = task->work;
    Float lift;
    Float height;

    if (placement_try_leave(task)) {
        return;
    }
    if (placement_editing(task)) {
        return;
    }
    switch (work->mode) {
    case CAVE_INIT:
        UPDRAFT(work)->lift = CAVE_LIFT;
        work->mode = CAVE_BLOWING;
        break;
    case CAVE_BLOWING:
        if (player_in_cylinder(&work->pos, WIND_RADIUS, BLOWING_HEIGHT)) {
            height = player_work[0]->pos.y - work->pos.y;
            /* @hack spelling: lift assigned in the test, which loads LIFT_LEAST
             * before it works out lift (FMOV.S @R0,FR6 at 0x0c910398);
             * assigned ahead of it, 15 of 99 words differ */
            if ((lift = UPDRAFT(work)->lift - (height * LIFT_FALL)) <
                LIFT_LEAST) {
                lift = LIFT_LEAST;
            }
            player_updraft(0, 0.0F, lift, 0.0F);
            if (TASK_TIMER(work) > RIDE_STEPS) {
                UPDRAFT(work)->lift -= CAVE_FADE;
            }
            if (TASK_TIMER(work)++ == RIDE_STEPS) {
                stage_set_entrance(CAVE_MOUTH_ENTRANCE);
                story_go_to(STAGE_MYSTIC_RUINS, MYSTIC_RUINS_STATION);
            }
        } else {
            UPDRAFT(work)->lift = CAVE_LIFT;
            TASK_TIMER(work) = 0;
        }
        break;
    }
}

/* a leaf that tumbles as it bobs about its top, until its LEAF_STEPS steps run
 * out */
void windy_leaf_2_step(struct task *task)
{
    struct task_work *work = task->work;
    Float random;

    switch (work->mode) {
    case LEAF_INIT:
        TASK_TIMER(work) = LEAF_STEPS;
        random = rand() * RAND_SCALE;
        work->scl.x = random - 0.5F;
        work->scl.z = (rand() * RAND_SCALE) - 0.5F;
        work->ang.y = rand() * 2.0F;
        work->ang.x = rand() * 2.0F;
        LEAF(work)->spin_x = ((rand() * RAND_SCALE) - 0.5F) * 6553.6F;
        LEAF(work)->spin_y = ((rand() * RAND_SCALE) - 0.5F) * 6553.6F;
        work->tick = (5.0F + (rand() * (4.0F * RAND_SCALE))) * 16.0F;
        LEAF(work)->kind = (int)(random * 10.0F) % 3;
        work->mode = LEAF_RISING;
        work->ang.z = 0;
        work->scl.y = 0.0F;
        break;
    case LEAF_RISING:
        work->scl.x += njSin(work->ang.y) * 0.05F;
        work->scl.z += njCos(work->ang.y) * 0.05F;
        work->scl.y += (njSin(work->ang.x * 2) + 0.5F) * 0.1F;
        if (work->pos.y > LEAF(work)->top) {
            work->mode = LEAF_FALLING;
        }
        break;
    case LEAF_FALLING:
        /* @hack spelling: scl.y stepped in the test, which compares the
         * difference still in FR3 and stores it in the slot of its BF/S
         * (0x0c910614); stepped ahead of it, 74 of 249 words differ */
        if ((work->scl.y -= 0.05F) < -0.5F) {
            work->scl.y = -0.5F;
        }
        if (work->pos.y < LEAF(work)->top) {
            work->mode = LEAF_RISING;
        }
        break;
    }
    work->ang.x += LEAF(work)->spin_x;
    work->ang.y += LEAF(work)->spin_y;
    work->pos.x += work->scl.x;
    work->pos.y += work->scl.y;
    work->pos.z += work->scl.z;
    windy_leaf_2_draw(task);
    if (TASK_TIMER(work)-- == 0) {
        task_kill(task);
    }
}

static void windy_leaf_2_draw(struct task *task)
{
    struct task_work *work = task->work;
    Float scale = work->tick * (1.0F / 16.0F);

    texture_set_stage();
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    njRotateZXY(NULL, work->ang.x, work->ang.y, work->ang.z);
    njScale(NULL, scale, scale, scale);
    draw_model(windy_leaf_2_object->model);
    njPopMatrix(1);
}

/* a board that bobs about its top on the wind, tilting as it goes. once the
 * Wind Stone is set, a board that has started blows away upward, and one yet to
 * start never appears. while the set editor holds the task, the step only
 * draws it */
void windy_board_step(struct task *task)
{
    struct task_work *work = task->work;

    if (placement_try_leave(task)) {
        return;
    }
    if (placement_editing(task)) {
        windy_board_draw(task);
        return;
    }
    if (windy_stone_set()) {
        switch (work->mode) {
        case DRIFT_INIT:
            work->mode = DRIFT_GONE;
            break;
        case DRIFT_RISING:
        case DRIFT_FALLING:
            work->mode = DRIFT_AWAY;
            break;
        }
    }
    switch (work->mode) {
    case DRIFT_INIT:
        DRIFT(work)->sway_speed_x = ((rand() * RAND_SCALE) - 0.5F) * 4096.0F;
        DRIFT(work)->sway_speed_z = ((rand() * RAND_SCALE) - 0.5F) * 4096.0F;
        DRIFT(work)->sway_x = 0;
        DRIFT(work)->sway_z = 0;
        TASK_TIMER(work) = rand() * 2.0F;
        DRIFT(work)->top = work->pos.y + work->scl.y;
        task->draw = windy_board_draw;
        task->destructor = windy_board_destroy;
        work->mode = DRIFT_RISING;
        work->scl.y = 0.0F;
        break;
    case DRIFT_RISING:
        TASK_TIMER(work) += RISE_PHASE_STEP;
        work->scl.y += (njSin(TASK_TIMER(work)) + 0.5F) * 0.05F;
        if (work->scl.y > 1.0F) {
            work->scl.y = 1.0F;
        }
        if (work->pos.y > DRIFT(work)->top) {
            work->mode = DRIFT_FALLING;
        }
        DRIFT(work)->sway_x += DRIFT(work)->sway_speed_x;
        DRIFT(work)->sway_z += DRIFT(work)->sway_speed_z;
        break;
    case DRIFT_FALLING:
        /* @hack spelling: scl.y stepped in the test, which compares the
         * difference still in FR2 and stores it in the slot of its BF/S
         * (0x0c9108ce); stepped ahead of it, 32 of 245 words and 6 of 81 other
         * words differ */
        if ((work->scl.y -= 0.08F) < -0.4F) {
            work->scl.y = -0.4F;
        }
        if (work->pos.y < DRIFT(work)->top) {
            work->mode = DRIFT_RISING;
        }
        DRIFT(work)->sway_x -= DRIFT(work)->sway_speed_x;
        DRIFT(work)->sway_z -= DRIFT(work)->sway_speed_z;
        break;
    case DRIFT_AWAY:
        work->scl.y += 0.2F;
        if (work->scl.y > 2.0F) {
            work->scl.y = 2.0F;
        }
        break;
    case DRIFT_GONE:
        return;
    }
    work->ang.x = njSin(DRIFT(work)->sway_x) * 4096.0F;
    work->ang.z = njSin(DRIFT(work)->sway_z) * 4096.0F;
    work->pos.y += work->scl.y;
    windy_board_draw(task);
}

static void windy_board_destroy(struct task *task)
{
}

static void windy_board_draw(struct task *task)
{
    struct task_work *work = task->work;
    Float scale = work->scl.x;
    Angle ang;

    texture_set_stage();
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    TURN_VIA(njRotateY, work->ang.y, ang);
    TURN_VIA(njRotateZ, work->ang.z, ang);
    TURN_VIA(njRotateX, work->ang.x, ang);
    njScale(NULL, scale, scale, scale);
    draw_model(windy_board_object->model);
    njPopMatrix(1);
}

/* a leaf that bobs on the wind as the board does */
void windy_leaf_1_step(struct task *task)
{
    struct task_work *work = task->work;

    if (placement_try_leave(task)) {
        return;
    }
    if (placement_editing(task)) {
        windy_leaf_1_draw(task);
        return;
    }
    if (windy_stone_set()) {
        switch (work->mode) {
        case DRIFT_INIT:
            work->mode = DRIFT_GONE;
            break;
        case DRIFT_RISING:
        case DRIFT_FALLING:
            work->mode = DRIFT_AWAY;
            break;
        }
    }
    switch (work->mode) {
    case DRIFT_INIT:
        DRIFT(work)->sway_speed_x = ((rand() * RAND_SCALE) - 0.5F) * 4096.0F;
        DRIFT(work)->sway_speed_z = ((rand() * RAND_SCALE) - 0.5F) * 4096.0F;
        DRIFT(work)->sway_x = 0;
        DRIFT(work)->sway_z = 0;
        TASK_TIMER(work) = rand() * 2.0F;
        DRIFT(work)->top = work->pos.y + work->scl.y;
        task->draw = windy_leaf_1_draw;
        task->destructor = windy_leaf_1_destroy;
        work->mode = DRIFT_RISING;
        work->scl.y = 0.0F;
        break;
    case DRIFT_RISING:
        TASK_TIMER(work) += RISE_PHASE_STEP;
        work->scl.y += (njSin(TASK_TIMER(work)) + 0.5F) * 0.05F;
        if (work->scl.y > 1.0F) {
            work->scl.y = 1.0F;
        }
        if (work->pos.y > DRIFT(work)->top) {
            work->mode = DRIFT_FALLING;
        }
        DRIFT(work)->sway_x += DRIFT(work)->sway_speed_x;
        DRIFT(work)->sway_z += DRIFT(work)->sway_speed_z;
        break;
    case DRIFT_FALLING:
        /* @hack spelling: scl.y stepped in the test, which compares the
         * difference still in FR2 and stores it in the slot of its BF/S
         * (0x0c910bee); stepped ahead of it, 32 of 245 words and 6 of 95 other
         * words differ */
        if ((work->scl.y -= 0.08F) < -0.4F) {
            work->scl.y = -0.4F;
        }
        if (work->pos.y < DRIFT(work)->top) {
            work->mode = DRIFT_RISING;
        }
        DRIFT(work)->sway_x -= DRIFT(work)->sway_speed_x;
        DRIFT(work)->sway_z -= DRIFT(work)->sway_speed_z;
        break;
    case DRIFT_AWAY:
        work->scl.y += 0.2F;
        if (work->scl.y > 2.0F) {
            work->scl.y = 2.0F;
        }
        break;
    case DRIFT_GONE:
        return;
    }
    work->ang.x = njSin(DRIFT(work)->sway_x) * 4096.0F;
    work->ang.z = njSin(DRIFT(work)->sway_z) * 4096.0F;
    work->pos.y += work->scl.y;
    windy_leaf_1_draw(task);
}

static void windy_leaf_1_destroy(struct task *task)
{
}

static void windy_leaf_1_draw(struct task *task)
{
    struct task_work *work = task->work;
    Float scale = work->scl.x;
    Angle ang;

    texture_set_stage();
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    TURN_VIA(njRotateY, work->ang.y, ang);
    TURN_VIA(njRotateZ, work->ang.z, ang);
    TURN_VIA(njRotateX, work->ang.x, ang);
    njScale(NULL, scale, scale, scale);
    draw_model(windy_leaf_1_object->model);
    njPopMatrix(1);
}

static void windy_gust_step(struct particle *particle,
                            struct particle_pool *pool)
{
    if (GUST_LEFT(particle) == 0) {
        particle_kill(particle);
    } else {
        GUST_LEFT(particle)--;
        if (particle->ang > 0) {
            particle->ang += 64;
        } else {
            particle->ang -= 64;
        }
        particle->pos.y += particle->speed.y;
    }
}

/* create a faint gust at the wind's height, at a random point within radius of
 * its x and of its z. the gust rises until its GUST_STEPS steps run out */
static void windy_gust_create(struct task_work *work, Float radius)
{
    struct particle *gust =
        particle_alloc(dust_glow_pool.pool, windy_gust_step);

    if (gust != NULL) {
        gust->pos.x =
            work->pos.x + (((rand() * RAND_SCALE) - 0.5F) * radius * 2.0F);
        gust->pos.y = work->pos.y;
        gust->pos.z =
            work->pos.z + (((rand() * RAND_SCALE) - 0.5F) * radius * 2.0F);
        gust->frame = rand() * (4.0F * RAND_SCALE);
        gust->speed.y = 4.0F + (rand() * RAND_SCALE);
        gust->scale = 0.5F + (rand() * (0.5F * RAND_SCALE));
        gust->ang = rand() * 2.0F;
        gust->depth = -20.0F;
        gust->color.a = 0.15F;
        gust->color.r = 1.0F;
        gust->color.g = 1.0F;
        gust->color.b = 1.0F;
        GUST_LEFT(gust) = GUST_STEPS;
    }
}

static void windy_debris_step(struct task *task)
{
    struct task_work *work = task->work;

    switch (work->mode) {
    case DEBRIS_INIT:
        DEBRIS(work)->spin_x = ((rand() * RAND_SCALE) - 0.5F) * 2048.0F;
        DEBRIS(work)->spin_z = ((rand() * RAND_SCALE) - 0.5F) * 2048.0F;
        work->ang.x = rand() * 2.0F;
        work->ang.y = rand() * 2.0F;
        task->draw = windy_debris_draw;
        work->mode = DEBRIS_FLYING;
        break;
    case DEBRIS_FLYING:
        if (TASK_TIMER(work) == 0) {
            task_kill(task);
            return;
        } else {
            TASK_TIMER(work)--;
            work->ang.x += DEBRIS(work)->spin_x;
            work->ang.z += DEBRIS(work)->spin_z;
            work->pos.y += work->scl.y;
        }
        break;
    }
    windy_debris_draw(task);
}

static void windy_debris_draw(struct task *task)
{
    struct task_work *work = task->work;
    Float scale = work->scl.x;

    texture_set_stage();
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    njRotateXYZ(NULL, work->ang.x, work->ang.y, work->ang.z);
    njScale(NULL, scale, scale, scale);
    draw_model(DEBRIS(work)->model);
    njPopMatrix(1);
}

/* create a big leaf at height y, at a random point within radius of x and of z.
 * the leaf rises and spins until its BIG_LEAF_STEPS steps run out */
static void windy_debris_leaves(Float x, Float y, Float z, Float radius)
{
    struct task *task = task_create(TASK_WORK, 6, windy_debris_step);

    if (task != NULL) {
        struct task_work *work = task->work;

        DEBRIS(work)->model = windy_leaf_2_object->model;
        TASK_TIMER(work) = BIG_LEAF_STEPS;
        work->scl.x = 4.0F + (rand() * (4.0F * RAND_SCALE));
        work->scl.y = 1.5F + (rand() * (0.5F * RAND_SCALE));
        work->pos.x = x + (((rand() * RAND_SCALE) - 0.5F) * radius * 2.0F);
        work->pos.y = y;
        work->pos.z = z + (((rand() * RAND_SCALE) - 0.5F) * radius * 2.0F);
    }
}

/* create a bit of board, twig or leaf at height y, at a random point within
 * radius of x and of z. the bit rises and spins until its BIT_STEPS steps run
 * out */
static void windy_debris_scatter(Float x, Float y, Float z, Float radius)
{
    struct task *task = task_create(TASK_WORK, 6, windy_debris_step);

    if (task != NULL) {
        struct task_work *work = task->work;

        DEBRIS(work)->model =
            windy_debris_objects[(int)(rand() * (3.0F * RAND_SCALE))]->model;
        TASK_TIMER(work) = BIT_STEPS;
        work->scl.x = 2.0F + (rand() * (2.0F * RAND_SCALE));
        work->scl.y = 1.0F + (rand() * (0.5F * RAND_SCALE));
        work->pos.x = x + (((rand() * RAND_SCALE) - 0.5F) * radius * 2.0F);
        work->pos.y = y;
        work->pos.z = z + (((rand() * RAND_SCALE) - 0.5F) * radius * 2.0F);
    }
}

static void windy_entrance_open(void)
{
    switch (get_character()) {
    case CHARACTER_SONIC:
        event_flag_set(FLAG_SONIC_MR_ENTRANCE_WINDY);
        break;
    case CHARACTER_TAILS:
        event_flag_set(FLAG_TAILS_MR_ENTRANCE_WINDY);
        break;
    case CHARACTER_E102:
        event_flag_set(FLAG_E102_MR_ENTRANCE_WINDY);
        break;
    }
}
