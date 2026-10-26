/* O SHAT1 and O SHAT2, shutters that drop behind a player who passes
 * through. they rise again for one in front */
#include <ninja.h>
#include <sg_xpt.h>
#include <stdlib.h>

#include "collision/collision.h"
#include "debug/editor.h"
#include "draw/tree.h"
#include "player/player.h"
#include "sound/sound.h"
#include "stg09/casino.h"
#include "stg09/helpers.h"
#include "stg09/shutter.h"
#include "system/task.h"
#include "system/util.h"

extern NJS_OBJECT shutter_object_1;
extern NJS_OBJECT shutter_object_2;

/* the shutter runs and draws only while player 0 or 1 is within this
 * distance of it */
#define REACH 600.0F

/* where shutter_side finds a player */
#define SIDE_FRONT 0x2
#define SIDE_BEHIND 0x4
#define SIDE_OUTSIDE 0x8 /* above, below or beside the shutter */
#define SIDE_FAR 0x10 /* farther in front or behind than its reach */

/* the bit of a player's crossing state that shutter_crossing sets while the
 * player is near the front. it clears the bit while the player is in front
 * but outside or far */
#define SEEN_FRONT SIDE_FRONT

enum shutter_mode {
    SHUTTER_UP = 1,
    SHUTTER_DROPPING,
    SHUTTER_RISING,
    SHUTTER_DOWN
};

struct shutter_kind {
    NJS_OBJECT *object;
    /* shapes[0] and shapes[1], the shutter and its rounded foot, move with
     * its height. shapes[2], a box below it, stays */
    struct collision_shape shapes[3];
    /* a player still counts as near the shutter this far past shapes[0]'s
     * half depth, in front or behind */
    Float reach;
    Float raised; /* the height it rises to */
    Float unk_a8; /* never read */
};

struct shutter_vars {
    /* the shutters before and after it in shutter_list */
    struct task *prev;
    struct task *next;
    Float height; /* how far the shutter stands above its place */
};

#define VARS(work) ((struct shutter_vars *)&(work)->vars)
#define KIND(work) ((work)->submode)
/* WAIT counts steps in a row. while the shutter is up, it counts those on
 * which shutter_crossing returns 1 for either player. while it is down, it
 * counts those on which it returns -1. the shutter drops once WAIT passes
 * DROP_WAIT, and rises once it passes RISE_WAIT */
#define WAIT(work) ((work)->tick)
#define DROP_WAIT 10
#define RISE_WAIT 2
#define SPEED(work) ((work)->scl.y) /* its speed upward while it drops */
/* the shutters with the same ID drop together */
#define ID(work) ((Sint16)(work)->scl.z)
/* the address of player 0's or player 1's crossing state, which
 * shutter_crossing keeps. the state is byte 2 or 3 of ang.x, its high half on
 * the little-endian SH-4 */
#define CROSSING(work, player) ((Sint8 *)&(work)->ang.x + 2 + (player))
/* in work->flags, from scl.x's bit 0: the shutter starts no drop and never
 * rises */
#define IGNORES_PLAYERS 0x8000

/* the dust puffs along the shutter's foot as it lands hard. the puffs are
 * PUFF_SPACING apart from its middle, within shapes[0]'s half width each way.
 * each lies up to PUFF_JITTER further along x at random */
#define PUFF_SPACING 15.0F
#define PUFF_JITTER 10.0F

static Float shutter_gravity = -0.2F;
static Float dat_0cb155e4 = -0.22F; /* never read */
/* the share of SPEED kept each step the shutter drops */
static Float shutter_drag = 0.9F;
/* SPEED after a bounce, as a share of SPEED as the shutter lands */
static Float shutter_bounce = -0.4F;
/* the dust's scale, its rise and its speed along z. each is per unit of the
 * speed the shutter lands at */
