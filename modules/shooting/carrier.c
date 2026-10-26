/* the egg carrier. it never moves from the world's origin. it draws at full
 * size there until the stage task sets carrier_arrived at act 2's end. it
 * then draws at half size and puts out six hodai. it opens its hatch to
 * fire a blast and shells, and its core takes the hits. CARRIER_HITS of
 * them, or CARRIER_STEPS of the fight, bring it down */
#include <ninja.h>
#include <sg_xpt.h>
#include <stdlib.h>

#include "collision/collision.h"
#include "draw/draw.h"
#include "effect/bomb.h"
#include "effect/dust.h"
#include "enemy/enemy.h"
#include "hud/life_gauge.h"
#include "player/lockon.h"
#include "shooting/carrier.h"
#include "shooting/carrier_hodai.h"
#include "shooting/hud.h"
#include "shooting/reticle.h"
#include "shooting/shooting.h"
#include "sound/sound.h"
#include "stage/stage.h"
#include "system/frame.h"
#include "system/task.h"
#include "system/util.h"

#define VARS(work) CARRIER_VARS(work)

enum carrier_mode {
    CARRIER_INIT = 0,
    CARRIER_COURSE = 1, /* at full size, until carrier_arrived is set */
    CARRIER_ARRIVE = 2,
    CARRIER_OPEN = 3,
    CARRIER_FIRE = 4,
    CARRIER_CLOSE = 8,
    CARRIER_WAIT = 9,
    CARRIER_EXPLODE = 10,
    CARRIER_SINK = 11,
    CARRIER_HIDDEN = 12, /* while an act's event plays */
    CARRIER_DELAY = 13,
    CARRIER_GONE = 14
};

enum blast_mode { BLAST_INIT, BLAST_PLAY, BLAST_STILL, BLAST_GONE };
enum core_mode { CORE_INIT, CORE_RUN, CORE_GONE };

#define CARRIER_HITS 6
/* the fight's steps before the carrier goes down anyway: ninety seconds, at
 * 60 steps a second */
#define CARRIER_STEPS (90 * 60)
#define CARRIER_HODAI 6

#define CORE_POINTS 100
#define FLASH_STEPS 20 /* a hit's flash on the carrier and its core */
#define HATCH_OPEN 29.0F /* the carrier's action frame with the hatch open */

/* CARRIER_ARRIVE's steps, and those of the first CARRIER_WAIT after it */
#define ARRIVE_STEPS 120
#define FIRST_WAIT 120
/* each later wait lasts from WAIT_LEAST up to WAIT_MOST steps, at random */
#define WAIT_LEAST 240.0F
#define WAIT_MOST 540.0F
/* CARRIER_FIRE's steps, which its timer counts down. the charge sounds
 * while more than SHELLS_LAST are left. a shell goes every 4th step from
 * SHELLS_FIRST left to SHELLS_LAST, and the blast sounds with BLAST_SOUND
 * left */
#define FIRE_STEPS 220
#define SHELLS_FIRST 120
#define SHELLS_LAST 100
#define BLAST_SOUND 85
/* CARRIER_EXPLODE's steps. a burst goes off at the core every BURST_EVERY
 * steps while more than SMOKE_STEPS are left. smoke then rises every
 * SMOKE_EVERY */
#define EXPLODE_STEPS 360
#define SMOKE_STEPS 120
#define BURST_EVERY 16
#define SMOKE_EVERY 8
#define SHELL_STEPS 120 /* the steps a shell flies */

/* the core's place on the carrier as carrier_draw draws it, at half size */
#define CORE_X 0.0F
#define CORE_Y (-324.0F)
#define CORE_Z (-1849.0F)

#define SE_CARRIER_ENGINE 674
#define SE_CARRIER_HATCH 685
#define SE_CARRIER_HATCH_STOP 686
#define SE_CARRIER_CHARGE 687
#define SE_CARRIER_BLAST 688

extern NJS_TEXLIST shooting0_texlist;
extern NJS_ACTION carrier_action;
extern NJS_ACTION carrier_propellers_action;
extern NJS_ACTION carrier_blast_action;

int carrier_arrived = 0;

static struct collision_shape carrier_blast_shape = {
    SHAPE_CYLINDER | SHAPE_HURTABLE,
    ATTACK(1) | SHAPE_HURTS_PLAYERS,
    PUSH_ALL,
    {0.0F, 0.0F, 0.0F},
    1000.0F,
    2000.0F,
    0.0F,
    {0, 0x4000, 0x4000},
    0,
    NULL};
