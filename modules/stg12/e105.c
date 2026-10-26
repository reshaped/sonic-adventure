/* E-105's introduction, eight missile launchers and defeat in Hot Shelter */
#include "stg12/e105.h"
#include "animal/animal.h"
#include "camera/camera.h"
#include "collision/collision.h"
#include "debug/editor.h"
#include "draw/draw.h"
#include "draw/tree.h"
#include "effect/bomb.h"
#include "enemy/enemy.h"
#include "hud/boss_name.h"
#include "hud/hud.h"
#include "hud/life_gauge.h"
#include "player/lockon.h"
#include "player/player.h"
#include "sound/sound.h"
#include "sound/stream.h"
#include "stage/stage.h"
#include "stg12/camera_sweep.h"
#include "stg12/e105_ring_motion.h"
#include "stg12/missile.h"
#include "system/frame.h"
#include "system/math.h"
#include "system/pad.h"
#include "system/task.h"
#include "system/util.h"
#include <ninja.h>
#include <sg_xpt.h>
#include <shinobi.h>
#include <stdlib.h>

enum e105_mode { E105_INTRO, E105_FIGHT, E105_DEFEAT, E105_DEBUG };
enum launcher_mode {
    LAUNCHER_READY,
    LAUNCHER_ARM,
    LAUNCHER_EXTEND,
    LAUNCHER_RELEASE,
    LAUNCHER_RECOIL,
    LAUNCHER_BROKEN
};
#define HEAD_DETACHED 0x1
/* the submode a mode has on its last run */
#define E105_DONE (-1)

/* the head and the three rings, which turn about y, as indexes of rotation,
 * rotation_speed and target_speed. the launchers ride RING_2 */
enum e105_part { HEAD, RING_1, RING_2, RING_3, PARTS };

/* the head's height above E-105's position */
#define HEAD_HEIGHT 45.0F

#define LAUNCHERS 8
/* the turn from one launcher to the next */
#define LAUNCHER_TURN (0x10000 / LAUNCHERS)
/* a launcher's base and far end, where its sensor sits, out from E-105's
 * axis */
#define LAUNCHER_BASE 22.5F
#define LAUNCHER_END 46.5F
/* a held missile slides out from the base as extension eases toward
 * EXTENSION_AIM. the hold ends once extension reaches EXTENSION_END, short
 * of the aim */
#define EXTENSION_AIM 24.0F
#define EXTENSION_END 23.0F
/* the launchers arm only while fewer missiles than this are alive */
#define MISSILES 8

/* ring 3 has BOBS parts that bob, each at most BOB_DEPTH below its place. a
 * bob's phase turns BOB_STEP a step */
#define BOBS 6
#define BOB_DEPTH 15.0F
#define BOB_STEP 0x222

/* intro waits for player 0 to stand on the ground in the arena. it reaches
 * ARENA_RADIUS across from arena_center, and from ARENA_BOTTOM to ARENA_TOP
 * high */
#define ARENA_RADIUS 200.0F
#define ARENA_BOTTOM 150.0F
#define ARENA_TOP 300.0F

/* the steps the camera sweeps round E-105 as it bursts */
#define SWEEP_STEPS 480

#define SE_EXPLOSION 25
#define SE_E105_HIT 155
#define SE_E105_HEAD_OFF 329
/* played as the stage clears */
#define SE_E105_DOWN 1327

/* a part of ring 3 that bobs. offset moves it down from its place by
 * BOB_DEPTH times the size of phase's sine */
struct bob {
    Float offset;
    Angle phase;
};
/* a held missile's step reads missile_pos and missile_dir through pointers.
 * E-105's destructor calls task_kill on each held missile, which makes
 * task_destroy the missile's next step. so no missile reads the state once
 * it is freed */
struct launcher {
    int mode;
    struct task *sensor; /* what E-102 locks on to and shoots */
    struct task *missile; /* the missile it holds, or NULL */
    Float extension; /* how far missile has slid out from origin */
    NJS_POINT3 missile_pos;
    NJS_VECTOR missile_dir;
    NJS_POINT3 sensor_pos;
    NJS_POINT3 origin; /* its base */
    Float recoil; /* 0 at rest, below 0 as a launch pushes it in */
    Float recoil_speed;
};
/* heap_alloc leaves the state uncleared. intro's first run, not init, sets
 * steps, flags and lead. the step that first runs intro reads steps and
 * lead before intro sets them, but nothing uses what it computes from them */
