/* Chaos 4's split. he turns into a blob, goes under, and four pieces of
 * him leap out of the swamp. one after another, each hops at player 0
 * three times and sinks. once the last has sunk, the pieces melt into
 * drops that flow back to him, and he forms again from the blob */
#include <mathf.h>
#include <ninja.h>
#include <sg_xpt.h>
#include <stdlib.h>

#include "b_chaos4/boss.h"
#include "b_chaos4/camera.h"
#include "b_chaos4/split.h"
#include "b_chaos4/swamp.h"
#include "b_chaos4/textures.h"
#include "b_chaos4/util.h"
#include "camera/camera.h"
#include "chaos/chaos.h"
#include "chaos/effect.h"
#include "chaos/emeralds.h"
#include "collision/collision.h"
#include "draw/motion.h"
#include "draw/tree.h"
#include "effect/brain.h"
#include "enemy/enemy.h"
#include "player/player.h"
#include "sound/sound.h"
#include "system/frame.h"
#include "system/task.h"
#include "system/util.h"

/* his submodes in BOSS_SPLIT. he waits in SPLIT_PIECES until the last
 * piece to hop has sunk and set SPLIT_GATHER */
enum split_mode {
    SPLIT_START = 0,
    SPLIT_BLOB = 1,
    SPLIT_PIECES = 3,
    SPLIT_GATHER = 10,
    SPLIT_WAIT = 11,
    SPLIT_FORM = 12
};

/* each phase ends on the step its countdown passes below 0. BLOB_TIME
 * counts the blob before he splits, and WAIT_TIME the wait from
 * SPLIT_GATHER until the blob shows again. FORM_TIME counts his growth out
 * of the blob, which takes his scale from 0 by 1 / FORM_TIME a step */
#define BLOB_TIME 120
#define WAIT_TIME 90
#define FORM_TIME 150

#define PIECES 4

#define HALF_TURN NJM_DEG_ANG(180)

/* gravity adds PIECE_FALL to a piece's speed's y each step, except while
 * it waits or lies sunk. a piece waits under the swamp before it hops,
 * through a countdown from its index times PIECE_DELAY, and then jumps out
 * at PIECE_JUMP */
#define PIECE_FALL (-9.8F / 60.0F)
#define PIECE_DELAY 60.0F
#define PIECE_JUMP 3.0F
/* each time a piece lands on the swamp's surface, it hops up at PIECE_HOP.
 * its first hop runs straight at player 0, at PIECE_RUN in x and z. each
 * later hop moves its speed in x and z PIECE_STEER of the way to that run
 * at him. it sinks at its landing after PIECE_HOPS hops */
#define PIECE_HOPS 3
#define PIECE_RUN 2.5F
#define PIECE_STEER 0.3F
#define PIECE_HOP 1.5F
/* the chance a step that a hopping piece runs chaos_effect_trail at its
 * place. a trail from a place to itself is one trail piece and one
 * spark */
#define TRAIL_CHANCE 0.3F
#define SUNK_Y (-20.0F) /* the depth a sinking piece stops at */
/* at SPLASH_Y or above, a piece splashes on the swamp's surface each step,
 * within SPLASH_SPREAD of it in x and z */
#define SPLASH_Y 19.0F
#define SPLASH_SPREAD 5.0F
/* the drops a piece's landing throws up, and their size */
#define SPLASH_DROPS 64
#define SPLASH_SIZE 1.5F
/* a landing sets a piece's wobble's speed to WOBBLE_KICK in x and z, and to
 * -WOBBLE_KICK in y. each step the speed keeps WOBBLE_DAMP of itself, and
 * loses WOBBLE_SPRING of the wobble, which pulls the piece back toward
 * round */
#define WOBBLE_KICK 0.05F
#define WOBBLE_DAMP 0.996F
#define WOBBLE_SPRING 0.318F
#define PIECE_DROPS 100 /* the drops each piece melts into */
#define PIECE_ATTACK 2 /* the attack level of a piece's shape */

/* a drop flies out in a random direction at up to DROP_SPEED. its flow's
 * countdown starts from DROP_TIME, less up to DROP_TIME_SPREAD at random.
 * each step its speed keeps DROP_DRAG of itself, and gains DROP_PULL of the
 * gap from the drop to Chaos 4 */
#define DROP_SPEED 5.0F
#define DROP_TIME 150.0F
#define DROP_TIME_SPREAD 30.0F
#define DROP_DRAG 0.93F
#define DROP_PULL 0.001F