static NJS_POINT3 carrier_hodai_places[CARRIER_HODAI] = {
#include "shooting/carrier/carrier_hodai_places.inc"
};
static struct collision_shape shell_shape = {SHAPE_SPHERE | SHAPE_HURTABLE,
                                             ATTACK(2) | GUARD(2) |
                                                 SHAPE_HURTS_PLAYERS,
                                             0,
                                             {0.0F, 0.0F, 0.0F},
                                             35.0F,
                                             0.0F,
                                             0.0F,
                                             {0},
                                             0,
                                             NULL};
static NJS_VECTOR shell_speed = {0.0F, 0.0F, -90.0F};
static struct collision_shape core_shape = {SHAPE_SPHERE | SHAPE_HURTABLE,
                                            ATTACK(1) | SHAPE_HURTS_PLAYERS,
                                            PUSH_FIRM(PUSH_ALL),
                                            {0.0F, 0.0F, 0.0F},
                                            70.0F,
                                            0.0F,
                                            0.0F,
                                            {0},
                                            0,
                                            NULL};

Bool hodai_may_fire;
/* the fight's steps so far, from the end of the carrier's arrival */
static int carrier_steps;

static void carrier_draw_full(struct task *task);
static void carrier_engine(struct task *task);
static void carrier_draw(struct task *task);
static void carrier_take_arrival(struct task *task);
static void carrier_open(struct task *task);
static void carrier_close(struct task *task);
static void carrier_fire(struct task *task);
static void carrier_wait(struct task *task);
static void carrier_destroy(struct task *task);
static void carrier_follow_events(struct task *task);
static void carrier_follow_gauge(struct task *task);
static void carrier_smoke(struct task *task);
static void carrier_explode(struct task *task);
static void carrier_sink(struct task *task);
static void carrier_spin(struct task_work *work);
static void carrier_count_time(struct task *task);
static void carrier_blast_step(struct task *task);
static void carrier_blast_draw(struct task *task);
static void carrier_blast_destroy(struct task *task);
static void carrier_blast_play(struct task *task);
static void carrier_add_hodai(struct task *task);
static void shell_step(struct task *task);
static void core_destroy(struct task *task);
static void core_hit(struct task *task);
static void core_place(struct task *task);
static void core_step(struct task *task);

void carrier_step(struct task *task)
{
    struct task_work *work = task->work;

    carrier_follow_events(task);
    switch (work->mode) {
    case CARRIER_INIT:
        task->destructor = carrier_destroy;
        task->draw = carrier_draw_full;
        life_gauge_health = CARRIER_HITS;
        VARS(work)->flash = 0;
        VARS(work)->down = FALSE;
        work->timer = 0;
        VARS(work)->hatch = 0.0F;
        VARS(work)->spin = 0.0F;
        work->pos.x = 0.0F;
        work->pos.y = 0.0F;
        work->pos.z = 0.0F;
        work->ang.x = 0;
        work->ang.y = 0;
        work->ang.z = 0;
        carrier_steps = 0;
        task_create_child(TASK_WORK, core_step, task);
        work->mode = CARRIER_COURSE;
        break;
    case CARRIER_COURSE:
        carrier_draw_full(task);
        carrier_take_arrival(task);
        carrier_engine(task);
        hodai_may_fire = FALSE;
        break;
    case CARRIER_ARRIVE:
        carrier_draw(task);
        if (--TASK_TIMER(work) == 0) {
            work->timer = FIRST_WAIT;
            work->mode = CARRIER_WAIT;
        }
        break;
    case CARRIER_OPEN:
        ++carrier_steps;
        carrier_draw(task);
        carrier_open(task);
        hodai_may_fire = TRUE;
        carrier_follow_gauge(task);
        collision_register(work);
        break;
    case CARRIER_FIRE:
        carrier_count_time(task);
        carrier_draw(task);
        carrier_fire(task);
        hodai_may_fire = FALSE;
        carrier_follow_gauge(task);
        collision_register(work);
        break;
    case CARRIER_CLOSE:
        carrier_count_time(task);
        carrier_draw(task);
        carrier_close(task);
        hodai_may_fire = TRUE;
        carrier_follow_gauge(task);
        collision_register(work);
        break;
    case CARRIER_WAIT:
        ++carrier_steps;
        carrier_draw(task);
        carrier_wait(task);
        hodai_may_fire = TRUE;
        carrier_follow_gauge(task);
        break;
    case CARRIER_HIDDEN:
        break;
    case CARRIER_EXPLODE:
        carrier_draw(task);
        carrier_explode(task);
        break;
    case CARRIER_SINK:
        carrier_count_time(task);
        carrier_draw(task);
        carrier_sink(task);
        carrier_smoke(task);
        break;
    case CARRIER_DELAY:
        carrier_draw(task);
        if (--TASK_TIMER(work) <= 0) {
            work->mode = CARRIER_GONE;
        }
        break;
    case CARRIER_GONE:
        carrier_arrived = -1;
        carrier_draw(task);
    }
    carrier_spin(work);
    task_step_children(task);
}

