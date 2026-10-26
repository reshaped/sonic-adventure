/* the Egg Walker's flames, breathed from the nozzle on its turret. every
 * other flame hurts what it touches, until it starts to dim */
#include <mathf.h>
#include <ninja.h>
#include <sg_xpt.h>
#include <stdlib.h>

#include "b_egm2/egg_walker.h"
#include "b_egm2/flames.h"
#include "b_egm2/parts.h"
#include "b_egm2/texlists.h"
#include "camera/camera.h"
#include "collision/collision.h"
#include "draw/motion.h"
#include "enemy/enemy.h"
#include "player/player.h"
#include "sound/sound.h"
#include "system/frame.h"
#include "system/task.h"
#include "system/util.h"

/* the boss's submodes in EGG_WALKER_FLAMES */
enum flames_mode { FLAMES_START, FLAMES_BREATHE };

enum flame_mode { FLAME_START, FLAME_FLY, FLAME_DOWN };

/* work->vars of a flame */
struct flame_vars {
    Float frame; /* its sprite frame */
    Sint32 life; /* the steps it has left */
};

#define VARS(work) ((struct flame_vars *)&(work)->vars)

/* a flame's record, which its task's extra points to */
struct flame {
    NJS_VECTOR velocity;
    Bool burns; /* TRUE when it has a sphere that hurts what it touches */
};

/* the boss stops breathing fire once player 0 comes within FLAMES_REACH
 * across the ground. otherwise it stops as its timer, set to FLAMES_TIME,
 * passes below 0. it breathes a flame on each step the timer is even */
#define FLAMES_TIME 180
#define FLAMES_REACH 90.0F

#define SE_FLAMES 0x1dc

#define STEPS_PER_SECOND 60.0F

#define QUARTER_TURN 0x4000
#define HALF_TURN 0x8000

/* each step, the turret covers TURRET_PACE of the short way to its aim.
 * the nozzle covers NOZZLE_PACE of its own */
#define TURRET_PACE 0.025F
#define NOZZLE_PACE 0.06F
/* each new aim of the turret puts it TURRET_SWEEP to one side of player 0,
 * by turns. each new aim of the nozzle tips it NOZZLE_TIP above or below
 * level, by turns */
#define TURRET_SWEEP NJM_DEG_ANG(40)
#define NOZZLE_TIP NJM_DEG_ANG(30)
/* the turret holds each aim for TURRET_HOLD seconds, and the nozzle for
 * NOZZLE_HOLD. each hold lasts up to HOLD_MORE seconds longer, at random */
#define TURRET_HOLD 0.5F
#define NOZZLE_HOLD 0.1F
#define HOLD_MORE 0.5F

/* a flame leaves the nozzle at FLAME_SPEED times a random share, from
 * SPEED_LEAST up to SPEED_LEAST + SPEED_MORE */
#define FLAME_SPEED 5.0F
#define SPEED_LEAST 0.75F
#define SPEED_MORE 0.25F

/* a flame lives FLAME_LIFE steps, and up to LIFE_MORE more at random. each
 * step it moves by its velocity, which then keeps FLAME_DRAG of itself and
 * gains FLAME_FALL in y. it stops once it comes down to FLAME_GROUND. over
 * its last FLAME_FADE steps, it dims and hurts nothing */
#define FLAME_LIFE (1.2F * STEPS_PER_SECOND)
#define LIFE_MORE 30.0F
#define FLAME_DRAG 0.975F
#define FLAME_FALL (-9.8F / 360.0F)
#define FLAME_GROUND 10.0F
#define FLAME_FADE 30.0F

/* a flame's sprite has FLAME_FRAMES frames. its animation starts at a
 * random frame and advances FLAME_PACE frames each step. it goes back to
 * frame 0 as it reaches the last, so the last never draws */
#define FLAME_FRAMES 16
#define FLAME_PACE 0.5F

/* a flame's scale is FLAME_SCALE, less FLAME_GROW for each GROW_STEPS steps
 * of its life above FLAME_LIFE - GROW_STEPS. so a flame of FLAME_LIFE
 * starts at FLAME_SCALE - FLAME_GROW, and one with more life starts
 * smaller. each grows to FLAME_SCALE and holds there */
