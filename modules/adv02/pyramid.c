/* the pyramid in the jungle, which rises once Sonic's flag is set, opening the
 * way into the ruin */
#include <mathf.h>
#include <ninja.h>
#include <sg_xpt.h>
#include <stdlib.h>

#include "adv02/pyramid.h"
#include "adv02/texlists.h"
#include "collision/world.h"
#include "debug/editor.h"
#include "debug/print.h"
#include "draw/model.h"
#include "effect/dust.h"
#include "player/player.h"
#include "progress/progress.h"
#include "stage/stage.h"
#include "system/task.h"
#include "system/util.h"

/* pyramid_object is the collision model, and pyramid_model the one drawn */
extern NJS_OBJECT pyramid_object;
extern NJS_MODEL pyramid_model;
extern NJS_MODEL pyramid_rubble_model;

#define PYRAMID_X (-515.83F)
#define PYRAMID_Y 50.0F
#define PYRAMID_Z (-1179.22F)

/* the height the pyramid rises, and the steps it takes */
#define RISE 40.0F
#define RISE_STEPS 1000

/* pyramid_update and pyramid_draw do nothing while no player is this near */
#define DRAW_RANGE 3000.0F
/* the collision world tests the pyramid only while a player is this near */
#define NEAR 390.0F

/* each face is a triangle, FACE_HEIGHT from its foot up to its tip and twice
 * FACE_HALF across its foot, leaning back from upright by pyramid_slope */
#define FACE_HEIGHT 300.0F
#define FACE_HALF 170.0F
#define FACES 3

/* pieces of rubble in the pool, and the steps each piece lasts */
#define RUBBLE_COUNT 150
#define RUBBLE_STEPS 200
/* a piece at or below this height, in the pyramid's frame, is freed */
#define RUBBLE_FLOOR (-5.0F)

/* the pyramid is a huge, solid object. for Sonic, whose flag raises it, it
 * also moves: its collision mesh is made again each step */
#define SONIC_FLAGS (WORLD_HUGE | WORLD_MOVES | WORLD_SOLID)
#define OTHER_FLAGS (WORLD_HUGE | WORLD_SOLID)

enum pyramid_mode { PYRAMID_INIT, PYRAMID_RUN };

/* pyramid states, in work->tick. work->timer counts the steps of the rise, as
 * a Uint16 */
enum pyramid_state { PYRAMID_DOWN, PYRAMID_RISING, PYRAMID_UP };

#define STATE(work) ((work)->tick)

struct rubble {
    NJS_POINT3 pos;
    NJS_VECTOR speed;
    struct angles ang;
    struct angles spin; /* the change in ang each step */
    Uint16 life; /* the steps left */
    Float scale;
    struct rubble *next; /* in the live or the free list */
};

/* the falling pieces, listed from live, and the free ones, listed from free */
struct rubble_pool {
    struct rubble *live;
    struct rubble *free;
    struct rubble pieces[RUBBLE_COUNT];
};

struct pyramid_vars {
    NJS_OBJECT *object; /* the pyramid's copy in the collision world */
    Float height; /* how far it has risen */
    struct rubble_pool *rubble; /* from the heap */
};

#define VARS(work) ((struct pyramid_vars *)&(work)->vars)
#define FORCE(task) ((struct task_force *)(task)->force)

static void pyramid_init(struct task *task);
static void pyramid_start(struct task *task);
static void pyramid_add_to_world(struct task *task);
static void pyramid_update(struct task *task);
static void pyramid_draw(struct task *task);
static void rubble_init(struct rubble_pool *pool);
static void rubble_update(struct task_work *work);
static void rubble_move(struct task_work *work, struct rubble *piece);
static void rubble_draw(struct rubble_pool *pool);
static void rubble_add(struct rubble_pool *pool, NJS_POINT3 *pos,
                       NJS_VECTOR *speed, struct angles *ang,
                       struct angles *spin, Float scale, int life);