struct e105 {
    Float health; /* in the life gauge's units: 4, less 0.5 a broken launcher */
    int steps; /* since intro's first run */
    int timer; /* steps left in the defeat's current submode */
    NJS_POINT3 burst_origin; /* where the defeat's blasts center */
    int flags;
    int previous_mode;
    int next_mode;
    Angle rotation[PARTS];
    Angle rotation_speed[PARTS];
    Angle target_speed[PARTS];
    Angle target_heading;
    Float lead; /* target leads player 0 by this many steps of its speed */
    NJS_POINT3 target;
    struct bob bob[BOBS];
    struct launcher launcher[LAUNCHERS];
};

static void wait_for_player(struct task *task);
static void draw(struct task *task);
static struct task *sensor_create(void);
static NJS_POINT3 launcher_offset;
static void (*modes[4])(struct task *task);
static NJS_OBJECT *bob_objects[BOBS];
static struct collision_shape sensor_shape;
static const NJS_POINT3 arena_center;
static const char boss_title[6];
/* the task draw last ran for, which bob_motion_part reads: a motion callback
 * gets no task. dat_0caa58d8.c defines it */
extern struct task *e105_drawing_task;
extern NJS_OBJECT e105_head_object;
extern NJS_OBJECT e105_ring_1_object;
extern NJS_OBJECT e105_ring_2_object;
extern NJS_OBJECT e105_ring_3_object;
extern NJS_OBJECT e105_launcher_object;
extern NJS_OBJECT e105_broken_launcher_object[];
extern NJS_OBJECT e105_bob_0_object;
extern NJS_OBJECT e105_bob_1_object;
extern NJS_OBJECT e105_bob_2_object;
extern NJS_OBJECT e105_bob_3_object;
extern NJS_OBJECT e105_bob_4_object;
extern NJS_OBJECT e105_bob_5_object;

#define BODY_ATTACK 3

/* E-105's body: two spheres, above and below its center, that attack at
 * BODY_ATTACK. with guard 3, no attack hurts them */
static struct collision_shape body_shapes[2] = {
    {SHAPE_HURTABLE,
     SHAPE_HURTS_PLAYERS | SHAPE_HURTS_ENEMIES | SHAPE_HURTS_OBJECTS |
         GUARD(3) | BODY_ATTACK,
     PUSH_FIRM(PUSH_ALL),
     {0.0F, 20.0F, 0.0F},
     35.0F,
     0.0F,
     0.0F,
     {0, 0, 0},
     0,
     NULL},
    {SHAPE_HURTABLE,
     SHAPE_HURTS_PLAYERS | SHAPE_HURTS_ENEMIES | SHAPE_HURTS_OBJECTS |
         GUARD(3) | BODY_ATTACK,
     PUSH_FIRM(PUSH_ALL),
     {0.0F, -10.0F, 0.0F},
     35.0F,
     0.0F,
     0.0F,
     {0, 0, 0},
     0,
     NULL}};
static void init(struct task *task);
static void destroy(struct task *task);

void e105_start(struct task *task)
{
    time_stop();
    if (placement_editing(task)) {
        if (!task->extra) {
            init(task);
        }
        draw(task);
    } else {
        init(task);
        task->step = wait_for_player;
        task->draw = draw;
        task->destructor = destroy;
    }
}

static void init(struct task *task)
{
    struct task_work *work = task->work;
    struct e105 *state;
    struct bob *part;
    struct launcher *launcher;
    Float sine;
    Float cosine;
    int i;

    collision_init(task, body_shapes, COLLISION_ENEMY, 1, 2);
    work->collision->radius = 100.0F;
    task->extra = state = heap_alloc(sizeof(struct e105));
    state->next_mode = E105_INTRO;
    state->previous_mode = E105_INTRO;
    work->mode = E105_INTRO;
    work->submode = 0;
    for (i = 0; i < PARTS; i++) {
        state->rotation[i] = 0;
        state->rotation_speed[i] = 0;
        state->target_speed[i] = 0;
    }
    part = state->bob;
    for (i = 0; i < BOBS; i++, part++) {
        part->phase =
            (Angle)(rand() * (360.0F * RAND_SCALE) * (65536.0F / 360.0F));
        _builtin_fsca(part->phase, &sine, &cosine);
        part->offset = -((sine < 0.0F ? -sine : sine) * BOB_DEPTH);
    }

    launcher = state->launcher;
    for (i = 0; i < LAUNCHERS; i++, launcher++) {
        launcher->mode = LAUNCHER_READY;
        launcher->missile = 0;
        launcher->extension = 0.0F;
        launcher->recoil = 0.0F;
        launcher->recoil_speed = 0.0F;

        launcher->sensor = sensor_create();
    }
}

