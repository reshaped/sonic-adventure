/* Chaos 4's task: his setup and his modes, his swim between the pads and
 * his choice of attacks, his move, his splashes and his draw */
#include <mathf.h>
#include <ninja.h>
#include <sg_xpt.h>
#include <shinobi.h>
#include <stdlib.h>

#include "b_chaos4/boss.h"
#include "b_chaos4/camera.h"
#include "b_chaos4/column.h"
#include "b_chaos4/hit.h"
#include "b_chaos4/leap.h"
#include "b_chaos4/pads.h"
#include "b_chaos4/ribbon.h"
#include "b_chaos4/sickle.h"
#include "b_chaos4/split.h"
#include "b_chaos4/surface.h"
#include "b_chaos4/swamp.h"
#include "b_chaos4/sweep.h"
#include "b_chaos4/textures.h"
#include "b_chaos4/util.h"
#include "camera/camera.h"
#include "chaos/chaos.h"
#include "chaos/effect.h"
#include "chaos/ribbon.h"
#include "chaos/steer.h"
#include "collision/collision.h"
#include "draw/draw.h"
#include "draw/motion.h"
#include "draw/tree.h"
#include "effect/brain.h"
#include "effect/flare.h"
#include "hud/hud.h"
#include "hud/life_gauge.h"
#include "player/player.h"
#include "sound/sound.h"
#include "stage/stage.h"
#include "system/frame.h"
#include "system/pad.h"
#include "system/task.h"
#include "system/util.h"
#include "vmu/vibration.h"

/* a row of attack_choices holds up to three choices, and ATTACK_END, which
 * is no mode, fills the rest */
#define ATTACK_END 12
#define SE_HIT 0x16b
/* the sound held while the intro's camera circles him */
#define SE_ORBIT 0x170

/* a bit of his record's move that swim sets. nothing reads it */
#define MOVE_UNUSED 0x8

#define BOTTOM_Y (-30.0F) /* the bottom he can't sink below */

/* what MOVE_FALLS's gravity takes from his speed's y a step. MOVE_TOWARD
 * keeps SPEED_KEPT of his speed a step and adds PULL of his gap to the
 * target, or PULL_LAST times that at LAST_HEALTH or less */
#define GRAVITY (9.8F / 60.0F)
#define SPEED_KEPT 0.85F
#define PULL 0.003F
#define PULL_LAST 1.5F

/* pace_motion sets his motion's rate, which scales a MOTION_PACED action
 * such as ACTION_REST, to the length of his speed, but no lower than
 * LEAST_RATE */
#define LEAST_RATE 0.6F
/* while he moves faster than WAKE_SPEED in x and z, he leaves a wake on a
 * step by a chance of WAKE_CHANCE */
#define WAKE_SPEED 0.6F
#define WAKE_CHANCE 0.2F

#define HALF_TURN NJM_DEG_ANG(180)

/* the bit of his struct chaos's figures for his body, which chaos_setup
 * sets */
#define FIGURE_BODY 0x1

/* swim's submodes. SWIM_AIM aims a leg at a pad, and SWIM_LEG swims it */
enum swim_mode { SWIM_START, SWIM_AIM, SWIM_LEG };

/* his swim's legs to an attack, and a leg's steps. at LAST_HEALTH or less
 * a leg takes LEG_STEPS / LEG_PACE_LAST steps */
#define LEGS 3
#define LEG_STEPS 90
#define LEG_PACE_LAST 0.75F
/* a leg's start takes a random amount up to DIVE_MOST from his speed's y */
#define DIVE_MOST 10.0F

enum intro_mode {
    INTRO_START,
    INTRO_FLARE,
    INTRO_SINK,
    INTRO_ORBIT,
    INTRO_ORBITING,
    INTRO_WAIT,
    INTRO_END
};

/* the intro's sink a step, the depth it ends at, and the steps its wait
 * and its orbit count down from */
#define INTRO_SINK_SPEED 1.1F
#define INTRO_DEPTH (-5.0F)
#define INTRO_WAIT_TIME 77
#define INTRO_ORBIT_TIME 150

/* the calls of choose_attack that send him to BOSS_SURFACE: every
 * SURFACE_COUNT, or every SURFACE_COUNT_LAST at LAST_HEALTH or less */
#define SURFACE_COUNT 2
#define SURFACE_COUNT_LAST 4

/* attack_choices' rows, by his health: above HEALTH_MIDDLE, above
 * HEALTH_LOW, then HEALTH_LOW or less. and by player 0's distance from him
 * in x and z: below DISTANCE_NEAR, below DISTANCE_MIDDLE, then farther */