#define FLAME_SCALE 15.0F
#define FLAME_GROW 10.0F
#define GROW_STEPS 60.0F

/* the nozzle's offset from part 0, before the body's and the turret's
 * turns */
static const NJS_POINT3 nozzle_offset = {21.9F, 40.0F, -2.0F};
static struct collision_shape flame_shape[1] = {
    {SHAPE_SPHERE | SHAPE_HURTABLE,
     SHAPE_HURTS_OBJECTS | SHAPE_HURTS_ENEMIES | SHAPE_HURTS_PLAYERS |
         SHAPE_GUARD | SHAPE_ATTACK,
     PUSH_PLAYERS,
     {0.0F, -3.0F, 0.0F},
     8.0F,
     0.0F,
     0.0F,
     {0, 0, 0},
     0,
     NULL},
};
/* the flame sprite's frames: frame n is the whole of egm2_fire_texlist's
 * texture n, bigfire01 to bigfire16 */
static NJS_TEXANIM flame_texanims[FLAME_FRAMES] = {
#include "b_egm2/flames/flame_texanims.inc"
};

/* whether the last flame made burns. flame_shoot makes every other one
 * burn */
static Sint8 last_flame_burns;
static NJS_SPRITE flame_sprite;

static void flames_aim(struct task *task);
static void flames_breathe(struct task *task);
static void nozzle_pos(NJS_POINT3 *pos, Angle body, Angle turret);
static void flame_run(struct task *task);
static void flame_draw(struct task *task);

void egg_walker_flames(struct task *task)
{
    struct task_work *work = task->work;
    struct egg_walker *boss = task->extra;
    int turn;

    switch (work->submode) {
    case FLAMES_START:
        EGG_WALKER_VARS(work)->timer = FLAMES_TIME;
        boss->turret_hold = 0;
        boss->nozzle_hold = 0;
        flames_aim(task);
        work->submode = FLAMES_BREATHE;
    case FLAMES_BREATHE:
        sound_play_held(SE_FLAMES, 0, 1, 0);
        turn = (Uint16)(boss->turret_aim - boss->turret_yaw);
        boss->turret_yaw += SHORT_TURN(turn) * TURRET_PACE;
        /* @hack spelling: turn assigned inside the sum, under which SHC takes
         * nozzle_aim's address before the turret's store and loads both
         * angles through registers (MOV #100,R4 at 0x0c904364, MOV.W @R4,R4
         * at 0x0c904380); as a statement of its own, 22 of 235 words
         * differ */
        boss->nozzle_pitch +=
            (turn = (Uint16)(boss->nozzle_aim - boss->nozzle_pitch),
             SHORT_TURN(turn)) *
            NOZZLE_PACE;
        if (--boss->turret_hold < 0) {
            flames_aim(task);
            boss->turret_aim += ((++boss->turret_sweeps & 1) == 0)
                                    ? TURRET_SWEEP
                                    : -TURRET_SWEEP;
            boss->turret_hold =
                (TURRET_HOLD + (rand() * (HOLD_MORE * RAND_SCALE))) *
                STEPS_PER_SECOND;
        }
        if (--boss->nozzle_hold < 0) {
            /* @hack spelling: the aim set level, to 0, and then tipped, as
             * the turret's is aimed at player 0 and then swept. SHC stores
             * the 0 (MOV.L R11,@(R0,R14) at 0x0c90443a) and reads it back
             * for the add (MOV.L @R1,R2 at 0x0c904482); as one store of the
             * tip, 105 of 235 words differ */
            boss->nozzle_aim = 0;
            boss->nozzle_aim +=
                ((++boss->nozzle_tips & 1) == 0) ? NOZZLE_TIP : -NOZZLE_TIP;
            boss->nozzle_hold =
                (NOZZLE_HOLD + (rand() * (HOLD_MORE * RAND_SCALE))) *
                STEPS_PER_SECOND;
        }
        if (EGG_WALKER_VARS(work)->timer % 2 == 0) {
            flames_breathe(task);
        }
        if (njSqrt(SQUARE(work->pos.x - player_work[0]->pos.x) +
                   SQUARE(work->pos.z - player_work[0]->pos.z)) <
                FLAMES_REACH ||
            --EGG_WALKER_VARS(work)->timer < 0) {
            sound_stop(SE_FLAMES);
            work->submode = MODE_DONE;
        }
        break;
    }
    if (work->submode == MODE_DONE) {
        if (work->mode != EGG_WALKER_WALK) {
            boss_next_mode = EGG_WALKER_WALK;
            boss_last_mode = work->mode;
        }
        work->submode = FLAMES_START;
    }
}