static Float shutter_dust_scale = 2.0F;
static Float shutter_dust_rise = 0.01F;
static Float shutter_dust_out = -0.5F;
/* O SHAT1's kind, then O SHAT2's */
static struct shutter_kind shutter_kinds[2] = {
    {
        &shutter_object_1,
        {
            {
                SHAPE_RECTANGLE,
                0,
                PUSH_ALL | PUSH_FIRM(PUSH_ALL),
                {0.0F, 30.0F, -2.865F},
                24.0F,
                30.0F,
                2.865F,
                {0},
                0,
                NULL,
            },
            {
                SHAPE_CYLINDER2,
                0,
                PUSH_ALL | PUSH_FIRM(PUSH_ALL),
                {0.0F, 0.0F, -2.865F},
                2.865F,
                24.0F,
                0.0F,
                {0, 0, NJM_DEG_ANG(90)},
                0,
                NULL,
            },
            {
                SHAPE_RECTANGLE,
                0,
                PUSH_ALL | PUSH_FIRM(PUSH_ALL),
                {0.0F, -9.0F, -2.865F},
                30.0F,
                8.0F,
                2.865F,
                {0},
                0,
                NULL,
            },
        },
        30.0F,
        50.0F,
        0.1F,
    },
    {
        &shutter_object_2,
        {
            {
                SHAPE_RECTANGLE,
                0,
                PUSH_ALL | PUSH_FIRM(PUSH_ALL),
                {0.0F, 72.5F, 0.0F},
                64.0F,
                72.5F,
                5.0F,
                {0},
                0,
                NULL,
            },
            {
                SHAPE_CYLINDER2,
                0,
                PUSH_ALL | PUSH_FIRM(PUSH_ALL),
                {0.0F, 0.0F, 0.0F},
                5.0F,
                64.0F,
                0.0F,
                {0, 0, NJM_DEG_ANG(90)},
                0,
                NULL,
            },
            {
                SHAPE_RECTANGLE,
                0,
                PUSH_ALL | PUSH_FIRM(PUSH_ALL),
                {0.0F, -9.0F, 0.0F},
                65.0F,
                8.0F,
                5.0F,
                {0},
                0,
                NULL,
            },
        },
        50.0F,
        105.0F,
        2.0F,
    },
};
/* every shutter, the latest first */
static struct task *shutter_list = NULL;

/* where player is from the shutter: SIDE_FRONT or SIDE_BEHIND, with
 * SIDE_OUTSIDE and SIDE_FAR where they hold. 0 while player_work[player] is
 * null */
static Sint8 shutter_side(struct task *task, Sint32 player)
{
    Sint8 side = 0;
    struct task_work *other = player_work[player];
    struct task_work *work = task->work;
    struct shutter_kind *kind = &shutter_kinds[KIND(work)];
    NJS_POINT3 at;
    Angle ang;

    if (!other) {
        return 0;
    }
    njPushMatrix(&_unit_matrix_);
    TURN_VIA(njRotateY, -work->ang.y, ang);
    njTranslate(NULL, -work->pos.x, -work->pos.y, -work->pos.z);
    njCalcPoint(NULL, &other->pos, &at);
    njPopMatrix(1);
    at.z -= kind->shapes[0].center.z;
    if (_builtin_fabsf(work->pos.y + kind->shapes[0].center.y - other->pos.y) >
            kind->shapes[0].size_y + 20.0F ||
        _builtin_fabsf(at.x) > kind->shapes[0].radius) {
        side = SIDE_OUTSIDE;
    }
    if (at.z >= 0.0F) {
        side |= SIDE_FRONT;
    } else {
        side |= SIDE_BEHIND;
    }
    if (kind->reach + kind->shapes[0].size_z < _builtin_fabsf(at.z)) {
        side |= SIDE_FAR;
    }
    return side;
}

/* -1 while player is near the shutter's front: in front, within its reach
 * and not outside it. 1 while player is behind it and *state holds
 * SEEN_FRONT. 0 otherwise. *state is player's crossing state */