#define HEALTH_MIDDLE 4.0F
#define HEALTH_LOW 2.0F
#define DISTANCE_NEAR 50.0F
#define DISTANCE_MIDDLE 100.0F

static const NJS_POINT3 start_pos = {0.0F, 100.0F, 0.0F};
/* a choice of attack: its mode, and the bound of a random roll from 0 to 1
 * that takes it. a row's bounds rise to 1. choose_attack takes the first
 * choice whose bound the roll doesn't pass */
struct attack_choice {
    Uint8 mode;
    Float bound;
};

static const struct attack_choice attack_choices[3][3][3] = {
    {{{BOSS_SWEEP, 1.0F}, {ATTACK_END, 0.0F}, {ATTACK_END, 0.0F}},
     {{BOSS_SICKLE, 1.0F}, {ATTACK_END, 0.0F}, {ATTACK_END, 0.0F}},
     {{BOSS_SICKLE, 1.0F}, {ATTACK_END, 0.0F}, {ATTACK_END, 0.0F}}},
    {{{BOSS_SWEEP, 1.0F}, {ATTACK_END, 0.0F}, {ATTACK_END, 0.0F}},
     {{BOSS_SICKLE, 0.75F}, {BOSS_SPLIT, 1.0F}, {ATTACK_END, 0.0F}},
     {{BOSS_SICKLE, 0.75F}, {BOSS_SPLIT, 1.0F}, {ATTACK_END, 0.0F}}},
    {{{BOSS_SWEEP, 1.0F}, {ATTACK_END, 0.0F}, {ATTACK_END, 0.0F}},
     {{BOSS_SICKLE, 0.4F}, {BOSS_SPLIT, 0.8F}, {BOSS_LEAP, 1.0F}},
     {{BOSS_LEAP, 1.0F}, {ATTACK_END, 0.0F}, {ATTACK_END, 0.0F}}}};

/* the brain's offset from boss point 8, along -z in the camera's frame */
static const NJS_VECTOR brain_offset = {0.0F, 0.0F, -1.0F};
/* the drawn arm's shoulder turn. draw_arm turns z by [1] (-1440), x by [0]
 * (8532) and y by [2] (-1080). sweep.c's shoulder_ang holds the same three
 * numbers as x, y and z */
static const Angle shoulder_angles[3] = {8532, -1440, -1080};

static const NJS_POINT3 flare_pos = {-179.0F, 300.0F, -240.0F};

/* his tables in the module's generated data: models and motions, and the
 * welds, shapes and marks that go with them */
extern struct weld chaos4_welds[];
extern NJS_OBJECT chaos4_blob_object;
extern struct motion_entry chaos4_actions[];
extern NJS_OBJECT *chaos4_alpha_objects[];
extern struct collision_shape chaos4_shapes[];
extern char chaos4_shape_rows[2][CHAOS4_SHAPES];
extern struct boss_mark chaos4_marks[];
/* the speed take_hit throws player 0 back with, in the frame he faces until the
 * first hit turns it into the world in place */
extern NJS_VECTOR hit_recoil;

NJS_POINT3 *chaos4_pos;
NJS_POINT3 *chaos4_scale;
/* set until the intro's splash, as he first sinks into the swamp */
static int splash_pending;
static int intro_timer; /* the steps left in the intro's wait or orbit */
/* the intro's flare, whose task the intro kills once he has sunk */
static struct task *intro_flare;

static Uint8 choose_pad(struct chaos *extra);
static void choose_attack(struct task *task);
static void step(struct task *task);
static void splash(struct chaos4 *record);
static void draw(struct task_work *work, struct chaos *state);
static void draw_arm(struct task_work *work);
static void set_material(NJS_OBJECT *object);
static void intro(struct task *task);

void chaos4_set_shapes(struct task_work *work, int row)
{
    int i;
    for (i = 0; i < CHAOS4_SHAPES; i++) {
        if (chaos4_shape_rows[row][i]) {
            work->collision->shapes[i].flags &= ~SHAPE_OFF;
        } else {
            work->collision->shapes[i].flags |= SHAPE_OFF;
        }
    }
}

static void place_shapes(struct collision_work *collision)
{
    collision->shapes[0].center = boss_points[0].pos;
    collision->shapes[1].center = boss_points[1].pos;
    collision->shapes[3].center = boss_points[3].pos;
    collision->shapes[2].center = boss_points[2].pos;
    collision->shapes[5].center = boss_points[5].pos;
    collision->shapes[4].center = boss_points[4].pos;
    collision->shapes[6].center = boss_points[6].pos;
}