/* a piece's record, at its task's extra */
struct split_piece {
    NJS_VECTOR speed;
    Sint32 index; /* its place in the order the pieces hop in, from 0 */
    /* what its draw adds along each axis to its round scale of 0.5, which
     * squashes it */
    NJS_VECTOR wobble;
    NJS_VECTOR wobble_speed; /* how far its wobble moves a step */
    struct task_work *parent; /* Chaos 4's work */
};

struct split_piece_vars {
    Uint8 unk_00[4];
    Sint32 timer; /* the countdown to its first hop */
    Sint32 landings; /* its landings on the swamp's surface since it jumped */
};

#define PIECE_VARS(work) ((struct split_piece_vars *)&(work)->vars)

enum split_piece_mode {
    PIECE_START,
    PIECE_THROWN,
    PIECE_WAIT,
    PIECE_HOPPING,
    PIECE_SINK,
    PIECE_SUNK
};

/* a returning drop's record, at its task's extra */
struct return_drop {
    Uint8 unk_00[12];
    NJS_VECTOR speed;
};

struct return_drop_vars {
    Uint8 unk_00[4];
    Sint32 timer; /* the countdown to the end of its flow */
};

#define DROP_VARS(work) ((struct return_drop_vars *)&(work)->vars)

/* a drop's flow ends in DROP_GONE, whose step kills the drop. no step sets
 * DROP_END, whose case kills the drop as DROP_GONE's does */
enum return_drop_mode { DROP_START, DROP_FLOW, DROP_END, DROP_GONE };

static void split_piece_create(struct task_work *parent, int index);
static void split_piece_step(struct task *task);
static void split_piece_wobble(struct task *task);
static void split_piece_draw(struct task *task);
static void split_piece_destroy(struct task *task);
static void return_drop_create(NJS_POINT3 *pos, NJS_POINT3 *to);
static void return_drop_step(struct task *task);
static void return_drop_draw(struct task *task);
static void return_drop_destroy(struct task *task);

static NJS_MATERIAL piece_materials[1] = {
#include "b_chaos4/split/piece_materials.inc"
};

static Sint16 piece_meshes0[132] = {
#include "b_chaos4/split/piece_meshes0.inc"
};

static Sint16 piece_meshes1[1760] = {
#include "b_chaos4/split/piece_meshes1.inc"
};

static NJS_COLOR piece_uvs0[132] = {
#include "b_chaos4/split/piece_uvs0.inc"
};

static NJS_COLOR piece_uvs1[1760] = {
#include "b_chaos4/split/piece_uvs1.inc"
};

static NJS_MESHSET piece_meshsets[2] = {
#include "b_chaos4/split/piece_meshsets.inc"
};

static NJS_POINT3 piece_points[464] = {
#include "b_chaos4/split/piece_points.inc"
};

static NJS_VECTOR piece_normals[464] = {
#include "b_chaos4/split/piece_normals.inc"
};

static NJS_MODEL piece_model = {
#include "b_chaos4/split/piece_model.inc"
};

static NJS_OBJECT piece_object = {
#include "b_chaos4/split/piece_object.inc"
};

/* the speed each piece is thrown up out of him at, by its index */
static const NJS_VECTOR piece_speeds[PIECES] = {
    {1.0F, 3.0F, 0.0F},
    {-1.0F, 3.0F, 0.0F},
    {0.0F, 3.0F, 1.0F},
    {0.0F, 3.0F, -1.0F},
};

/* a piece's ball, which hurts players. it pushes every group, and no group
 * pushes it */
static struct collision_shape piece_shape[1] = {
    {SHAPE_SPHERE | SHAPE_HURTABLE,
     SHAPE_HURTS_PLAYERS | PIECE_ATTACK,
     PUSH_ALL | PUSH_FIRM(PUSH_ALL),
     {0.0F, 0.0F, 0.0F},
     8.0F,
     0.0F,
     0.0F,
     {0, 0, 0},
     0,
     NULL},
};