static void destroy(struct task *task)
{
    struct e105 *state = task->extra;
    struct launcher *launcher = state->launcher;
    int i;

    for (i = 0; i < LAUNCHERS; i++, launcher++) {
        if (launcher->sensor) {
            task_kill(launcher->sensor);
        }
        if (launcher->missile) {
            task_kill(launcher->missile);
        }
    }
}

struct head_motion {
    int age;
    NJS_VECTOR velocity;
    Angle spin_x, spin_y, spin_z;
};

static void arm_launcher(struct task *task, int index);
static void head_create(NJS_POINT3 *position, Angle heading);

static int player_in_arena(struct task *task)
{
    struct task_work *work = player_get_task(0)->work;
    NJS_POINT3 *position = &work->pos;

    /* point_distance_squared_across only reads its points, so the cast that
     * drops arena_center's const writes nothing */
    Float distance =
        point_distance_squared_across(position, (NJS_POINT3 *)&arena_center);
    Float height;

    if (distance <= ARENA_RADIUS * ARENA_RADIUS) {
        height = position->y;
        if (height <= ARENA_TOP && height >= ARENA_BOTTOM &&
            (work->flags & PLAYER_ON_GROUND)) {
            return 1;
        }
    }
    return 0;
}

/* the heading from origin to target, across */
static Angle heading_to(NJS_POINT3 *target, NJS_POINT3 *origin)
{
    Float dx = target->x - origin->x;
    Float dz = target->z - origin->z;
    return angle_arc_tan2(dz, dx);
}

static void approach_angle(Angle target, Angle *angle, Angle *velocity,
                           Angle limit, Float acceleration, Float damping)
{
    Angle change = (Angle)(angle_difference(*angle, target) * acceleration);
    *velocity += change;
    *velocity *= damping;
    *velocity = CLAMP(*velocity, -limit, limit);
    *angle += *velocity;
    if ((*angle - target < 0 ? target - *angle : *angle - target) < 91 &&
        (*velocity < 0 ? -*velocity : *velocity) < 18) {
        *velocity = 0;
        *angle = target;
    }
}

static void bob_parts(struct task *task)
{
    struct e105 *state = task->extra;
    struct bob *part = state->bob;
    Float sine;
    Float cosine;
    unsigned int i;
    for (i = BOBS; i > 0; --i) {
        part->phase += BOB_STEP;
        _builtin_fsca(part->phase, &sine, &cosine);
        part->offset = -((sine < 0.0F ? -sine : sine) * BOB_DEPTH);
        ++part;
    }
}

static void turn_toward_player(struct task *task)
{
    struct e105 *state = task->extra;
    approach_angle(state->target_heading, &state->rotation[HEAD],
                   &state->rotation_speed[HEAD], 0x444, 0.001F, 0.98F);
}

static void turn_rings(struct task *task)
{
    struct e105 *state = task->extra;
    int i;
    for (i = RING_1; i < PARTS; ++i) {
        state->rotation[i] += state->rotation_speed[i];
        state->rotation_speed[i] +=
            (state->target_speed[i] - state->rotation_speed[i]) * 0.02F;
    }
}

static void place_launchers(struct task *task)
{
    struct task_work *work = task->work;
    struct e105 *state = task->extra;
    NJS_VECTOR *speed = &player_motion[0]->speed;
    NJS_POINT3 *position = &player_get_task(0)->work->pos;
    struct launcher *launcher;
    Angle angle;
    int i;
    /* @hack spelling: njPopMatrix called through pop, whose address SHC loads
     * into R9 at 0x0c9153a8, ahead of njRotateY's; called by name, 58 of 154
     * words differ */
    Bool (*pop)(Uint32);

    state->target.x = position->x;
    state->target.y = position->y;
    state->target.z = position->z;
    state->target.x += speed->x * state->lead;
    state->target.z += speed->z * state->lead;
    state->target_heading = heading_to(&work->pos, &state->target);
    launcher = state->launcher;
    njPushMatrix(&_unit_matrix_);
    njTranslateV(0, &work->pos);
    TURN_VIA(njRotateY, -state->rotation[RING_2], angle);

    pop = njPopMatrix;
    for (i = 0; i < LAUNCHERS; i++, ++launcher) {
        njPushMatrix(0);

        /* @hack type: z stored through its byte offset, which makes SHC load
         * z's address from the pool for both stores (0x0c9153b0,
         * 0x0c9153c0); as launcher_offset.z, 64 of 154 words differ */
        *(Float *)((Uint8 *)&launcher_offset + 8) = LAUNCHER_BASE;
        njCalcPoint(0, &launcher_offset, &launcher->origin);
        launcher_offset.z = launcher->recoil + LAUNCHER_END;
        njCalcPoint(0, &launcher_offset, &launcher->sensor_pos);
        pop(1);
        njRotateY(0, LAUNCHER_TURN);
    }
    pop(1);
}