static void pace_motion(struct chaos *extra, struct chaos4 *record)
{
    extra->motion.speed =
        njSqrt(SQUARE(record->speed.x) + SQUARE(record->speed.y) +
               SQUARE(record->speed.z));
    if (extra->motion.speed < LEAST_RATE) {
        extra->motion.speed = LEAST_RATE;
    }
}

static void move(struct task_work *work, struct task_motion *motion,
                 struct chaos *extra)
{
    struct chaos4 *record = CHAOS4_VARS(work)->record;
    /* @hack spelling: goal, a second name for extra, which keeps extra on the
     * stack for pace_motion and reads the target through R4 (MOV.L R6,@R15
     * at 0x0c9004cc, MOV R6,R4 at 0x0c9004d8); through extra, 64 of 249
     * words differ */
    struct chaos *goal = extra;
    NJS_VECTOR offset;

    if ((record->move & MOVE_FALLS) && work->pos.y >= SWAMP_Y) {
        record->speed.y -= GRAVITY;
    }
    /* @hack spelling: gotos out of the switch, which leave move without a pool
     * of its own. SHC places its constants in swim's first pool, after
     * swim's switch (MOVA @(0x0c9006e0),R0 at 0x0c9004da). as a switch
     * whose cases fall through, move takes a pool of its own, and 209 of
     * 249 words differ */
    switch (record->move & MOVE_HOW) {
    case MOVE_STILL:
        goto finish_movement;
    case MOVE_DRIFT:
        goto apply_velocity;
    case MOVE_TOWARD_ACROSS:
    case MOVE_TOWARD:
        break;
    default:
        goto finish_movement;
    }
    record->speed.x = record->speed.x * SPEED_KEPT;
    record->speed.y = record->speed.y * SPEED_KEPT;
    record->speed.z = record->speed.z * SPEED_KEPT;
    /* @hack spelling: offset's members through its address, which keeps offset
     * on the stack, each store through a fresh copy of R15 (0x0c90051e to
     * 0x0c900616). as members, move runs 96 bytes short, and 218 of 249
     * words differ */
    (&offset)->x = goal->target.x - work->pos.x;
    (&offset)->y = goal->target.y - work->pos.y;
    (&offset)->z = goal->target.z - work->pos.z;
    if ((record->move & MOVE_HOW) == MOVE_TOWARD_ACROSS) {
        offset.y = 0.0F;
    }
    if (life_gauge_health <= LAST_HEALTH) {
        (&offset)->x = (&offset)->x * (PULL * PULL_LAST);
        (&offset)->y = (&offset)->y * (PULL * PULL_LAST);
        (&offset)->z = (&offset)->z * (PULL * PULL_LAST);
    } else {
        (&offset)->x = (&offset)->x * PULL;
        (&offset)->y = (&offset)->y * PULL;
        (&offset)->z = (&offset)->z * PULL;
    }
    record->speed.x = record->speed.x + (&offset)->x;
    record->speed.y = record->speed.y + (&offset)->y;
    record->speed.z = record->speed.z + (&offset)->z;
apply_velocity:
    work->pos.x = work->pos.x + record->speed.x;
    work->pos.y = work->pos.y + record->speed.y;
    work->pos.z = work->pos.z + record->speed.z;
finish_movement:
    if (work->pos.y < BOTTOM_Y) {
        work->pos.y = BOTTOM_Y;
        record->speed.y = 0.0F;
    }
    pace_motion(extra, record);
    chaos4_turn(work);
    if (njSqrt(SQUARE(record->speed.x) + SQUARE(record->speed.z)) >
        WAKE_SPEED) {
        if (rand() * RAND_SCALE < WAKE_CHANCE) {
            wake_create(&work->pos, &work->ang);
        }
    }
}