static Sint8 shutter_crossing(struct task *task, Sint8 player, Sint8 *state)
{
    Sint8 crossing = 0;
    Sint8 side = shutter_side(task, player);

    if (side & SIDE_FRONT) {
        if ((side & SIDE_OUTSIDE) || (side & SIDE_FAR)) {
            *state &= ~SEEN_FRONT;
        } else {
            *state |= SEEN_FRONT;
            crossing = -1;
        }
    } else if (side & SIDE_BEHIND) {
        if (*state & SEEN_FRONT) {
            crossing = 1;
        }
    }
    return crossing;
}

static void shutter_init(struct task *task, int kind);

void shutter_step(struct task *task)
{
    shutter_init(task, 0);
}

void shutter_tall_step(struct task *task)
{
    shutter_init(task, 1);
}

static void shutter_destroy(struct task *task)
{
    struct task_work *work = task->work;

    if (VARS(work)->next) {
        VARS(VARS(work)->next->work)->prev = VARS(work)->prev;
    }
    if (VARS(work)->prev) {
        VARS(VARS(work)->prev->work)->next = VARS(work)->next;
    } else {
        shutter_list = VARS(work)->next;
    }
}

static void shutter_draw(struct task *task)
{
    struct task_work *work = task->work;
    struct shutter_kind *kind = &shutter_kinds[KIND(work)];
    Angle ang;

    if (player_in_range(&work->pos, REACH)) {
        casino_set_texture(CASINO_TEXTURES_ACTS_12);
        njPushMatrix(NULL);
        njTranslate(NULL, work->pos.x, work->pos.y + VARS(work)->height,
                    work->pos.z);
        TURN_VIA(njRotateY, work->ang.y, ang);
        draw_tree(kind->object);
        njPopMatrix(1);
    }
}