/* aim the turret at player 0 from part 0. then set turret_aim to aim it
 * again, from where the nozzle would be at that aim */
static void flames_aim(struct task *task)
{
    struct task_work *work = task->work;
    struct egg_walker *boss = task->extra;
    NJS_POINT3 to;
    Float yaw;

    VECTOR_DIFF(&to, &player_work[0]->pos, egg_walker_part_pos(0));
    yaw = QUARTER_TURN - (Angle)(atan2f(to.z, to.x) * ANGLE_PER_RADIAN);
    yaw = -yaw - work->ang.y + HALF_TURN;
    nozzle_pos(&to, work->ang.y, (Angle)yaw);
    VECTOR_DIFF(&to, &player_work[0]->pos, &to);
    yaw = QUARTER_TURN - (Angle)(atan2f(to.z, to.x) * ANGLE_PER_RADIAN);
    boss->turret_aim = -yaw - work->ang.y + HALF_TURN;
}

void egg_walker_flames_init(void)
{
    sprite_init(&flame_sprite, &egm2_fire_texlist, flame_texanims);
}

/* make a flame at *pos, flying at speed. yaw and pitch, in angle units,
 * give its heading and pitch. every other flame burns. it makes none when
 * no task or memory is free */
static void flame_shoot(NJS_POINT3 *pos, Float yaw, Float pitch, Float speed)
{
    struct task *task;
    struct task_work *work;
    struct flame *flame;

    /* @hack spelling: the task stored as it's tested, which tests the result
     * in R0 and copies it to R14 in the branch's slot (TST R0,R0 at
     * 0x0c904710, MOV R0,R14 at 0x0c904714); as two statements, SHC tests
     * R14 and 3 of 123 words differ */
    if ((task = task_create(TASK_WORK, 3, flame_run)) == NULL) {
        return;
    }
    work = task->work;
    flame = heap_calloc(1, sizeof(struct flame));
    if (flame == NULL) {
        task_kill(task);
        return;
    }
    task->extra = flame;
    enemy_alloc_record(task, work);
    VECTOR_COPY(&work->pos, pos);
    flame->velocity.y = speed * njSin((Angle)pitch);
    flame->velocity.x = speed * njCos((Angle)pitch) * njSin((Angle)yaw);
    flame->velocity.z = speed * njCos((Angle)pitch) * njCos((Angle)yaw);
    task->step = flame_run;
    task->draw = flame_draw;
    if (last_flame_burns == 0) {
        flame->burns = TRUE;
        collision_init(task, flame_shape, COLLISION_ENEMY, 1, 1);
        last_flame_burns++;
    } else {
        flame->burns = FALSE;
        last_flame_burns = 0;
    }
}

/* a flame's step. it flies until it comes down to FLAME_GROUND, and stays
 * there until its life runs out. a flame that burns registers its sphere
 * until its last FLAME_FADE steps */
static void flame_run(struct task *task)
{
    struct task_work *work = task->work;
    struct flame *flame = task->extra;

    switch (work->mode) {
    case FLAME_START:
        VARS(work)->life = FLAME_LIFE + (rand() * (LIFE_MORE * RAND_SCALE));
        VARS(work)->frame = rand() * (FLAME_FRAMES * RAND_SCALE);
        work->mode = FLAME_FLY;
    case FLAME_FLY:
        VECTOR_SUM(&work->pos, &work->pos, &flame->velocity);
        VECTOR_SCALE(&flame->velocity, &flame->velocity, FLAME_DRAG);
        flame->velocity.y += FLAME_FALL;
        if (work->pos.y <= FLAME_GROUND) {
            work->pos.y = FLAME_GROUND;
            work->mode = FLAME_DOWN;
        }
        break;
    case FLAME_DOWN:
        break;
    }
    if (flame->burns && VARS(work)->life >= FLAME_FADE) {
        collision_register(work);
    }
    VARS(work)->frame += FLAME_PACE;
    if ((int)VARS(work)->frame >= FLAME_FRAMES - 1.0F) {
        VARS(work)->frame = 0.0F;
    }
    if (--VARS(work)->life < 0) {
        task_kill(task);
    }
    flame_draw(task);
}