static void pyramid_destroy(struct task *task);
static Bool pyramid_risen(void);

/* the dust the rise raises along two sides of the pyramid's foot, in the
 * pyramid's frame */
static Float pyramid_dust_y = 40.0F; /* both sides' height */
static Float pyramid_dust_x = 130.0F; /* one side lies at x -pyramid_dust_x */
static Float pyramid_dust_z = 135.0F; /* and the other at z pyramid_dust_z */
/* how far either way along its side, in z and in x, a side's puff starts */
static Float pyramid_dust_spread_z = 140.0F;
static Float pyramid_dust_spread_x = 140.0F;
/* the chance each side raises a puff on each step of the rise */
static Float pyramid_dust_chance = 0.17F;
static Float pyramid_dust_speed = 3.5F; /* a puff's speed out from its side */
/* the growth, rate, drag and lift pyramid_update stores into pyramid_dust and
 * pyramid_trail each step. every live puff reads its params from them */
static Float pyramid_dust_growth = 0.04F;
static Float pyramid_dust_rate = 0.04F;
static Float pyramid_dust_drag = 0.2F;
static Float pyramid_dust_lift = 0.03F;
static Float pyramid_trail_growth = 0.04F;
static Float pyramid_trail_rate = 0.08F;
static Float pyramid_trail_drag = 0.05F;
static Float pyramid_trail_lift = 0.04F;
/* the chance of a new piece of rubble on each step of the rise. a piece puffs
 * its trail every pyramid_trail_steps steps of its life */
static Float pyramid_rubble_chance = 0.17F;
static int pyramid_trail_steps = 15;
static struct dust_params pyramid_dust = {
    1.0F,
    0.1F,
    0.06F,
    0.2F,
    0.05F,
    {0.0F, 0.0F, 0.0F},
    {0.0F, 0.0F, 0.0F},
    {1.0F, 1.0F, 1.0F, 1.0F},
};
static struct dust_params pyramid_trail = {
    1.0F,
    0.1F,
    0.08F,
    0.2F,
    0.3F,
    {0.0F, 0.0F, 0.0F},
    {0.0F, 0.0F, 0.0F},
    {0.4F, 1.0F, 1.0F, 1.0F},
};

/* the flag that raises the pyramid, by character: only Sonic has one */
static int pyramid_flags[CHARACTER_COUNT] = {
    FLAG_SONIC_MR_ENTRANCE_RUIN, -1, -1, -1, -1, -1, -1, -1,
};

/* a face, from its tip: the vector down to one corner of its foot, and the
 * vector across the foot to the other */
static NJS_VECTOR pyramid_face_down = {-FACE_HALF, -FACE_HEIGHT, FACE_HALF};
static NJS_VECTOR pyramid_face_across = {FACE_HALF * 2, 0.0F, 0.0F};
/* the y angles that turn the face above to each face rubble bounces off */
static Angle pyramid_faces[FACES] = {0, 0x4000, 0xc000};

/* the faces' lean back from upright, set by pyramid_start */
static Angle pyramid_slope;

void pyramid_step(struct task *task)
{
    struct task_work *work = task->work;

    if (!placement_editing(task)) {
        switch (work->mode) {
        case PYRAMID_INIT:
            pyramid_init(task);
            break;
        case PYRAMID_RUN:
            pyramid_draw(task);
            pyramid_update(task);
            break;
        }
    } else {
        switch (work->mode) {
        case PYRAMID_INIT:
            pyramid_init(task);
            break;
        case PYRAMID_RUN:
            debug_print(EDITOR_LABEL(EDITOR_ROW_ANG_Y), "-> ROTATE Y");
            pyramid_draw(task);
            pyramid_update(task);
            break;
        }
    }
}

static void pyramid_init(struct task *task)
{
    struct task_work *work = task->work;

    task->draw = pyramid_draw;
    task->destructor = pyramid_destroy;
    pyramid_start(task);
    work->mode = PYRAMID_RUN;
}