static void swim(struct task *task)
{
    /* @hack redundant: register on work and extra, which keeps work in
     * R14, extra in R13 and record in R12 (0x0c9006be to 0x0c9006c8). as
     * plain locals, 17 of 199 words differ */
    register struct task_work *work = task->work;
    register struct chaos *extra = task->extra;
    struct chaos4 *record = CHAOS4_VARS(work)->record;
    NJS_POINT3 destination;
    Uint8 pad;
    Angle heading;

    switch (work->submode) {
    case SWIM_START:
        chaos4_set_shapes(work, SHAPES_ON);
        extra->motion.wanted = ACTION_REST;
        work->submode = SWIM_LEG;
        record->move = MOVE_TOWARD | MOVE_UNUSED;
        record->turn = TURN_HEADING;
        /* fall through */
    case SWIM_AIM:
        pad = choose_pad(extra);
        /* @hack spelling: destination's members through its address, which
         * keeps destination on the stack, each store through a fresh copy
         * of R15 (0x0c90074c to 0x0c9007fa). as members, swim runs 32 bytes
         * short, and 171 of 199 words differ */
        (&destination)->x = pad_places[pad].x;
        (&destination)->y = pad_places[pad].y;
        (&destination)->z = pad_places[pad].z;
        record->speed.y -= rand() * (DIVE_MOST * RAND_SCALE);
        if (--record->legs_left <= 0) {
            (&destination)->x = (&destination)->x + work->pos.x;
            (&destination)->y = (&destination)->y + work->pos.y;
            (&destination)->z = (&destination)->z + work->pos.z;
            (&destination)->x = (&destination)->x * 0.5F;
            (&destination)->y = (&destination)->y * 0.5F;
            (&destination)->z = (&destination)->z * 0.5F;
        }
        destination.y = 0.0F;
        extra->target.x = (&destination)->x;
        extra->target.y = (&destination)->y;
        extra->target.z = (&destination)->z;
        heading = atan2f(extra->target.z - work->pos.z,
                         extra->target.x - work->pos.x) *
                  ANGLE_PER_RADIAN;
        record->heading = heading;
        CHAOS4_VARS(work)->leg_timer = LEG_STEPS;
        if (life_gauge_health <= LAST_HEALTH) {
            CHAOS4_VARS(work)->leg_timer /= LEG_PACE_LAST;
        }
        work->submode = SWIM_LEG;
        break;
    case SWIM_LEG:
        if (--CHAOS4_VARS(work)->leg_timer <= 0) {
            if (record->legs_left <= 0) {
                record->legs_left = LEGS;
                choose_attack(task);
                work->submode = MODE_DONE;
            } else {
                work->submode = SWIM_AIM;
            }
        }
        break;
    }
    if (work->submode == MODE_DONE) {
        work->submode = SWIM_START;
    }
}

/* the pad his swim's next leg heads for, as an index into pad_places.
 * while pad_stood_on names a pad, it takes that pad by a chance. the chance
 * is 0.9 at 1 or 2 health, 0.7 at 3 or 4, and 0.6 otherwise. else it takes
 * a pad from 1 to PADS - 1 at random. either way it sets pad_stood_on to
 * PADS */
static Uint8 choose_pad(struct chaos *extra)
{
    Float chance;
    Uint8 requested = pad_stood_on;
    pad_stood_on = PADS;
    if (requested < PADS) {
        switch ((int)extra->health) {
        default:
        case 5:
            chance = 0.6F;
            break;
        case 3:
        case 4:
            chance = 0.7F;
            break;
        case 1:
        case 2:
            chance = 0.9F;
            break;
        }
        if (rand() * RAND_SCALE <= chance) {
            return requested;
        }
    }
    while (!(requested = (Uint8)(rand() * ((Float)PADS * RAND_SCALE)))) {
    }
    return requested;
}

/* choose his mode after a swim. every SURFACE_COUNT calls, or every
 * SURFACE_COUNT_LAST at LAST_HEALTH or less, it is BOSS_SURFACE. while
 * player 0 flies it is BOSS_LEAP. otherwise it is a choice from the row of
 * attack_choices for his health and player 0's distance */