/* 1 when a shot flying at the sensor's lock-on hits the sensor, and
 * otherwise 0. on a hit, the sensor lets the lock-on go */
static int launcher_take_hit(struct launcher *launcher)
{
    struct task *task = launcher->sensor;
    struct task_work *work = task->work;
    struct lockon_target *storage = LOCKON_TARGET(task);
    struct task *contact;
    if ((work->flags & COLLISION_DAMAGED) &&
        work->collision->hit->kind == COLLISION_ATTACK &&
        storage->lockon != 0) {
        contact = work->collision->hit->owner;
        if (contact) {
            struct task *ring = lockon_of_shot(contact);
            if (ring == storage->lockon) {
                lockon_release(task);
                return 1;
            }
        }
    }
    return 0;
}

static void step_launchers(struct task *task)
{
    struct e105 *state = task->extra;
    struct task_work *work = task->work;
    struct launcher *launcher;
    struct task_work *sensor;
    struct task_work *player;
    NJS_VECTOR delta;
    NJS_VECTOR facing;
    Angle difference;
    Float alignment;
    int broken = 0;
    int i;

    launcher = state->launcher;
    for (i = 0; i < LAUNCHERS; ++i, ++launcher) {
        sensor = launcher->sensor->work;
        sensor->pos.x = launcher->sensor_pos.x;
        sensor->pos.y = launcher->sensor_pos.y;
        sensor->pos.z = launcher->sensor_pos.z;
        if (launcher_take_hit(launcher) == 1) {
            if (launcher->missile) {
                task_kill(launcher->missile);
                launcher->missile = 0;
            }
            state->health -= 0.5F;
            sound_play_at(SE_E105_HIT, 0, 0, 0, work->pos.x, work->pos.y,
                          work->pos.z);
            /* @hack redundant: guard 3, which no attack exceeds, set by
             * clearing the guard first, as AND #243 and OR #12 at 0x0c91559c
             * do. GUARD(3) sets every bit of SHAPE_GUARD, so the clear changes
             * nothing. with |= GUARD(3), the object's code is 32 bytes shorter
             * and 358 of 430 words differ */
            launcher->sensor->work->collision->shapes->levels =
                (launcher->sensor->work->collision->shapes->levels &
                 ~SHAPE_GUARD) |
                GUARD(3);

            /* @hack redundant: a loop of one pass round the call, which makes
             * SHC load its 1.0 first, FLDI1 FR13 at 0x0c915524; without it, 3
             * of 430 words differ */
            do {
                bomb_create(&launcher->sensor_pos, 1.0F);
            } while (0);
            launcher->recoil = 0.0F;
            launcher->recoil_speed = 0.0F;
            launcher->mode = LAUNCHER_BROKEN;
        }
        player = player_get_task(0)->work;
        /* @hack spelling: delta and facing written through their addresses,
         * which copies R15 to R3 for each store, as at 0x0c9155c0; by name,
         * the object's code is 32 bytes shorter and 331 of 430 words differ */
        (&delta)->x = work->pos.x - player->pos.x;
        (&delta)->z = work->pos.z - player->pos.z;
        (&facing)->x = launcher->sensor_pos.x - launcher->origin.x;
        (&facing)->z = launcher->sensor_pos.z - launcher->origin.z;
        delta.y = 0.0F;
        facing.y = 0.0F;
        /* only a launcher facing player 0 registers its sensor. one facing
         * away lets its lock-on go, and the shot flies on */
        alignment = vector_inner_product(&delta, &facing);
        if (alignment >= 0.0F) {
            lockon_release_hit(launcher->sensor);
        } else {
            collision_register(sensor);
        }
        switch (launcher->mode) {
        case LAUNCHER_READY:
            /* @bug arithmetic: launcher i tests its heading every 10i steps.
             * launcher 0 takes steps % 0, which SHC's __modls answers with 0
             * (JSR at 0x0c9156bc), so it tests on every step */
            if (state->steps % (i * 10) == 0) {
                difference = angle_difference(
                    state->target_heading,
                    heading_to(&work->pos, &launcher->sensor_pos));
                /* @bug logic: angle_difference gives -0x8000 to 0x7fff, so
                 * this test always holds (CMP/GE at 0x0c9156e0, CMP/GT at
                 * 0x0c9156ea). a ready launcher arms whichever way it faces */
                if (difference >= -0x8000 && difference <= 0x8000) {
                    if (e105_missile_count < MISSILES) {
                        arm_launcher(task, i);
                    }
                }
            }
            break;
        case LAUNCHER_ARM:
            if (launcher->missile) {
                launcher->mode = LAUNCHER_READY;
                break;
            }
            launcher->extension = 0.0F;
            /* @bug uninitialized: e105_missile_create copies missile_pos and
             * missile_dir (FMOV.S @R11,FR3 at 0x0c916906 on) before the
             * fall-through into LAUNCHER_EXTEND sets them in this same step.
             * a launcher's first missile copies the heap's old bytes, and a
             * later one the last launch's. the missile's first hold step reads
             * them again. only a paused frame before that step draws the stale
             * copy */
            launcher->missile = e105_missile_create(&launcher->missile_pos,
                                                    &launcher->missile_dir);
            ++launcher->mode;
        case LAUNCHER_EXTEND:
            launcher->extension +=
                (EXTENSION_AIM - launcher->extension) * 0.06F;
            if (launcher->extension >= EXTENSION_END) {
                ++launcher->mode;
            } else {
                /* @hack spelling: delta read and written through its address,
                 * which copies R15 to a register for each access, as at
                 * 0x0c915790; by name, the object's code is 32 bytes shorter
                 * and 181 of 430 words differ */
                (&delta)->x = launcher->origin.x - work->pos.x;
                (&delta)->y = launcher->origin.y - work->pos.y;
                (&delta)->z = launcher->origin.z - work->pos.z;
                delta.y = 0.0F;
                vector_unit_across(&delta, &delta);
                launcher->missile_dir.x = (&delta)->x;
                launcher->missile_dir.y = (&delta)->y;
                launcher->missile_dir.z = (&delta)->z;
                (&delta)->x = (&delta)->x * launcher->extension;
                (&delta)->y = (&delta)->y * launcher->extension;
                (&delta)->z = (&delta)->z * launcher->extension;
                launcher->missile_pos.x = (&delta)->x + launcher->origin.x;
                launcher->missile_pos.y = (&delta)->y + launcher->origin.y;
                launcher->missile_pos.z = (&delta)->z + launcher->origin.z;
            }
            break;
        case LAUNCHER_RELEASE:
            e105_missile_release(launcher->missile);
            launcher->missile = 0;
            launcher->recoil_speed = -2.5F;
            ++launcher->mode;
            break;
        case LAUNCHER_RECOIL:
            launcher->recoil += launcher->recoil_speed;
            launcher->recoil_speed *= 0.85F;
            launcher->recoil += 0.5F;
            if (launcher->recoil >= 0.0F) {
                launcher->recoil = 0.0F;
                launcher->recoil_speed = 0.0F;
                launcher->mode = LAUNCHER_READY;
            }
            break;
        case LAUNCHER_BROKEN:
            ++broken;
            break;
        }
    }

    if (broken >= LAUNCHERS) {
        state->health = 0.0F;
        if (work->mode != E105_DEFEAT) {
            state->next_mode = E105_DEFEAT;
            state->previous_mode = (int)work->mode;
        }
    }
}