static void shutter_run(struct task *task)
{
    struct task_work *work = task->work;
    struct shutter_kind *kind = &shutter_kinds[KIND(work)];
    Sint8 first;
    Sint8 second;

    if (!player_in_range(&work->pos, REACH)) {
        return;
    }
    shutter_draw(task);
    if (placement_editing(task)) {
        if ((Sint8)work->scl.x & 1) {
            njPrintC(EDITOR_LABEL_RIGHT(EDITOR_ROW_SCL_X), "-> open ");
        } else {
            njPrintC(EDITOR_LABEL_RIGHT(EDITOR_ROW_SCL_X), "-> Norm ");
        }
        njPrintC(EDITOR_LABEL_RIGHT(EDITOR_ROW_SCL_Z), "-> ID ");
        return;
    }
    switch (work->mode) {
    case SHUTTER_UP:
        first = shutter_crossing(task, 0, CROSSING(work, 0));
        second = shutter_crossing(task, 1, CROSSING(work, 1));
        if (!(work->flags & IGNORES_PLAYERS) && (first == 1 || second == 1)) {
            if (WAIT(work)++ > DROP_WAIT) {
                struct task *other = shutter_list;

                WAIT(work) = 0;
                work->mode = SHUTTER_DROPPING;
                work->timer = 0;
                for (; other; other = VARS(other->work)->next) {
                    if (other != task && ID(work) == ID(other->work) &&
                        other->work->mode == SHUTTER_UP) {
                        other->work->mode = SHUTTER_DROPPING;
                        other->work->timer = 0;
                        WAIT(other->work) = 0;
                    }
                }
            }
        } else {
            WAIT(work) = 0;
        }
        break;
    case SHUTTER_DROPPING:
        shutter_crossing(task, 0, CROSSING(work, 0));
        shutter_crossing(task, 1, CROSSING(work, 1));
        VARS(work)->height += SPEED(work);
        SPEED(work) += shutter_gravity;
        SPEED(work) *= shutter_drag;
        if (VARS(work)->height < 0.0F) {
            VARS(work)->height = 0.0F;
            if (SPEED(work) < shutter_gravity * 2.0F) {
                NJS_VECTOR speed;
                NJS_POINT3 right;
                NJS_POINT3 left;
                Float landing = _builtin_fabsf(SPEED(work));
                Float scale;
                Angle ang;
                int count;
                int i;

                speed.x = 0.0F;
                speed.z = shutter_dust_out * landing;
                speed.y = shutter_dust_rise * landing;
                scale = shutter_dust_scale * landing;
                count = (int)(kind->shapes[0].radius * (1.0F / PUFF_SPACING));
                njPushMatrix(&_unit_matrix_);
                njTranslateV(NULL, &work->pos);
                TURN_VIA(njRotateY, work->ang.y, ang);
                njCalcVector(NULL, &speed, &speed);
                sound_play_at(SE_SLAM, 0, 0, (int)landing, work->pos.x,
                              work->pos.y, work->pos.z);
                for (i = 0; i < count; i++) {
                    right.x = ((Float)i * PUFF_SPACING) +
                              (rand() * (PUFF_JITTER * RAND_SCALE));
                    right.y = 0.0F;
                    right.z = 0.0F;
                    left.x = ((Float)-i * PUFF_SPACING) +
                             (rand() * (PUFF_JITTER * RAND_SCALE));
                    left.y = 0.0F;
                    left.z = 0.0F;
                    njCalcPoint(NULL, &right, &right);
                    njCalcPoint(NULL, &left, &left);
                    casino_dust_toward_camera(&right, &speed, scale, 8.0F);
                    if (i) {
                        casino_dust_toward_camera(&left, &speed, scale, 8.0F);
                    }
                }
                njPopMatrix(1);
                SPEED(work) *= shutter_bounce;
            } else {
                work->mode = SHUTTER_DOWN;
                work->timer = 0;
            }
        }
        break;
    case SHUTTER_DOWN:
        first = shutter_crossing(task, 0, CROSSING(work, 0));
        second = shutter_crossing(task, 1, CROSSING(work, 1));
        if (!(work->flags & IGNORES_PLAYERS) && (first == -1 || second == -1)) {
            if (WAIT(work)++ > RISE_WAIT) {
                WAIT(work) = 0;
                work->mode = SHUTTER_RISING;
                work->timer = 0;
            }
        } else {
            WAIT(work) = 0;
        }
        break;
    case SHUTTER_RISING:
        shutter_crossing(task, 0, CROSSING(work, 0));
        shutter_crossing(task, 1, CROSSING(work, 1));
        VARS(work)->height += 1.0F;
        SPEED(work) = 0.0F;
        if (VARS(work)->height > kind->raised) {
            VARS(work)->height = kind->raised;
            work->mode = SHUTTER_UP;
            work->timer = 0;
        }
    }
    work->collision->shapes[0].center.y =
        kind->shapes[0].center.y + VARS(work)->height;
    work->collision->shapes[1].center.y =
        kind->shapes[1].center.y + VARS(work)->height;
    collision_register(work);
    work->collision->radius = kind->raised + 10.0F;
}

static void shutter_init(struct task *task, int kind)
{
    struct task_work *work = task->work;

    task->draw = shutter_draw;
    task->step = shutter_run;
    task->destructor = shutter_destroy;
    work->mode = SHUTTER_UP;
    KIND(work) = kind;
    WAIT(work) = 0;
    VARS(work)->height = shutter_kinds[kind].raised;
    SPEED(work) = 0.0F;
    if (!placement_editing(task)) {
        work->ang.x = 0;
        work->ang.z = 0;
        collision_init(task, shutter_kinds[kind].shapes, COLLISION_OBJECT, 1,
                       3);
    }
    VARS(work)->next = shutter_list;
    VARS(work)->prev = NULL;
    if (shutter_list) {
        VARS(shutter_list->work)->prev = task;
    }
    if ((Sint8)work->scl.x & 1) {
        work->flags |= IGNORES_PLAYERS;
    } else {
        work->flags &= ~IGNORES_PLAYERS;
    }
    shutter_list = task;
    *CROSSING(work, 0) = 0;
    *CROSSING(work, 1) = 0;
}