static void choose_attack(struct task *task)
{
    struct chaos *extra = task->extra;
    struct task_work *work = task->work;
    struct chaos4 *record = CHAOS4_VARS(work)->record;
    Float distance;
    Float chance;
    int limit;
    int health_band;
    int distance_band;
    int i;

    limit = extra->health <= LAST_HEALTH ? SURFACE_COUNT_LAST : SURFACE_COUNT;

    /* @hack redundant: a never-run block of tests of the record's draw and
     * move, which keeps the image's order of saved registers, with
     * boss_next_mode's address in R12 (0x0c9009e4); without it, R11 and R12
     * trade, as do R2 and R3, and 14 of 196 words differ */
    if (0) {
        if (record->draw) {
        }
        if (record->move) {
        }
    }

    /* @hack redundant: a goto to the next line, which keeps the attack count in
     * R2 (MOV.B @(R0,R4),R2 at 0x0c9009e6); without it, 5 of 196 words
     * differ */
    goto count_attack;
count_attack:
    if (++record->attack_count >= limit) {
        record->attack_count = 0;
        if (work->mode != BOSS_SURFACE) {
            boss_next_mode = BOSS_SURFACE;
            boss_last_mode = work->mode;
        }
        return;
    }
    if (chaos4_player_flying()) {
        if (work->mode != BOSS_LEAP) {
            boss_next_mode = BOSS_LEAP;
            boss_last_mode = work->mode;
        }
        return;
    }
    if (extra->health <= HEALTH_LOW) {
        health_band = 2;
    } else if (extra->health <= HEALTH_MIDDLE) {
        health_band = 1;
    } else {
        health_band = 0;
    }
    distance = njSqrt(SQUARE(work->pos.x - player_work[0]->pos.x) +
                      SQUARE(work->pos.z - player_work[0]->pos.z));
    if (distance < DISTANCE_NEAR) {
        distance_band = 0;
    } else if (distance < DISTANCE_MIDDLE) {
        distance_band = 1;
    } else {
        distance_band = 2;
    }
    chance = rand() * RAND_SCALE;
    for (i = 0; i < 2; i++) {
        if (attack_choices[health_band][distance_band][i + 1].mode ==
                ATTACK_END ||
            chance <= attack_choices[health_band][distance_band][i].bound) {
            if ((int)attack_choices[health_band][distance_band][i].mode !=
                work->mode) {
                boss_next_mode =
                    attack_choices[health_band][distance_band][i].mode;
                boss_last_mode = work->mode;
            }
            return;
        }
    }

    /* @hack redundant: a never-run block of tests of the row and
     * boss_last_mode, which keeps the row's base and the modes' addresses in
     * the image's registers (0x0c900ae6 to 0x0c900afc); without it, the row's
     * index is built in R8, the function runs 4 bytes long, set_alpha after
     * it moves, and 47 of 196 words differ */
    if (0) {
        if (attack_choices[health_band]) {
        }
        if (boss_last_mode) {
        }
    }

    /* @hack redundant: a goto to the next line, which keeps the last choice's
     * row in R2 and its mode in R3 (0x0c900b40 to 0x0c900b46); without it,
     * 12 of 196 words differ */
    goto last_choice;
last_choice:
    if ((int)attack_choices[health_band][distance_band][2].mode != work->mode) {
        boss_next_mode = attack_choices[health_band][distance_band][2].mode;
        boss_last_mode = work->mode;
    }
}

/* make the first material of each object in objects, a null-ended list,
 * translucent at percent of full alpha */
static void set_alpha(NJS_OBJECT **objects, int percent)
{
    /* @hack redundant: register on alpha, which keeps the attrflags masks
     * in R7, R1 and R13 (0x0c900b86 to 0x0c900b8a). as a plain int, 4 of 43
     * words differ */
    register int alpha = percent * 255 / 100;
    NJS_OBJECT *object;
    for (; (object = *objects) != NULL; objects++) {
        if (object->model) {
            object->model->mats->attrflags |=
                NJD_FLAG_USE_ENV | NJD_FLAG_USE_ALPHA;
            object->model->mats->attrflags &= ~NJD_FLAG_IGNORE_SPECULAR;
            object->model->mats->attrflags |= NJD_FLAG_USE_ALPHA;
            object->model->mats->diffuse.argb.a = alpha;
        }
    }
}

/* in BOSS_SURFACE, when a shape of his was hurt this step: ask for
 * BOSS_HIT, turn his shapes off, and throw player 0 back by hit_recoil */
static void take_hit(struct task_work *work)
{
    if (work->flags & COLLISION_DAMAGED) {
        work->flags &= ~COLLISION_DAMAGED;
        if (work->mode != BOSS_HIT) {
            boss_next_mode = BOSS_HIT;
            boss_last_mode = work->mode;
        }
        work->submode = MODE_DONE;
        chaos4_set_shapes(work, SHAPES_OFF);
        sound_play(SE_HIT, 0, 0, 0);
        vibration_start(0, 11, 30, 1);
        /* @bug logic: turns hit_recoil into the world in place, so each hit
         * after the first turns the last hit's world speed again, not the speed
         * in the frame player 0 faces (JSR at 0x0c900c68) */
        player_turn_to_world(player_work[0], &hit_recoil);
        /* @hack spelling: x read through hit_recoil's address, and y and z as
         * members, which loads each member from a pool entry of its own
         * (0x0c900c6c to 0x0c900c72). as members, 51 of 51 words differ */
        player_set_speed(0, (&hit_recoil)->x, hit_recoil.y, hit_recoil.z);
    }
}