void chaos4_split(struct task_work *work, struct task_motion *motion,
                  void *extra)
{
    struct chaos *chaos = extra;
    struct chaos4 *record = CHAOS4_VARS(work)->record;
    int i;

    switch (work->submode) {
    case SPLIT_START:
        chaos4_set_shapes(work, SHAPES_ON);
        chaos->target.y = SWAMP_Y;
        swamp_ripple_add(&work->pos);
        /* @hack spelling: the action set through extra, cast, not chaos, which
         * keeps a second copy of extra on the stack (MOV.L R6,@R15 at
         * 0x0c905032) and stores the action through it (0x0c905084). through
         * chaos, 106 of 190 words differ */
        ((struct chaos *)extra)->motion.wanted = ACTION_RISE;
        CHAOS4_VARS(work)->timer = BLOB_TIME;
        record->draw &= ~DRAW_BODY;
        record->draw |= DRAW_BLOB;
        work->scl.x = work->scl.y = work->scl.z = 1.0F;
        chaos4_emeralds_create();
        brain_shown = 0;
        work->submode = SPLIT_BLOB;
    case SPLIT_BLOB:
        if (--CHAOS4_VARS(work)->timer < 0) {
            record->draw &= ~DRAW_BLOB;
            work->pos.y = 0.0F;
            chaos4_set_shapes(work, SHAPES_OFF);
            for (i = 0; i < PIECES; i++) {
                split_piece_create(work, i);
            }
            work->submode = SPLIT_PIECES;
        }
        break;
    case SPLIT_PIECES:
        break;
    case SPLIT_GATHER:
        CHAOS4_VARS(work)->timer = WAIT_TIME;
        work->submode = SPLIT_WAIT;
        break;
    case SPLIT_WAIT:
        if (--CHAOS4_VARS(work)->timer < 0) {
            record->draw |= DRAW_BLOB;
            work->scl.x = work->scl.y = work->scl.z = 0.0F;
            chaos4_set_shapes(work, SHAPES_ON);
            CHAOS4_VARS(work)->timer = FORM_TIME;
            chaos_emerald_return_all();
            work->submode = SPLIT_FORM;
        }
        break;
    case SPLIT_FORM:
        work->scl.x = work->scl.y = work->scl.z += 1.0F / FORM_TIME;
        if (--CHAOS4_VARS(work)->timer < 0) {
            record->draw |= DRAW_BODY;
            record->draw &= ~DRAW_BLOB;
            brain_shown = 1;
            work->submode = MODE_DONE;
        }
        break;
    }
    if (work->submode == MODE_DONE) {
        if (work->mode != BOSS_SWIM) {
            boss_next_mode = BOSS_SWIM;
            boss_last_mode = work->mode;
        }
        work->submode = SPLIT_START;
    }
}

/* make a piece at the position of parent, Chaos 4's work. index is its
 * place in the order the pieces hop in */
static void split_piece_create(struct task_work *parent, int index)
{
    struct task *task;
    struct task_work *work;
    struct split_piece *piece;

    if ((task = task_create(TASK_WORK, 3, split_piece_step)) != NULL) {
        work = task->work;
        enemy_alloc_record(task, work);
        task->extra = piece = heap_calloc(1, sizeof(struct split_piece));
        VECTOR_COPY(&work->pos, &parent->pos);
        piece->index = index;
        piece->parent = parent;
        task->step = split_piece_step;
        task->draw = split_piece_draw;
        task->destructor = split_piece_destroy;
        collision_init(task, piece_shape, COLLISION_OBJECT, 1, 1);
    }
}

/* a piece is thrown up out of him, falls back under and waits. it then
 * jumps out and hops at player 0 until it sinks, and the fight's camera
 * aims at it while it hops. once his submode reaches SPLIT_GATHER, it melts
 * into drops that flow back to him. at SPLASH_Y or above, it splashes round
 * itself */