/* put the pyramid at its fixed place, at y angle 0, and set pyramid_slope from
 * the faces' sizes. then allocate the rubble pool from the heap, and put the
 * pyramid in the collision world. it starts risen if its flag is already set */
static void pyramid_start(struct task *task)
{
    struct task_work *work = task->work;

    pyramid_slope = (Angle)(atan2f(FACE_HALF, FACE_HEIGHT) * ANGLE_PER_RADIAN);
    work->pos.x = PYRAMID_X;
    work->pos.y = PYRAMID_Y;
    work->pos.z = PYRAMID_Z;
    work->ang.y = 0;
    VARS(work)->rubble = heap_alloc(sizeof(struct rubble_pool));
    pyramid_add_to_world(task);
    rubble_init(VARS(work)->rubble);
    if (pyramid_risen()) {
        VARS(work)->height = RISE;
        STATE(work) = PYRAMID_UP;
    } else {
        VARS(work)->height = 0.0F;
        STATE(work) = PYRAMID_DOWN;
    }
}

/* add a copy of the pyramid's model to the collision world at its position,
 * with no turn or speed in the task's force */
static void pyramid_add_to_world(struct task *task)
{
    NJS_OBJECT *from = &pyramid_object;
    struct task_work *work = task->work;
    NJS_OBJECT *object;

    FORCE(task)->turn.x = 0;
    FORCE(task)->turn.y = 0;
    FORCE(task)->turn.z = 0;
    FORCE(task)->speed.x = 0.0F;
    FORCE(task)->speed.y = 0.0F;
    FORCE(task)->speed.z = 0.0F;
    object = world_object_alloc();
    VARS(work)->object = object;
    object->evalflags = from->evalflags;
    object->scl[0] = 1.0F;
    object->scl[1] = 1.0F;
    object->scl[2] = 1.0F;
    object->ang[0] = 0;
    object->ang[1] = work->ang.y;
    object->ang[2] = 0;
    object->pos[0] = work->pos.x;
    object->pos[1] = work->pos.y;
    object->pos[2] = work->pos.z;
    object->model = from->model;
    object->child = NULL;
    object->sibling = NULL;
    if (get_character() == CHARACTER_SONIC) {
        world_add(SONIC_FLAGS, task, object);
    } else {
        world_add(OTHER_FLAGS, task, object);
    }
}

/* once the flag is set, rise RISE over RISE_STEPS, shaken by three sine swings.
 * each step of the rise raises dust along two sides of the foot, and may drop a
 * piece of rubble down a face. a risen pyramid whose flag is clear drops back
 * at once. the collision copy follows the height */