/* the carrier at full size, at a frame's first step only */
static void carrier_draw_full(struct task *task)
{
    struct task_work *work = task->work;

    if (frame_step == 0) {
        njFogDisable();
        njSetTexture(&shooting0_texlist);
        njPushMatrix(NULL);
        njTranslate(NULL, work->pos.x, work->pos.y, work->pos.z);
        njRotateXYZ(NULL, work->ang.x, work->ang.y, work->ang.z);
        draw_action(&carrier_action, VARS(work)->hatch);
        draw_action(&carrier_propellers_action, VARS(work)->spin);
        njPopMatrix(1);
    }
}

/* the engines' sound, which plays louder over one stretch of act 1's path
 * and two of act 2's */
static void carrier_engine(struct task *task)
{
    if (IN_STAGE(STAGE_SHOOTING1)) {
        if (shooting_path_index > 3000 && shooting_path_index < 4300) {
            sound_play_for(SE_CARRIER_ENGINE, (int)task, 1, 0, 10);
            sound_set_volume((int)task, 64);
        } else {
            sound_play_for(SE_CARRIER_ENGINE, (int)task, 1, 0, 10);
            sound_set_volume((int)task, 32);
        }
    } else {
        if (shooting_path_index > 3000 && shooting_path_index < 4300) {
            sound_play_for(SE_CARRIER_ENGINE, (int)task, 1, 0, 10);
            sound_set_volume((int)task, 64);
        } else if (shooting_path_index > 7000 && shooting_path_index < 9000) {
            sound_play_for(SE_CARRIER_ENGINE, (int)task, 1, 0, 10);
            sound_set_volume((int)task, 64);
        } else {
            sound_play_for(SE_CARRIER_ENGINE, (int)task, 1, 0, 10);
            sound_set_volume((int)task, 32);
        }
    }
}

/* the carrier at half size, white while a hit's flash lasts, at a frame's
 * first step only */
static void carrier_draw(struct task *task)
{
    struct task_work *work = task->work;

    if (frame_step == 0) {
        njFogDisable();
        draw_attr_save();
        if (VARS(work)->flash > 0) {
            draw_attr_set(0, NJD_FLAG_USE_ALPHA);
            draw_set_material(1.0F, 1.0F, 1.0F, 1.0F);
            njColorBlendingMode(NJD_SOURCE_COLOR, NJD_COLOR_BLENDING_SRCALPHA);
            njColorBlendingMode(NJD_DESTINATION_COLOR,
                                NJD_COLOR_BLENDING_INVSRCALPHA);
        }
        njSetTexture(&shooting0_texlist);
        njPushMatrix(NULL);
        njTranslate(NULL, work->pos.x, work->pos.y, work->pos.z);
        njRotateXYZ(NULL, work->ang.x, work->ang.y, work->ang.z);
        njScale(NULL, 0.5F, 0.5F, 0.5F);
        draw_action(&carrier_action, VARS(work)->hatch);
        draw_action(&carrier_propellers_action, VARS(work)->spin);
        njPopMatrix(1);
        draw_attr_restore();
    }
}

/* take carrier_arrived once the stage task sets it: put out the hodai and
 * the gauge, and arrive */
static void carrier_take_arrival(struct task *task)
{
    struct task_work *work = task->work;

    if (carrier_arrived) {
        carrier_add_hodai(task);
        work->mode = CARRIER_ARRIVE;
        carrier_arrived = 0;
        work->timer = ARRIVE_STEPS;
        life_gauge_create_child(0, 0, CARRIER_HITS, task);
        task->draw = carrier_draw;
    }
}