static void arm_launcher(struct task *task, int index)
{
    struct e105 *state = task->extra;
    struct launcher *launcher = &state->launcher[index];
    if (launcher->mode == LAUNCHER_READY) {
        launcher->mode = LAUNCHER_ARM;
    }
}

static void intro(struct task *task)
{
    struct task_work *work = task->work;
    struct e105 *state = task->extra;
    struct launcher *launcher;
    int i;
    bob_parts(task);
    switch (work->submode) {
    case 0:
        state->health = 4.0F;
        state->steps = 0;
        state->flags = 0;
        state->lead = 1.0F;
        e105_missile_count = 0;
        launcher = state->launcher;
        for (i = 0; i < LAUNCHERS; ++i, ++launcher) {
            launcher->mode = LAUNCHER_READY;
            launcher->sensor->work->collision->shapes->levels =
                (launcher->sensor->work->collision->shapes->levels &
                 ~SHAPE_GUARD) |
                GUARD(2);
        }
        ++work->submode;
        break;
    case 1:
        if (player_in_arena(task)) {
            time_stop();
            pad_disable();
            camera_start_swing(work, 30.0F, 80.0F, 240);
            camera_override_mode(CAMERA_MODE_CHAOS_STINIT, 0);
            boss_name_create(boss_title, -1, 240, 120);
            stream_request_music(MUSIC_BOSSTRGT);
            ++work->submode;
        }
        break;
    case 2:
        if (camera_swing_done) {
            work->submode = E105_DONE;
        }
        break;
    }
    if (work->submode == E105_DONE) {
        time_start();
        pad_enable();
        life_gauge_create(0, 0, (int)state->health);
        shelter_camera_restore(task);
        if (work->mode != E105_FIGHT) {
            state->next_mode = E105_FIGHT;
            state->previous_mode = (int)work->mode;
        }
    }
}