static void step_mode(struct task *task)
{
    struct task_work *work = task->work;
    struct task_motion *motion = task->motion;
    struct chaos *extra = task->extra;
    switch (work->mode) {
    case BOSS_INTRO:
        intro(task);
        break;
    case BOSS_WAIT:
        switch (work->submode) {
        case 0:
            chaos4_set_shapes(work, SHAPES_ON);
            if (camera_swing_done) {
                if (get_character() == CHARACTER_TAILS) {
                    extra->health -= 1.0F;
                }
                life_gauge_create(0, 0, (int)extra->health);
                chaos4_camera_start(&work->pos);
                if (work->mode != BOSS_SWIM) {
                    boss_next_mode = BOSS_SWIM;
                    boss_last_mode = work->mode;
                }
                work->submode = MODE_DONE;
            }
            break;
        }
        pad_disable();
        break;
    case BOSS_SWIM:
        pad_enable();
        time_start();
        swim(task);
        chaos4_hint();
        break;
    case BOSS_LEAP:
        chaos4_leap(work, motion, extra);
        break;
    case BOSS_SICKLE:
        chaos4_sickle(work, motion, extra);
        break;
    case BOSS_SWEEP:
        chaos4_sweep(work, motion, extra);
        break;
    case BOSS_SPLIT:
        chaos4_split(work, motion, extra);
        break;
    case BOSS_SURFACE:
        chaos4_surface(work, motion, extra);
        take_hit(work);
        break;
    case BOSS_HIT:
        chaos4_hit(work, motion, extra);
        break;
    }
    move(work, motion, extra);
}

void chaos4_create(void)
{
    task_create(TASK_WORK | TASK_MOTION, 1, step);
}

/* his destructor. it ends his mode as his step would. it then frees his
 * ribbon, his light, his motion's buffers and his record, and ends the
 * hint */
static void destroy(struct task *task)
{
    struct task_work *work = task->work;
    struct motion *controller = task->extra;
    if (work->submode != MODE_DONE) {
        work->submode = MODE_DONE;
        step_mode(task);
    }
    ribbon_free();
    chaos_light_delete();
    njSetMotionCallback(NULL);
    controller->weld_mode = PLAYER_WELD_FREE;
    motion_weld(work, controller);
    controller->state = MOTION_FREE;
    motion_step(work, controller);
    chaos4_hint_end();
    heap_free(CHAOS4_VARS(work)->record);
    /* this changes nothing: task_destroy, which runs the destructor, clears
     * the step task_kill sets once the destructor returns */
    task_kill(task);
}

/* his task's step. at mode 0 it sets him up, and after that it runs his
 * mode. it then takes the mode boss_next_mode asks for, poses and draws
 * him, and places his shapes */
static void step(struct task *task)
{
    struct task_work *work = task->work;
    struct chaos *state = task->extra;
    /* @hack spelling: extra, a second name for state, which mode 0 reads again
     * from task->extra once it has stored state there. it keeps task->extra
     * in both R13 and R11 (0x0c900f38, 0x0c900f3e); through state alone,
     * 238 of 248 words differ */
    struct chaos *extra = task->extra;
    struct chaos4 *record;
    struct chaos4_arm *arm;

    switch (work->mode) {
    case 0:
        state = (struct chaos *)motion_create(work, sizeof(struct chaos));
        if (!state) {
            return;
        }
        record = heap_calloc(1, sizeof(struct chaos4));
        /* a failed allocation returns at mode 0, and the next step tries
         * again */
        /* @bug memory: when heap_calloc fails, the motion record motion_create
         * made leaks: nothing keeps it. BRA at 0x0c900f64 leaves for the exit
         * with it unfreed */
        /* @hack redundant: an else after the return, which keeps &work->pos in
         * R2 and &work->scl in R1 (0x0c900f68 to 0x0c900f70); without it, 14 of
         * 248 words differ */
        if (!record) {
            return;
        } else {
            CHAOS4_VARS(work)->record = record;
            chaos4_pos = &work->pos;
            chaos4_scale = &work->scl;
            collision_init(task, chaos4_shapes, COLLISION_ENEMY, 1,
                           CHAOS4_SHAPES);
            work->collision->radius = 1000.0F;
            work->collision->flags &= ~COLLISION_TARGET;
            time_stop();
            state->motion.wanted = ACTION_INTRO;
            state->motion.current = ACTION_INTRO;
            state->motion.actions = chaos4_actions;
            state->motion.welds = chaos4_welds;
            work->mode = BOSS_INTRO;
            task->extra = state;
            extra = task->extra;
            boss_work = work;
            chaos_record = extra;
            record->draw |= DRAW_BODY;
            brain_create(task, 1.5F, &boss_points[8].pos, &brain_offset);
            brain_shown = 1;
            task->destructor = destroy;
            ribbon_init(work, extra, &chaos4_ribbon);
            chaos_setup(work, extra, CHAOS_4);
            work->pos = start_pos;
            chaos_light_create(40.0F);
            set_alpha(chaos4_alpha_objects, chaos_params->alpha);
            chaos4_camera_rise_start();
            motion_step(work, &extra->motion);
            extra->attack_timer = chaos_params->attack_wait;
            extra->texlist = &chaos4_common_texlist;
            chaos_params->turn = 182;
            chaos4_hide_tree(&chaos4_body_arm, FALSE);
            swamp_create();
            swamp_bottom_create();
            pads_create();
            arm = &record->arm;
            arm->stiffness = 0.01F;
            arm->damping = 0.97F;
            arm->stretch = 1.0F;
            work->ang.y = 0;
        }
        break;
    default:
        step_mode(task);
        break;
    }
    if (work->mode != boss_next_mode) {
        if (work->submode != MODE_DONE) {
            work->submode = MODE_DONE;
            step_mode(task);
        }
        boss_last_mode = work->mode;
        work->mode = boss_next_mode;
        work->submode = 0;
    }
    if (!(motion_flags & MOTION_HOLDING)) {
        motion_step(work, &state->motion);
    }
    motion_weld(work, &state->motion);
    chaos_track_points(work, chaos4_marks, chaos_params->model_y);
    draw(work, state);
    place_shapes(work->collision);
    chaos4_sweep_shapes(work);
    splash(CHAOS4_VARS(work)->record);
    extra->steps += 1U;
    life_gauge_health = extra->health;
    collision_register(work);
}