static void pyramid_update(struct task *task)
{
    struct task_work *work = task->work;
    struct rubble_pool *pool = VARS(work)->rubble;
    NJS_POINT3 pos;
    NJS_VECTOR speed;
    struct angles ang;
    struct angles spin;
    Angle turn;
    Float down;
    Float across;

    if (player_in_range(&work->pos, DRAW_RANGE) == 0) {
        return;
    }
    pyramid_dust.growth = pyramid_dust_growth;
    pyramid_dust.rate = pyramid_dust_rate;
    pyramid_dust.drag = pyramid_dust_drag;
    pyramid_dust.lift = pyramid_dust_lift;
    pyramid_trail.growth = pyramid_trail_growth;
    pyramid_trail.rate = pyramid_trail_rate;
    pyramid_trail.drag = pyramid_trail_drag;
    pyramid_trail.lift = pyramid_trail_lift;
    if (player_in_range(&work->pos, NEAR)) {
        work->flags |= WORLD_TESTED;
    } else {
        work->flags &= ~WORLD_TESTED;
    }
    if (pyramid_risen()) {
        if (STATE(work) == PYRAMID_DOWN) {
            STATE(work) = PYRAMID_RISING;
            TASK_TIMER(work) = 0;
        }
    } else if (STATE(work) == PYRAMID_UP) {
        VARS(work)->height = 0.0F;
        STATE(work) = PYRAMID_DOWN;
    }
    if (STATE(work) == PYRAMID_RISING) {
        VARS(work)->height = (TASK_TIMER(work) * (RISE * (1.0F / RISE_STEPS))) +
                             (njSin(TASK_TIMER(work) * 12288) * 0.23F) +
                             (njSin(TASK_TIMER(work) * 16384) * 0.18F) +
                             (njSin(TASK_TIMER(work) * 20480) * 0.1F);
        njPushMatrix(&_unit_matrix_);
        njTranslateV(NULL, &work->pos);
        TURN_VIA(njRotateY, work->ang.y, turn);
        pyramid_dust.scale = 2.0F;
        pos.y = pyramid_dust_y;
        speed.y = 0.0F;
        pos.x = -pyramid_dust_x;
        speed.x = -pyramid_dust_speed;
        speed.z = 0.0F;
        if (rand() * RAND_SCALE < pyramid_dust_chance) {
            pos.z =
                pyramid_dust_spread_z * ((rand() * (2.0F * RAND_SCALE)) - 1.0F);
            njCalcPoint(NULL, &pos, &pyramid_dust.pos);
            njCalcVector(NULL, &speed, &pyramid_dust.speed);
            dust_create_from(&pyramid_dust);
        }
        pos.z = pyramid_dust_z;
        speed.x = 0.0F;
        speed.z = pyramid_dust_speed;
        if (rand() * RAND_SCALE < pyramid_dust_chance) {
            pos.x =
                pyramid_dust_spread_x * ((rand() * (2.0F * RAND_SCALE)) - 1.0F);
            njCalcPoint(NULL, &pos, &pyramid_dust.pos);
            njCalcVector(NULL, &speed, &pyramid_dust.speed);
            dust_create_from(&pyramid_dust);
        }
        njPopMatrix(1);
        if (rand() * RAND_SCALE < pyramid_rubble_chance) {
            speed.x = ((rand() * (2.0F * RAND_SCALE)) - 1.0F) * 1.5F;
            speed.z = ((rand() * (2.0F * RAND_SCALE)) - 1.0F) * 1.5F;
            speed.y = 0.3F;
            down = njSqrt((0.3F * 0.3F) + (rand() * (0.4F * RAND_SCALE)));
            across = down * rand() * RAND_SCALE;
            pos.x = (pyramid_face_down.x * down) +
                    (pyramid_face_across.x * across) + speed.x;
            pos.z = (pyramid_face_down.z * down) +
                    (pyramid_face_across.z * across) + speed.z;
            pos.y = (pyramid_face_down.y * down) +
                    (pyramid_face_across.y * across) + VARS(work)->height +
                    302.0F;
            ang.x = (Angle)(((rand() * (2.0F * RAND_SCALE)) - 1.0F) * 65536.0F);
            ang.y = (Angle)(((rand() * (2.0F * RAND_SCALE)) - 1.0F) * 65536.0F);
            ang.z = (Angle)(((rand() * (2.0F * RAND_SCALE)) - 1.0F) * 65536.0F);
            spin.x =
                (Angle)(((rand() * (2.0F * RAND_SCALE)) - 1.0F) * 16384.0F);
            spin.y =
                (Angle)(((rand() * (2.0F * RAND_SCALE)) - 1.0F) * 16384.0F);
            spin.z =
                (Angle)(((rand() * (2.0F * RAND_SCALE)) - 1.0F) * 16384.0F);
            rubble_add(pool, &pos, &speed, &ang, &spin,
                       0.4F + (rand() * (1.8F * RAND_SCALE)), RUBBLE_STEPS);
        }
        TASK_TIMER(work)++;
        if (TASK_TIMER(work) >= RISE_STEPS) {
            VARS(work)->height = RISE;
            STATE(work) = PYRAMID_UP;
        }
    }
    VARS(work)->object->pos[1] = work->pos.y + VARS(work)->height;
    rubble_update(work);
}