static void fight(struct task *task)
{
    struct task_work *work = task->work;
    turn_toward_player(task);
    bob_parts(task);
    step_launchers(task);

    /* @hack redundant: two empty switches on submode, whose tests are CMP/EQ #0
     * and CMP/EQ #-1 at 0x0c915ada and 0x0c915adc; without them, 15 of 17
     * words differ */
    switch (work->submode) {
    case 0:
        break;
    }
    switch (work->submode) {
    case E105_DONE:
        break;
    }
}

static void defeat(struct task *task)
{
    struct task_work *work = task->work;
    struct e105 *state = task->extra;
    struct task_work *player;
    NJS_POINT3 position;
    step_launchers(task);
    state->rotation[HEAD] += state->rotation_speed[HEAD];
    state->rotation_speed[HEAD] *= 0.95F;
    switch (work->submode) {
    case 0:
        time_stop();
        state->target_speed[RING_1] = 0;
        state->target_speed[RING_2] = 0;
        state->target_speed[RING_3] = 0;
        /* each homing missile bursts while e105_missile_count is negative */
        e105_missile_count = -1;
        state->burst_origin.x = work->pos.x;
        state->burst_origin.y = work->pos.y;
        state->burst_origin.z = work->pos.z;
        state->burst_origin.y += 35.0F;
        state->timer = 150;
        work->submode++;
        break;
    case 1:
        if (--state->timer <= 0) {
            player = player_get_task(0)->work;
            state->timer = 0;
            if (player->flags & PLAYER_ON_GROUND) {
                /* @hack spelling: position through its address, which copies
                 * R15 to a register for each store, as at 0x0c915bce; without
                 * it, 29 of 398 words differ */
                (&position)->x = work->pos.x;
                (&position)->y = work->pos.y;
                (&position)->z = work->pos.z;
                position.y += HEAD_HEIGHT;
                pad_disable();
                shelter_camera_sweep_start(&position, 0, SWEEP_STEPS);
                state->timer = SWEEP_STEPS;
                work->submode++;
            }
        }
        break;
    case 2:
        if (state->timer <= 360 && state->timer >= 60) {
            if (state->timer % ((state->timer * 15) / SWEEP_STEPS) == 0) {
                shelter_ray_create(&state->burst_origin, 120.0F);
            }
        }
        /* @bug arithmetic: the divisor is 0 once timer is 23 or less, and
         * SHC's __modls answers timer % 0 with 0 (JSR at 0x0c915c8a). so a
         * blast goes off on each of the last 23 steps */
        if (state->timer % ((state->timer * 20) / SWEEP_STEPS) == 0) {
            /* @hack spelling: position through its address, which copies R15 to
             * a register for each store, as at 0x0c915c94; without it, 234
             * of 398 words differ */
            (&position)->x = state->burst_origin.x;
            (&position)->y = state->burst_origin.y;
            (&position)->z = state->burst_origin.z;
            position.x += (rand() * (60.0F * RAND_SCALE)) - 30.0F;
            position.z += (rand() * (60.0F * RAND_SCALE)) - 30.0F;
            position.y += (rand() * (20.0F * RAND_SCALE)) - 10.0F;
            bomb_create(&position, 0.8F);
            if (rand() * RAND_SCALE < 0.6F) {
                sound_play_at(SE_EXPLOSION, 0, 0, -16, position.x, position.y,
                              position.z);
            }
        }
        if (--state->timer <= 0) {
            head_create(&work->pos, state->rotation[HEAD]);
            state->flags |= HEAD_DETACHED;
            /* @hack spelling: position through its address, which copies R15 to
             * a register for each store, as at 0x0c915d46; without it, 140
             * of 398 words differ */
            (&position)->x = work->pos.x;
            (&position)->y = work->pos.y;
            (&position)->z = work->pos.z;
            position.y += HEAD_HEIGHT;
            animal_create_staying(position.x, position.y, position.z,
                                  animal_start_kuja);
            sound_play(SE_E105_HEAD_OFF, 0, 0, 0);
            state->timer = 200;
            work->submode++;
        }
        break;
    case 3:
        if (state->timer >= 80) {
            if (state->timer % 30 == 0) {
                /* @hack spelling: position through its address, which copies
                 * R15 to a register for each store, as at 0x0c915e28; without
                 * it, 93 of 398 words differ */
                (&position)->x = state->burst_origin.x;
                (&position)->y = state->burst_origin.y;
                (&position)->z = state->burst_origin.z;
                position.x += (rand() * (45.0F * RAND_SCALE)) - 22.5F;
                position.z += (rand() * (45.0F * RAND_SCALE)) - 22.5F;
                position.y += (rand() * (20.0F * RAND_SCALE)) - 10.0F;
                bomb_create(&position, 0.8F);
            }
        }
        if (--state->timer <= 0) {
            pad_enable();
            sound_play(SE_E105_DOWN, 0, 0, 0);
            stage_clear();
            work->submode = E105_DONE;
        }
        break;
    }
    /* @hack redundant: an empty test of submode, the CMP/EQ #-1 at 0x0c915ebe;
     * without it, 10 of 398 words differ */
    if (work->submode == E105_DONE) {
    }
}