/* each step, splash on the swamp's surface over each of his shapes'
 * points, within 5 of the point in x and z. a point above the surface, or
 * down to 10 below it, splashes by a chance of 0.25. the chance is 0.2 down
 * to 20 below, 0.15 to 30 and 0.05 to 40, and none deeper. only his body
 * or his blob splashes */
static void splash(struct chaos4 *record)
{
    NJS_POINT3 position;
    Float depth;
    Float chance;
    int i;
    if (record->draw & (DRAW_BODY | DRAW_BLOB)) {
        for (i = 0; i < CHAOS4_SHAPES; i++) {
            chance = 0.0F;
            depth = -boss_points[i].pos.y + SWAMP_Y;
            if (depth <= 10.0F) {
                chance = 0.25F;
            } else if (depth <= 20.0F) {
                chance = 0.2F;
            } else if (depth <= 30.0F) {
                chance = 0.15F;
            } else if (depth <= 40.0F) {
                chance = 0.05F;
            }
            if (rand() * RAND_SCALE <= chance) {
                /* @hack spelling: the point copied through position's address,
                 * which stores each member through a fresh copy of R15 (MOV
                 * R15,R2 at 0x0c901322). as members, splash runs 32 bytes
                 * short, and 428 of 577 words differ */
                (&position)->x = boss_points[i].pos.x;
                (&position)->y = boss_points[i].pos.y;
                (&position)->z = boss_points[i].pos.z;
                position.y = SWAMP_Y;
                position.x += (rand() * (10.0F * RAND_SCALE)) + -5.0F;
                position.z += (rand() * (10.0F * RAND_SCALE)) + -5.0F;
                chaos4_splash_create(&position);
            }
        }
    }
}

/* on a frame's first step, while his struct chaos's figures hold
 * FIGURE_BODY, draw his body in its pose, or else his blob. then draw his
 * arm in the sweep */
static void draw(struct task_work *work, struct chaos *state)
{
    /* @hack spelling: extra, a second name for state, which keeps state in
     * both R13 and R12 (0x0c901794, 0x0c901796); through state alone, 132 of
     * 180 words differ */
    struct chaos *extra = state;
    struct chaos4 *record = CHAOS4_VARS(work)->record;
    int animation;
    Angle angle;
    NJS_ACTION *action;
    if (frame_step) {
        return;
    }
    njSetMotionCallback(set_material);
    chaos_light_follow(work);
    draw_control_save();
    draw_control_set(NJD_CONTROL_3D_OFFSET_MATERIAL);
    chaos_animate_surface(state);
    njSetTexture(extra->texlist);
    animation = state->motion.current;
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    if (extra->figures & FIGURE_BODY) {
        njPushMatrix(NULL);
        extra->colors[0].a = 0.0F;
        njSetConstantMaterial(&extra->colors[0]);
        TURN_VIA(njRotateZ, work->ang.z, angle);
        TURN_VIA(njRotateX, work->ang.x, angle);
        TURN_VIA(njRotateY, HALF_TURN - work->ang.y, angle);
        njTranslate(NULL, 0.0F, chaos_params->model_y, 0.0F);
        if (state->motion.state != MOTION_BLENDING) {
            action = state->motion.actions[animation].action;
        } else {
            action = state->motion.blend;
        }
        njSetConstantMaterial(&extra->colors[0]);
        if (record->draw & DRAW_BODY) {
            draw_action(action, state->motion.frame);
            ribbon_update();
        } else if (record->draw & DRAW_BLOB) {
            njScale(NULL, work->scl.x, work->scl.y, work->scl.z);
            draw_tree(&chaos4_blob_object);
        }
        njPopMatrix(1);
    }
    njPopMatrix(1);
    draw_arm(work);
    draw_control_restore();
    njSetMotionCallback(NULL);
}