/* open the hatch, and fire once it's all the way open */
static void carrier_open(struct task *task)
{
    struct task_work *work = task->work;

    VARS(work)->hatch += 0.5F;
    sound_play_for(SE_CARRIER_HATCH, (int)task, 1, 0, 2);
    if (VARS(work)->hatch > HATCH_OPEN) {
        VARS(work)->hatch = HATCH_OPEN;
        work->mode = CARRIER_FIRE;
        work->timer = FIRE_STEPS;
        task_create_child(TASK_WORK, carrier_blast_step, task);
        sound_play(SE_CARRIER_HATCH_STOP, 0, 0, 0);
    }
}

/* shut the hatch, then wait */
static void carrier_close(struct task *task)
{
    struct task_work *work = task->work;

    VARS(work)->hatch -= 0.5F;
    sound_play_for(SE_CARRIER_HATCH, (int)task, 1, 0, 2);
    if (VARS(work)->hatch <= 0.0F) {
        VARS(work)->hatch = 0.0F;
        work->timer = RANDOM(WAIT_LEAST, WAIT_MOST);
        work->mode = CARRIER_WAIT;
        sound_play(SE_CARRIER_HATCH_STOP, 0, 0, 0);
    }
}

/* hold the hatch open for the charge, the six shells and the blast. the
 * hatch shuts once the timer runs out */
static void carrier_fire(struct task *task)
{
    struct task_work *work = task->work;

    VARS(work)->hatch = HATCH_OPEN;
    if (--TASK_TIMER(work) <= 0) {
        work->mode = CARRIER_CLOSE;
        carrier_count_time(task);
    }
    if (TASK_TIMER(work) > SHELLS_LAST) {
        sound_play_for(SE_CARRIER_CHARGE, (int)task, 1, 0, 2);
    }
    if (TASK_TIMER(work) == BLAST_SOUND) {
        sound_play(SE_CARRIER_BLAST, 0, 0, 0);
    }
    if (TASK_TIMER(work) <= SHELLS_FIRST && TASK_TIMER(work) >= SHELLS_LAST) {
        if (!(TASK_TIMER(work) & 3)) {
            task_create_child(TASK_WORK, shell_step, task);
        }
    }
}

static void carrier_wait(struct task *task)
{
    struct task_work *work = task->work;

    VARS(work)->hatch = 0.0F;
    if (--TASK_TIMER(work) <= 0) {
        work->mode = CARRIER_OPEN;
    }
}

/* the carrier has nothing of its own to free */
static void carrier_destroy(struct task *task)
{
}

/* hide the carrier while an act's event plays, and draw it at full size
 * in CARRIER_COURSE after */
static void carrier_follow_events(struct task *task)
{
    struct task_work *work = task->work;

    switch (shooting_stage_mode) {
    case SHOOTING_EVENT1:
    case SHOOTING_EVENT2:
        task->draw = NULL;
        work->mode = CARRIER_HIDDEN;
        break;
    case SHOOTING_AFTER:
        task->draw = carrier_draw_full;
        work->mode = CARRIER_COURSE;
        break;
    }
}

/* count a hit's flash down. once the gauge is empty, the carrier goes down
 * and explodes */
static void carrier_follow_gauge(struct task *task)
{
    struct task_work *work = task->work;

    if (VARS(work)->flash > 0) {
        --VARS(work)->flash;
    }
    if (life_gauge_health == 0.0F) {
        VARS(work)->down = TRUE;
        work->timer = EXPLODE_STEPS;
        work->mode = CARRIER_EXPLODE;
    }
}

/* smoke from the core every 4 steps */
static void carrier_smoke(struct task *task)
{
    struct task_work *work = task->work;
    NJS_VECTOR speed = {0.0F, 8.0F, 0.0F};
    NJS_POINT3 pos;

    pos = work->pos;
    pos.x += CORE_X;
    pos.y += CORE_Y;
    pos.z += CORE_Z;
    ++work->timer;
    if (!(TASK_TIMER(work) & 3)) {
        dust_create(&pos, &speed, 50.0F);
    }
}

/* the bursts, then the smoke. the carrier sinks once the timer runs out */
static void carrier_explode(struct task *task)
{
    struct task_work *work = task->work;
    NJS_POINT3 bomb = {CORE_X, CORE_Y, CORE_Z};

    --work->timer;
    if (TASK_TIMER(work) <= 0) {
        work->mode = CARRIER_SINK;
    }
    if (TASK_TIMER(work) > SMOKE_STEPS) {
        if (!(TASK_TIMER(work) & (BURST_EVERY - 1))) {
            njPushMatrix(&_unit_matrix_);
            njTranslateV(NULL, &work->pos);
            njRotateXYZ(NULL, work->ang.x, work->ang.y, work->ang.z);
            njCalcPoint(NULL, &bomb, &bomb);
            njPopMatrix(1);
            bomb_create(&bomb, 6.0F);
            sound_play(SE_HIT, 0, 0, 0);
        }
    } else {
        NJS_VECTOR speed = {0.0F, 8.0F, 0.0F};
        NJS_POINT3 pos;

        pos = work->pos;
        pos.x += CORE_X;
        pos.y += CORE_Y;
        pos.z += CORE_Z;
        if (!(TASK_TIMER(work) & (SMOKE_EVERY - 1))) {
            dust_create(&pos, &speed, 30.0F);
        }
    }
}