static void debug_wait(struct task *task)
{
    struct task_work *work = task->work;
    struct e105 *state = task->extra;
    step_launchers(task);
    bob_parts(task);
    if (per_info[0]->press & (PDD_DGT_TA | PDD_DGT_TB)) {
        if (work->mode != E105_FIGHT) {
            state->next_mode = E105_FIGHT;
            state->previous_mode = (int)work->mode;
        }
        shelter_camera_restore(task);
    }
}

static void step(struct task *task)
{
    struct e105 *state = task->extra;
    struct task_work *work = task->work;
    ++state->steps;
    place_launchers(task);
    modes[work->mode](task);
    turn_rings(task);
    /* a mode that ends runs once more with E105_DONE as its submode, unless
     * it set E105_DONE itself */
    if (work->mode != state->next_mode) {
        if (work->submode != E105_DONE) {
            work->submode = E105_DONE;
            modes[work->mode](task);
        }
        work->mode = state->next_mode;
        work->submode = 0;
    }
    life_gauge_health = state->health;
    collision_register(work);
    draw(task);
}

static void wait_for_player(struct task *task)
{
    struct task *player = player_get_task(0);
    if (player && player->work) {
        task->step = step;
    }
}

static void bob_motion_part(NJS_OBJECT *object)
{
    struct e105 *state = e105_drawing_task->extra;
    int i;
    for (i = 0; i < BOBS; ++i) {
        if (object == bob_objects[i]) {
            njTranslate(0, 0.0F, state->bob[i].offset, 0.0F);
            break;
        }
    }
}

static void draw_launchers(struct e105 *state)
{
    struct launcher *launcher = state->launcher;
    int i;
    for (i = 0; i < LAUNCHERS; ++i, ++launcher) {
        njPushMatrix(0);
        njTranslate(0, 0.0F, 20.0F, launcher->recoil + LAUNCHER_BASE);
        if (launcher->mode == LAUNCHER_BROKEN) {
            draw_tree(e105_broken_launcher_object);
        } else {
            draw_tree(&e105_launcher_object);
        }
        njPopMatrix(1);
        njRotateY(0, LAUNCHER_TURN);
    }
}

static void draw(struct task *task)
{
    struct task_work *work = task->work;
    struct e105 *state = task->extra;
    Angle angle;
    e105_drawing_task = task;
    if (frame_step) {
        return;
    }
    njSetTexture(&e105_texlist);
    njPushMatrix(0);
    njTranslateV(0, &work->pos);
    /* @bug uninitialized: the head goes undrawn where the heap left
     * HEAD_DETACHED set in flags (MOV.L @(24,R14),R0 at 0x0c916276). init
     * stores no flags, and intro's first run clears it. so every frame of the
     * set editor, where intro never runs, tests the heap's bit, as does a
     * paused frame before that run */
    if (!(state->flags & HEAD_DETACHED)) {
        njPushMatrix(0);
        TURN_VIA(njRotateY, -state->rotation[HEAD] - 0x4000, angle);
        draw_tree(&e105_head_object);
        njPopMatrix(1);
    }
    njPushMatrix(0);
    TURN_VIA(njRotateY, -state->rotation[RING_1], angle);
    draw_tree(&e105_ring_1_object);
    njPopMatrix(1);
    njPushMatrix(0);
    TURN_VIA(njRotateY, -state->rotation[RING_2], angle);
    draw_tree(&e105_ring_2_object);
    draw_launchers(state);
    njPopMatrix(1);
    njPushMatrix(0);
    TURN_VIA(njRotateY, -state->rotation[RING_3], angle);
    njSetMotionCallback(bob_motion_part);
    draw_motion(&e105_ring_3_object, &e105_ring_motion, 0.0F);
    njSetMotionCallback(0);
    njPopMatrix(1);
    njPopMatrix(1);
}