/* draw the flame's sprite on a frame's first step, facing the camera. its
 * color adds to what lies behind it */
static void flame_draw(struct task *task)
{
    struct task_work *work = task->work;
    NJS_ARGB color;
    Angle ang;
    Float until_grown;

    if (frame_step != 0) {
        return;
    }
    njPushMatrix(NULL);
    color.a = 1.0F;
    if (VARS(work)->life >= FLAME_FADE) {
        color.r = color.g = color.b = 1.0F;
    } else {
        Float fade = VARS(work)->life * (1.0F / FLAME_FADE);

        color.r = color.g = color.b = fade;
    }
    njSetConstantMaterial(&color);
    njColorBlendingMode(NJD_SOURCE_COLOR, NJD_COLOR_BLENDING_SRCALPHA);
    njColorBlendingMode(NJD_DESTINATION_COLOR, NJD_COLOR_BLENDING_ONE);
    njTranslateV(NULL, &work->pos);
    TURN_VIA(njRotateY, camera_work->ang.y, ang);
    TURN_VIA(njRotateX, camera_work->ang.x + HALF_TURN, ang);
    /* @hack spelling: until_grown stored as it's tested, under which SHC sums
     * the scale in FR3 (FMOV.S @R0,FR3 at 0x0c904a04, FMAC FR0,FR15,FR3 at
     * 0x0c904a10); as two statements, it takes FR2 and 3 of 153 words
     * differ */
    if ((until_grown = VARS(work)->life - (FLAME_LIFE - GROW_STEPS)) > 0.0F) {
        work->scl.x =
            ((1.0F - (until_grown * (1.0F / GROW_STEPS))) * FLAME_GROW) +
            (FLAME_SCALE - FLAME_GROW);
    } else {
        work->scl.x = FLAME_SCALE;
    }
    njScale(NULL, work->scl.x, work->scl.x, work->scl.x);
    njDrawSprite3D(&flame_sprite, (int)VARS(work)->frame,
                   NJD_SPRITE_ALPHA | NJD_SPRITE_COLOR);
    color.a = 0.0F;
    color.r = 0.0F;
    color.g = 0.0F;
    color.b = 0.0F;
    njSetConstantMaterial(&color);
    njColorBlendingMode(NJD_SOURCE_COLOR, NJD_COLOR_BLENDING_BOTHSRCALPHA);
    njColorBlendingMode(NJD_DESTINATION_COLOR, NJD_COLOR_BLENDING_BOTHSRCALPHA);
    njPopMatrix(1);
}

/* breathe a flame from the nozzle, along the turret's heading and the
 * nozzle's pitch */
static void flames_breathe(struct task *task)
{
    struct task_work *work = task->work;
    struct egg_walker *boss = task->extra;
    NJS_POINT3 pos;

    nozzle_pos(&pos, work->ang.y, boss->turret_yaw);
    flame_shoot(
        &pos, -work->ang.y - boss->turret_yaw + HALF_TURN, -boss->nozzle_pitch,
        (SPEED_LEAST + (rand() * (SPEED_MORE * RAND_SCALE))) * FLAME_SPEED);
}

/* set *pos to the nozzle's place in the world. body is the boss's heading,
 * and turret the turret's turn. it allocates room for a matrix, and frees
 * it unused */
static void nozzle_pos(NJS_POINT3 *pos, Angle body, Angle turret)
{
    Angle ang;
    void *matrix = heap_alloc(sizeof(NJS_MATRIX));

    njPushMatrix(&_unit_matrix_);
    njTranslateV(NULL, egg_walker_part_pos(0));
    TURN_VIA(njRotateY, HALF_TURN - body, ang);
    TURN_VIA(njRotateY, -turret, ang);
    njCalcPoint(NULL, (NJS_POINT3 *)&nozzle_offset, pos);
    njPopMatrix(1);
    heap_free(matrix);
}