/* shut the hatch, slower than carrier_close does, then wait in
 * CARRIER_DELAY */
static void carrier_sink(struct task *task)
{
    struct task_work *work = task->work;

    VARS(work)->hatch -= 0.125F;
    sound_play_for(SE_CARRIER_HATCH, (int)task, 1, 0, 2);
    if (VARS(work)->hatch <= 0.0F) {
        VARS(work)->hatch = 0.0F;
        work->timer = 20;
        work->mode = CARRIER_DELAY;
        sound_play(SE_CARRIER_HATCH_STOP, 0, 0, 0);
    }
}

static void carrier_spin(struct task_work *work)
{
    VARS(work)->spin += 0.5F;
    if (VARS(work)->spin >= 30.0F) {
        VARS(work)->spin = 0.0F;
    }
}

/* count the fight's steps. past CARRIER_STEPS, empty the gauge and
 * explode */
static void carrier_count_time(struct task *task)
{
    struct task_work *work = task->work;

    ++carrier_steps;
    if (carrier_steps > CARRIER_STEPS) {
        life_gauge_health = 0.0F;
        work->mode = CARRIER_EXPLODE;
    }
}

/* the blast from the hatch, which plays its action once */
static void carrier_blast_step(struct task *task)
{
    struct task_work *work = task->work;

    switch (work->mode) {
    case BLAST_INIT:
        task->destructor = carrier_blast_destroy;
        task->draw = carrier_blast_draw;
        VARS(work)->hatch = 0.0F;
        work->pos.x = 0.0F;
        work->pos.y = -648.0F;
        work->pos.z = -3698.0F;
        work->ang.x = 0;
        work->ang.y = 0;
        work->ang.z = 0;
        collision_init(task, &carrier_blast_shape, COLLISION_ITEM, 1, 1);
        work->mode = BLAST_PLAY;
        break;
    case BLAST_PLAY:
        carrier_blast_draw(task);
        carrier_blast_play(task);
        break;
    case BLAST_STILL:
        break;
    case BLAST_GONE:
        break;
    }
}

/* the blast, offset from the carrier at the carrier's half size */
static void carrier_blast_draw(struct task *task)
{
    struct task_work *work = task->work;

    njFogDisable();
    njSetTexture(&shooting0_texlist);
    njPushMatrix(NULL);
    njTranslateV(NULL, &task->parent->work->pos);
    njScale(NULL, 0.5F, 0.5F, 0.5F);
    njTranslateV(NULL, &work->pos);
    draw_action(&carrier_blast_action, VARS(work)->hatch);
    njPopMatrix(1);
}

static void carrier_blast_destroy(struct task *task)
{
}

static void carrier_blast_play(struct task *task)
{
    struct task_work *work = task->work;

    VARS(work)->hatch += 0.5F;
    if (VARS(work)->hatch > 99.0F) {
        VARS(work)->hatch = 0.0F;
        work->mode = BLAST_GONE;
        task_kill(task);
    }
}

/* a hodai at each of carrier_hodai_places, at the carrier's half size */
static void carrier_add_hodai(struct task *task)
{
    struct task_work *work = task->work;
    struct task *hodai;
    int i;

    for (i = 0; i < CARRIER_HODAI; i++) {
        hodai = task_create_child(TASK_WORK, carrier_hodai_step, task);
        if (hodai != NULL) {
            hodai->work->pos.x =
                (carrier_hodai_places[i].x * 0.5F) + work->pos.x;
            hodai->work->pos.y =
                (carrier_hodai_places[i].y * 0.5F) + work->pos.y;
            hodai->work->pos.z =
                (carrier_hodai_places[i].z * 0.5F) + work->pos.z;
            hodai->work->ang.x = 0;
            hodai->work->ang.y = 0;
            hodai->work->ang.z = 0;
        }
    }
}