/* the task of a launcher's sensor, at level 3. returns the task, or null
 * when task_create returns null. init leaves the result unchecked */
static struct task *sensor_create(void)
{
    struct task *task;
    struct task_work *work;
    if ((task = task_create(TASK_WORK, 3, NULL)) != NULL) {
        work = task->work;

        /* @bug memory: collision_init copies 2 shapes from
         * sensor_shape, which holds 1 (MOV #2,R3 at 0x0c9163b8, the call at
         * 0x0c9163c4). the second is the 52 bytes after it, camera_sweep.c's
         * data from 0x0ca92058 */
        collision_init(task, &sensor_shape, COLLISION_ITEM, 1, 2);
        work->collision->radius = 200.0F;
        enemy_alloc_record(task, work);
        task->work->collision->shapes->levels =
            (task->work->collision->shapes->levels & ~SHAPE_GUARD) | GUARD(2);
    }
    return task;
}

static void head_draw(struct task *task)
{
    struct task_work *work = task->work;
    Angle angle;

    njPushMatrix(0);
    njTranslateV(0, &work->pos);
    TURN_VIA(njRotateZ, work->ang.z, angle);
    TURN_VIA(njRotateX, work->ang.x, angle);
    TURN_VIA(njRotateY, -work->ang.y - 0x4000, angle);
    njTranslate(0, 0.0F, -HEAD_HEIGHT, 0.0F);
    draw_tree(&e105_head_object);
    njPopMatrix(1);
}

static void head_step(struct task *task)
{
    struct task_work *work = task->work;
    struct head_motion *motion = task->extra;

    if (++motion->age >= 240) {
        task_kill(task);
    }
    /* @hack spelling: the sums written out, which keep motion in R13 and the
     * work in R14 (0x0c9164ca, 0x0c9164d0); with +=, the two swap and 42 of
     * 55 words differ */
    work->pos.x = work->pos.x + motion->velocity.x;
    work->pos.y = work->pos.y + motion->velocity.y;
    work->pos.z = work->pos.z + motion->velocity.z;
    motion->velocity.y -= 0.1F;
    work->ang.x += motion->spin_x;
    work->ang.z += motion->spin_z;
    head_draw(task);
}

static void head_create(NJS_POINT3 *position, Angle heading)
{
    struct task *task;
    struct task_work *work;
    struct head_motion *motion;
    Float sine;
    Float cosine;

    if ((task = task_create(TASK_WORK, 3, head_step)) != NULL) {
        work = task->work;
        motion = heap_alloc(sizeof(*motion));
        task->extra = motion;
        work->pos.x = position->x;
        work->pos.y = position->y;
        work->pos.z = position->z;
        work->pos.y += HEAD_HEIGHT;
        work->ang.x = 0;
        work->ang.z = 0;
        work->ang.y = heading;
        motion->spin_x = (Angle)(((rand() * (5.0F * RAND_SCALE)) - 2.5F) *
                                 (65536.0F / 360.0F));
        motion->spin_z = (Angle)(((rand() * (5.0F * RAND_SCALE)) - 2.5F) *
                                 (65536.0F / 360.0F));
        bomb_create(&work->pos, 3.0F);
        motion->age = 0;
        _builtin_fsca(
            (Angle)(rand() * (360.0F * RAND_SCALE) * (65536.0F / 360.0F)),
            &sine, &cosine);
        motion->velocity.x = cosine * 1.2F;
        motion->velocity.z = sine * 1.2F;
        motion->velocity.y = 2.0F;
        task->draw = head_draw;
    }
}

static const NJS_POINT3 arena_center = {0.0F, 0.0F, -3600.0F};
static const char boss_title[6] = {
#include "stg12/e105/boss_title.inc"
};

static NJS_POINT3 launcher_offset = {0.0F, 20.0F, 0.0F};

static void (*modes[])(struct task *task) = {intro, fight, defeat, debug_wait};

static NJS_OBJECT *bob_objects[] = {&e105_bob_0_object, &e105_bob_1_object,
                                    &e105_bob_2_object, &e105_bob_3_object,
                                    &e105_bob_4_object, &e105_bob_5_object};

static struct collision_shape sensor_shape = {
    SHAPE_HURTABLE | LOCKON_NO_NUMBER,
    SHAPE_HURTS_PLAYERS | SHAPE_HURTS_ENEMIES | SHAPE_HURTS_OBJECTS | GUARD(2),
    PUSH_FIRM(PUSH_ALL),
    {0.0F, 0.0F, 0.0F},
    8.0F,
    0.0F,
    0.0F,
    {0, 0, 0},
    0,
    NULL};