static void draw_arm(struct task_work *work)
{
    Angle angle;
    struct chaos4 *record = CHAOS4_VARS(work)->record;
    if (record->draw & DRAW_ARM) {
        njPushMatrix(NULL);
        njTranslateV(NULL, &boss_points[7].pos);
        TURN_VIA(njRotateZ, work->ang.z, angle);
        TURN_VIA(njRotateX, work->ang.x, angle);
        TURN_VIA(njRotateY, HALF_TURN - work->ang.y, angle);
        njPushMatrix(NULL);
        TURN_VIA(njRotateZ, shoulder_angles[1], angle);
        TURN_VIA(njRotateX, shoulder_angles[0], angle);
        TURN_VIA(njRotateY, shoulder_angles[2], angle);
        chaos4_sweep_draw(work);
        njPopMatrix(2);
    }
}

static void set_material(NJS_OBJECT *object)
{
    chaos_set_material(object);
}

/* the intro. he sinks from start_pos into the swamp, splashing as he meets
 * it, until he is below INTRO_DEPTH. after a wait, the camera circles him
 * while SE_ORBIT sounds, and the intro asks for BOSS_WAIT */
static void intro(struct task *task)
{
    struct task_work *work = task->work;
    NJS_POINT3 *position;

    switch (work->submode) {
    case INTRO_START:
        splash_pending = 1;
        intro_flare = flare_create(&flare_pos);
        pad_disable();
        time_stop();
        work->submode = INTRO_SINK;
        break;
    case INTRO_FLARE:
        /* no step sets INTRO_FLARE. it moves the flare by pad 0's buttons and
         * prints the flare's place, until A sends it on to INTRO_SINK */
        position = &intro_flare->work->pos;
        if (per_info[0]->on & PDD_DGT_KL) {
            position->x += 1.0F;
        }
        if (per_info[0]->on & PDD_DGT_KR) {
            position->x -= 1.0F;
        }
        if (per_info[0]->on & PDD_DGT_KU) {
            position->y += 1.0F;
        }
        if (per_info[0]->on & PDD_DGT_KD) {
            position->y -= 1.0F;
        }
        if (per_info[0]->on & PDD_DGT_TL) {
            position->z += 1.0F;
        }
        if (per_info[0]->on & PDD_DGT_TR) {
            position->z -= 1.0F;
        }
        njPrintD(0xa000a, (Int)position->x, 5);
        njPrintD(0xa000b, (Int)position->y, 5);
        njPrintD(0xa000c, (Int)position->z, 5);
        if (per_info[0]->on & PDD_DGT_TA) {
            work->submode = INTRO_SINK;
        }
        break;
    case INTRO_SINK:
        pad_disable();
        time_stop();
        if (splash_pending && work->pos.y <= SWAMP_Y) {
            splash_pending = 0;
            column_create(&work->pos, NULL);
            swamp_fade(1);
            swamp_ripple_add(&work->pos);
            sound_play(SE_SPLASH, 0, 0, 0);
        }
        if ((work->pos.y -= INTRO_SINK_SPEED) < INTRO_DEPTH) {
            intro_timer = INTRO_WAIT_TIME;
            task_kill(intro_flare);
            work->submode = INTRO_WAIT;
        }
        break;
    case INTRO_WAIT:
        if (--intro_timer < 0) {
            work->submode = INTRO_ORBIT;
        }
        break;
    case INTRO_ORBIT:
        chaos4_camera_orbit_start();
        sound_play_held(SE_ORBIT, 0, 1, 0);
        intro_timer = INTRO_ORBIT_TIME;
        work->submode = INTRO_ORBITING;
        /* fall through */
    case INTRO_ORBITING:
        if (--intro_timer < 0) {
            sound_stop(SE_ORBIT);
            work->submode = INTRO_END;
        }
        break;
    case INTRO_END:
        camera_swing_done = 1;
        if (work->mode != BOSS_WAIT) {
            boss_next_mode = BOSS_WAIT;
            boss_last_mode = work->mode;
        }
        work->submode = MODE_DONE;
        break;
    }
}