static void pyramid_draw(struct task *task)
{
    struct task_work *work = task->work;
    Angle turn;

    if (player_in_range(&work->pos, DRAW_RANGE) == 0) {
        return;
    }
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    TURN_VIA(njRotateY, work->ang.y, turn);
    njPushMatrix(NULL);
    njTranslate(NULL, 0.0F, VARS(work)->height, 0.0F);
    njSetTexture(&mr_pyramid_texlist);
    draw_model(&pyramid_model);
    njPopMatrix(1);
    rubble_draw(VARS(work)->rubble);
    njPopMatrix(1);
}

/* free every piece, with none falling */
static void rubble_init(struct rubble_pool *pool)
{
    struct rubble *piece;
    int i;

    pool->live = NULL;
    pool->free = pool->pieces;
    piece = pool->pieces;
    for (i = 0; i < RUBBLE_COUNT - 1; i++) {
        piece->next = piece + 1;
        piece++;
    }
    piece->next = NULL;
}

/* move each falling piece, with a puff of trail every pyramid_trail_steps steps
 * of its life. a piece is freed once its life runs out or it falls to
 * RUBBLE_FLOOR */
static void rubble_update(struct task_work *work)
{
    struct rubble_pool *pool = VARS(work)->rubble;
    struct rubble *piece;
    struct rubble *next;
    struct rubble *last;
    Angle turn;

    njPushMatrix(&_unit_matrix_);
    njTranslateV(NULL, &work->pos);
    TURN_VIA(njRotateY, work->ang.y, turn);
    last = NULL;
    for (piece = pool->live; piece != NULL; piece = next) {
        next = piece->next;
        if (piece->life != 0 && piece->pos.y > RUBBLE_FLOOR) {
            rubble_move(work, piece);
            if (piece->life % pyramid_trail_steps == 0) {
                pyramid_trail.scale = piece->scale * 0.8F;
                njCalcPoint(NULL, &piece->pos, &pyramid_trail.pos);
                njCalcVector(NULL, &piece->speed, &pyramid_trail.speed);
                dust_create_from(&pyramid_trail);
            }
            piece->life--;
            last = piece;
        } else {
            piece->next = pool->free;
            pool->free = piece;
            if (last == NULL) {
                pool->live = next;
            } else {
                last->next = next;
            }
        }
    }
    njPopMatrix(1);
}

/* move the piece a step. it bounces off the first face whose plane it crosses
 * inside the face's triangle, losing some speed and spin. every step it loses a
 * little more of both, and falls faster */