static void split_piece_step(struct task *task)
{
    struct task_work *work = task->work;
    struct split_piece *piece = task->extra;
    NJS_VECTOR to;
    Float distance;
    struct task_work *player;
    int i;

    switch (work->mode) {
    case PIECE_START:
        /* @hack spelling: the speed copied in three statements, which form the
         * z read's address in R4 (ADD R3,R4 at 0x0c9052fa); as one
         * VECTOR_COPY, 2 of 425 words differ */
        piece->speed.x = piece_speeds[piece->index].x;
        piece->speed.y = piece_speeds[piece->index].y;
        piece->speed.z = piece_speeds[piece->index].z;
        PIECE_VARS(work)->timer = piece->index * PIECE_DELAY;
        work->mode = PIECE_THROWN;
    case PIECE_THROWN:
        piece->speed.y += PIECE_FALL;
        if (piece->speed.x != 0.0F) {
            work->ang.y = (Angle)(atan2f(piece->speed.z, piece->speed.x) *
                                  ANGLE_PER_RADIAN);
        } else {
            work->ang.y = 0;
        }
        if (work->pos.y <= 0.0F && piece->speed.y <= 0.0F) {
            work->pos.y = 0.0F;
            piece->speed.x = piece->speed.y = piece->speed.z = 0.0F;
            work->mode = PIECE_WAIT;
        }
        break;
    case PIECE_WAIT:
        if (--PIECE_VARS(work)->timer < 0) {
            piece->speed.y = PIECE_JUMP;
            work->mode = PIECE_HOPPING;
        }
        break;
    case PIECE_HOPPING:
        chaos4_camera_look_at(&work->pos);
        if (rand() * RAND_SCALE <= TRAIL_CHANCE) {
            chaos_effect_trail(&work->pos, &work->pos, 1);
        }
        piece->speed.y += PIECE_FALL;
        work->ang.y =
            (Angle)(atan2f(piece->speed.z, piece->speed.x) * ANGLE_PER_RADIAN);
        if (piece->speed.y < 0.0F && work->pos.y <= SWAMP_Y) {
            sound_play(SE_SPLASH, 0, 0, 0);
            chaos_effect_splash(work, SPLASH_DROPS, SPLASH_SIZE, 0.0F);
            piece->wobble_speed.x = WOBBLE_KICK;
            piece->wobble_speed.z = WOBBLE_KICK;
            piece->wobble_speed.y = -WOBBLE_KICK;
            if (++PIECE_VARS(work)->landings > PIECE_HOPS) {
                work->mode = PIECE_SINK;
            } else {
                player = player_work[0];
                VECTOR_DIFF(&to, &player->pos, &work->pos);
                distance = njSqrt(SQUARE(to.x) + SQUARE(to.z));
                if (PIECE_VARS(work)->landings <= 1) {
                    piece->speed.x = to.x / distance * PIECE_RUN;
                    piece->speed.z = to.z / distance * PIECE_RUN;
                } else {
                    piece->speed.x +=
                        ((to.x / distance * PIECE_RUN) - piece->speed.x) *
                        PIECE_STEER;
                    piece->speed.z +=
                        ((to.z / distance * PIECE_RUN) - piece->speed.z) *
                        PIECE_STEER;
                }
                piece->speed.y = PIECE_HOP;
            }
        }
        break;
    case PIECE_SINK:
        piece->speed.y += PIECE_FALL;
        if (work->pos.y <= SUNK_Y) {
            piece->speed.x = piece->speed.y = piece->speed.z = 0.0F;
            if (piece->index == PIECES - 1) {
                piece->parent->submode = SPLIT_GATHER;
            }
            work->mode = PIECE_SUNK;
        }
        break;
    case PIECE_SUNK:
        if (piece->parent->submode >= SPLIT_GATHER) {
            for (i = 0; i < PIECE_DROPS; i++) {
                return_drop_create(&work->pos, &piece->parent->pos);
            }
            task_kill(task);
        }
        break;
    }
    VECTOR_SUM(&work->pos, &work->pos, &piece->speed);
    split_piece_wobble(task);
    collision_register(work);
    split_piece_draw(task);
    if (work->pos.y >= SPLASH_Y) {
        to.y = SWAMP_Y;
        to.x = work->pos.x + (rand() * (2 * SPLASH_SPREAD * RAND_SCALE)) -
               SPLASH_SPREAD;
        to.z = work->pos.z + (rand() * (2 * SPLASH_SPREAD * RAND_SCALE)) -
               SPLASH_SPREAD;
        chaos4_splash_create(&to);
    }
}

/* step the piece's wobble, which springs back toward round and dies
 * away */
static void split_piece_wobble(struct task *task)
{
    struct split_piece *piece = task->extra;
    NJS_VECTOR pull;

    VECTOR_SCALE(&piece->wobble_speed, &piece->wobble_speed, WOBBLE_DAMP);
    VECTOR_SCALE(&pull, &piece->wobble, WOBBLE_SPRING);
    VECTOR_DIFF(&piece->wobble_speed, &piece->wobble_speed, &pull);
    VECTOR_SUM(&piece->wobble, &piece->wobble, &piece->wobble_speed);
}

/* on a frame's first step, draw the piece turned along its heading, at a
 * scale of 0.5 plus its wobble. it is added to what lies behind it */
static void split_piece_draw(struct task *task)
{
    struct task_work *work = task->work;
    struct split_piece *piece = task->extra;
    Angle ang;

    if (frame_step == 0) {
        chaos4_material_reset();
        njSetTexture(&chaos4_common_texlist);
        njColorBlendingMode(NJD_SOURCE_COLOR, NJD_COLOR_BLENDING_SRCALPHA);
        njColorBlendingMode(NJD_DESTINATION_COLOR, NJD_COLOR_BLENDING_ONE);
        njPushMatrix(NULL);
        njTranslateV(NULL, &work->pos);
        TURN_VIA(njRotateY, HALF_TURN - work->ang.y, ang);
        njScale(NULL, piece->wobble.x + 0.5F, piece->wobble.y + 0.5F,
                piece->wobble.z + 0.5F);
        draw_tree(&piece_object);
        njPopMatrix(1);
    }
}