/* a shell from the core, which flies at shell_speed for SHELL_STEPS. its scl
 * holds its offset from the carrier. its modes are the carrier's:
 * CARRIER_INIT, then CARRIER_WAIT as it flies, then CARRIER_GONE */
static void shell_step(struct task *task)
{
    struct task_work *work = task->work;
    struct task_work *carrier = task->parent->work;

    switch (work->mode) {
    case CARRIER_INIT:
        work->scl.x = CORE_X;
        work->scl.y = CORE_Y;
        work->scl.z = CORE_Z;
        work->ang = carrier->ang;
        work->pos.x = carrier->pos.x + work->scl.x;
        work->pos.y = carrier->pos.y + work->scl.y;
        work->pos.z = carrier->pos.z + work->scl.z;
        collision_init(task, &shell_shape, COLLISION_ITEM, 1, 1);
        work->collision->radius = 180.0F;
        work->timer = SHELL_STEPS;
        work->mode = CARRIER_WAIT;
        break;
    case CARRIER_WAIT:
        work->scl.x += shell_speed.x;
        work->scl.y += shell_speed.y;
        work->scl.z += shell_speed.z;
        work->pos.x = carrier->pos.x + work->scl.x;
        work->pos.y = carrier->pos.y + work->scl.y;
        work->pos.z = carrier->pos.z + work->scl.z;
        if (--TASK_TIMER(work) <= 0) {
            work->mode = CARRIER_GONE;
        }
        collision_register(work);
        break;
    case CARRIER_GONE:
        task_destroy(task);
        break;
    }
}

static void core_destroy(struct task *task)
{
    enemy_end(task, task->work, (struct lockon_target *)task->motion);
}

/* a hit scores CORE_POINTS and takes a unit off the gauge. it flashes the
 * core and the carrier for FLASH_STEPS, and the core drops hits until its
 * flash ends. the core takes hits only while the carrier opens, fires or
 * shuts. otherwise its guard turns them away */
static void core_hit(struct task *task)
{
    struct task_work *work = task->work;
    struct task_work *carrier = task->parent->work;

    if (VARS(work)->flash > 0) {
        --VARS(work)->flash;
        work->flags &= ~COLLISION_DAMAGED;
    } else if (work->flags & COLLISION_DAMAGED) {
        VARS(work)->flash = FLASH_STEPS;
        VARS(carrier)->flash = FLASH_STEPS;
        shooting_score_hit(CORE_POINTS, work);
        lock_release(task);
        sound_play(SE_HIT, 0, 0, 0);
        bomb_create(&work->pos, 6.0F);
        life_gauge_health -= 1.0F;
        if (life_gauge_health == 0.0F) {
            sound_play(SE_SHOT_DOWN, 0, 0, 0);
            work->mode = CORE_GONE;
            shooting_time_bonus(carrier_steps);
        }
        collision_register(work);
    } else {
        switch (carrier->mode) {
        case CARRIER_OPEN:
        case CARRIER_FIRE:
        case CARRIER_CLOSE:
            work->collision->shapes->levels =
                work->collision->shapes->levels & ~SHAPE_GUARD;
            collision_register(work);
            break;
        default:
            work->collision->shapes->levels =
                (work->collision->shapes->levels & ~SHAPE_GUARD) | SHAPE_GUARD;
            lock_release(task);
            work->flags &= ~COLLISION_DAMAGED;
            break;
        }
    }
}

/* put the core at its place on the carrier, turned as the carrier turns */
static void core_place(struct task *task)
{
    NJS_POINT3 offset = {CORE_X, CORE_Y, CORE_Z};
    struct task_work *work;
    struct task_work *carrier;

    work = task->work;
    carrier = task->parent->work;
    njPushMatrix(&_unit_matrix_);
    njTranslateV(NULL, &carrier->pos);
    njRotateXYZ(NULL, carrier->ang.x, carrier->ang.y, carrier->ang.z);
    njCalcPoint(NULL, &offset, &work->pos);
    njPopMatrix(1);
}

static void core_step(struct task *task)
{
    struct task_work *work = task->work;

    switch (work->mode) {
    case CORE_INIT:
        task->destructor = core_destroy;
        core_place(task);
        enemy_alloc_record(task, work);
        collision_init(task, &core_shape, COLLISION_ITEM, 1, 1);
        work->mode = CORE_RUN;
        break;
    case CORE_RUN:
        core_place(task);
        core_hit(task);
        break;
    case CORE_GONE:
        task_kill(task);
        break;
    }
}