static void rubble_move(struct task_work *work, struct rubble *piece)
{
    NJS_POINT3 from;
    NJS_POINT3 to;
    NJS_POINT3 face_from;
    NJS_POINT3 face_to;
    NJS_POINT3 hit;
    NJS_VECTOR speed;
    Angle turn;
    int i;
    Float length;
    Float slope;
    Float time;

    from = piece->pos;
    to.x = from.x + piece->speed.x;
    to.y = from.y + piece->speed.y;
    to.z = from.z + piece->speed.z;
    for (i = 0; i < FACES; i++) {
        njPushMatrix(&_unit_matrix_);
        TURN(njRotateX, pyramid_slope);
        TURN_VIA(njRotateY, pyramid_faces[i], turn);
        njTranslate(NULL, 0.0F, -VARS(work)->height - FACE_HEIGHT, 0.0F);
        njCalcPoint(NULL, &from, &face_from);
        njCalcPoint(NULL, &to, &face_to);
        njCalcVector(NULL, &piece->speed, &speed);
        if (face_from.z >= 0.0F && face_to.z <= 0.0F) {
            length =
                njSqrt((FACE_HEIGHT * FACE_HEIGHT) + (FACE_HALF * FACE_HALF));
            slope = length * (1.0F / FACE_HALF);
            if (speed.z < 0.001F) {
                time = 0.5F;
            } else {
                time = -face_from.z / speed.z;
            }
            hit.x = face_from.x + (speed.x * time);
            /* @bug logic: hit.y advances by speed.x, not speed.y, so the
             * test below of the crossing against the triangle takes the wrong
             * height. the FMAC at 0x0c9046a0 takes @(12,R15), speed.x, loaded
             * at 0x0c90469c, where speed.y is @(16,R15) */
            hit.y = face_from.y + (speed.x * time);
            hit.z = 0.0F;
            if (!(hit.y > -slope * hit.x || hit.y > slope * hit.x ||
                  hit.y < -length)) {
                speed.z = -speed.z * 0.9F;
                speed.x *= 0.8F;
                speed.y *= 0.8F;
                piece->spin.x *= 0.9F;
                piece->spin.y *= 0.9F;
                piece->spin.z *= 0.9F;
                face_to.x = hit.x + (speed.x * (1.0F - time));
                face_to.y = hit.y + (speed.y * (1.0F - time));
                face_to.z = hit.z + (speed.z * (1.0F - time));
                njPushMatrix(NULL);
                njInvertMatrix(NULL);
                njCalcVector(NULL, &speed, &piece->speed);
                njCalcPoint(NULL, &face_to, &to);
                njPopMatrix(1);
                /* any i past FACES ends the loop once this face's matrix is
                 * popped: a piece bounces off one face only */
                i = 100;
            }
        }
        njPopMatrix(1);
    }
    piece->pos = to;
    piece->ang.x += piece->spin.x;
    piece->ang.y += piece->spin.y;
    piece->ang.z += piece->spin.z;
    piece->speed.x *= 0.99F;
    piece->speed.y *= 0.99F;
    piece->speed.z *= 0.99F;
    piece->speed.y -= 98.0F / (60.0F * 60.0F);
    piece->spin.x *= 0.99F;
    piece->spin.y *= 0.99F;
    piece->spin.z *= 0.99F;
}

static void rubble_draw(struct rubble_pool *pool)
{
    struct rubble *piece;
    Angle turn;
    Float scale;

    njSetTexture(&mr_pyramid_texlist);
    for (piece = pool->live; piece != NULL; piece = piece->next) {
        njPushMatrix(NULL);
        njTranslateV(NULL, &piece->pos);
        TURN_VIA(njRotateZ, piece->ang.z, turn);
        TURN_VIA(njRotateX, piece->ang.x, turn);
        TURN_VIA(njRotateY, piece->ang.y, turn);
        scale = piece->scale;
        njScale(NULL, scale, scale, scale);
        draw_model(&pyramid_rubble_model);
        njPopMatrix(1);
    }
}

/* start a free piece falling, if there is one */
static void rubble_add(struct rubble_pool *pool, NJS_POINT3 *pos,
                       NJS_VECTOR *speed, struct angles *ang,
                       struct angles *spin, Float scale, int life)
{
    struct rubble *piece = pool->free;

    if (piece != NULL) {
        pool->free = piece->next;
        piece->next = pool->live;
        pool->live = piece;
        piece->pos = *pos;
        piece->speed = *speed;
        piece->ang = *ang;
        piece->spin = *spin;
        piece->scale = scale;
        piece->life = life;
    }
}

static void pyramid_destroy(struct task *task)
{
    struct task_work *work = task->work;

    world_remove(task, VARS(work)->object);
    world_object_free(VARS(work)->object);
    heap_free(VARS(work)->rubble);
}

/* TRUE once the flag for player 0's character is set. FALSE for a character
 * with none: only Sonic has one */
static Bool pyramid_risen(void)
{
    int flag = pyramid_flags[player_get_character(0)];

    if (flag >= 0) {
        return event_flags[flag];
    }
    return FALSE;
}