static void split_piece_destroy(struct task *task)
{
}

/* make a drop that flies out of a piece at pos, and flows back to Chaos
 * 4. to is unread: the drop flows to chaos4_pos. it makes none when it
 * can't allocate its task or its record */
static void return_drop_create(NJS_POINT3 *pos, NJS_POINT3 *to)
{
    struct task *task;
    struct task_work *work;
    struct return_drop *drop;

    if ((task = task_create(TASK_WORK, 3, return_drop_step)) != NULL) {
        work = task->work;
        drop = heap_calloc(1, sizeof(struct return_drop));
        if (drop == NULL) {
            task_kill(task);
            return;
        }
        enemy_alloc_record(task, work);
        task->extra = drop;
        VECTOR_COPY(&work->pos, pos);
        task->step = return_drop_step;
        task->draw = return_drop_draw;
        task->destructor = return_drop_destroy;
    }
}

/* a drop flies out in a random direction, slows, and is pulled toward
 * him, until its countdown passes below 0 */
static void return_drop_step(struct task *task)
{
    struct task_work *work = task->work;
    struct return_drop *drop = task->extra;
    Float pitch;
    Float yaw;
    Float speed;
    NJS_VECTOR pull;

    switch (work->mode) {
    case DROP_START:
        pitch = RANDOM_ANGLE();
        yaw = RANDOM_ANGLE();
        speed = rand() * (DROP_SPEED * RAND_SCALE);
        drop->speed.y = speed * njSin((Angle)pitch);
        drop->speed.x = speed * njCos((Angle)pitch) * njSin((Angle)yaw);
        drop->speed.z = speed * njCos((Angle)pitch) * njCos((Angle)yaw);
        DROP_VARS(work)->timer =
            DROP_TIME - (int)(rand() * (DROP_TIME_SPREAD * RAND_SCALE));
        work->mode = DROP_FLOW;
        break;
    case DROP_FLOW:
        VECTOR_SCALE(&drop->speed, &drop->speed, DROP_DRAG);
        VECTOR_DIFF(&pull, chaos4_pos, &work->pos);
        VECTOR_SCALE(&pull, &pull, DROP_PULL);
        VECTOR_SUM(&drop->speed, &drop->speed, &pull);
        VECTOR_SUM(&work->pos, &work->pos, &drop->speed);
        if (--DROP_VARS(work)->timer < 0) {
            work->mode = DROP_GONE;
        }
        break;
    case DROP_END:
    case DROP_GONE:
        task_kill(task);
        break;
    }
    return_drop_draw(task);
}

/* on a frame's first step, draw chaos4_drop_sprite at the drop, added to
 * what lies behind it and facing the camera */
static void return_drop_draw(struct task *task)
{
    struct task_work *work = task->work;
    Angle ang;
    NJS_ARGB color;

    if (frame_step == 0) {
        chaos4_material_reset();
        njPushMatrix(NULL);
        color.a = 0.75F;
        color.r = 0.5F;
        color.g = 0.5F;
        color.b = 0.5F;
        njSetConstantMaterial(&color);
        njColorBlendingMode(NJD_SOURCE_COLOR, NJD_COLOR_BLENDING_ONE);
        njColorBlendingMode(NJD_DESTINATION_COLOR, NJD_COLOR_BLENDING_ONE);
        njTranslateV(NULL, &work->pos);
        TURN_VIA(njRotateY, camera_work->ang.y, ang);
        TURN_VIA(njRotateX, camera_work->ang.x, ang);
        njScale(NULL, 1.0F, 1.0F, 1.0F);
        njDrawSprite3D(&chaos4_drop_sprite, 0,
                       NJD_SPRITE_ALPHA | NJD_SPRITE_COLOR);
        color.a = 0.0F;
        color.r = 1.0F;
        color.g = 1.0F;
        color.b = 1.0F;
        njSetConstantMaterial(&color);
        njColorBlendingMode(NJD_SOURCE_COLOR, NJD_COLOR_BLENDING_BOTHSRCALPHA);
        njColorBlendingMode(NJD_DESTINATION_COLOR,
                            NJD_COLOR_BLENDING_BOTHSRCALPHA);
        njPopMatrix(1);
    }
}

static void return_drop_destroy(struct task *task)
{
}
