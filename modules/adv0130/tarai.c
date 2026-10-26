/* the washbasin puzzle the object list names O TARAI: six tiles to stand on
 * in the order of their faces */
#include <mathf.h>
#include <ninja.h>
#include <sg_xpt.h>
#include <stdlib.h>

#include "adv0130/egg_carrier_inside.h"
#include "adv0130/tarai.h"
#include "collision/collision.h"
#include "collision/world.h"
#include "debug/editor.h"
#include "draw/draw.h"
#include "draw/tree.h"
#include "event/play.h"
#include "object/placement.h"
#include "player/player.h"
#include "progress/progress.h"
#include "sound/sound.h"
#include "system/task.h"
#include "system/util.h"

extern NJS_OBJECT tarai_board_object;
extern NJS_OBJECT tarai_board_solid_object;
extern NJS_OBJECT tarai_tile_object; /* a tile, whose child is its face */
extern NJS_OBJECT tarai_basin_object;
extern NJS_OBJECT tarai_glow_object;
extern NJS_OBJECT tarai_ripple_base_object;
extern NJS_OBJECT tarai_ripple_object; /* the ripple's middle, which sways */

/* tiles on the board */
#define TILES 6

/* a tile starts down once it has been stood on for more than PRESS_STEPS
 * steps, together or apart, since it last started */
#define PRESS_STEPS 5
/* a tile's own texture, and the lit one it takes once it is down */
#define TILE_TEXTURE 2
#define TILE_LIT_TEXTURE 13

/* a basin falls once it has waited more than BASIN_WAIT_STEPS steps */
#define BASIN_WAIT_STEPS 20
/* for a tile pressed out of order, the board rolls from 0 up to 1. below
 * FLIP_ROLL it drops a flipping basin, below SWING_ROLL a swinging one, and
 * else a squashing one */
#define FLIP_ROLL 0.7F
#define SWING_ROLL 0.95F
/* a swinging basin's middle stands SWING_ARM from the pivot it tips about */
#define SWING_ARM 5.0F
/* the alpha a basin loses each step as it fades */
#define BASIN_FADE 0.08F
/* the steps from the sixth tile in order to SE_NOTICE */
#define SOLVED_STEPS 40
/* a squashing basin waits more than SQUASH_PAUSE_STEPS steps once it lands.
 * it then tips, faster each step up to SQUASH_TIP_SPEED a step, until it has
 * tipped SQUASH_TIP_ANGLE */
#define SQUASH_PAUSE_STEPS 8
#define SQUASH_TIP_SPEED 300
#define SQUASH_TIP_ANGLE NJM_DEG_ANG(20)

#define SE_TILE_RIGHT 568 /* a tile going down in order */
#define SE_TILE_WRONG 554 /* a tile going down out of order */
#define SE_BASIN_FALL 569 /* a basin starting to fall */
#define SE_BASIN_SQUASH 701 /* a squashing basin landing */

struct board_vars {
    NJS_OBJECT *solid; /* in the collision world */
    Sint32 count; /* the tiles pressed in order so far */
    /* -1 once a tile is pressed out of order, until the board takes it up */
    Sint32 wrong;
};

struct basin_vars {
    Float floor; /* the board's height, where a flip or a swing is gone */
    Float head; /* the height of the head it lands on */
    Float fade; /* its material's alpha, from 1 down */
};

/* the glow's vars: its alpha, which rises from -1 to 0 and falls back, and
 * the alpha's change each step. a ripple keeps its sway in alpha */
struct glow_vars {
    Float alpha;
    Float speed;
};

#define BOARD(work) ((struct board_vars *)&(work)->vars)
#define BASIN(work) ((struct basin_vars *)&(work)->vars)
#define GLOW(work) ((struct glow_vars *)&(work)->vars)

/* a tile's number on the board, the height it moves a step, and the heights
 * it stops at, up and down */
#define TILE(work) ((work)->tick)
#define TILE_SPEED(work) ((work)->scl.x)
#define TILE_TOP(work) ((work)->scl.y)
#define TILE_BOTTOM(work) ((work)->scl.z)

/* in a tile's work->flags, set by the checks while something stands on it */
#define TILE_STOOD_ON COLLISION_UNDER

/* a basin's TASK_MOTION record, which each kind uses its own way */
struct basin_motion {
    void *unk_00;
    NJS_VECTOR speed;
    /* the swing's and the squash's change of speed each step. the flip keeps
     * its offset from player 0 here instead, and then bounces along it */
    NJS_VECTOR fall;
    struct angles ang;
    struct angles spin;
    Float tilt;
    Float tilt_speed;
    Float gravity;
};

enum tarai_mode {
    TARAI_WAIT = 1,
    TARAI_DROP = 2, /* a tile went down out of order: drop a basin */
    TARAI_FALLING = 3,
    TARAI_LANDED = 4, /* set by the basin as it goes: let player 0 go */
    TARAI_SHUFFLE = 5,
    TARAI_SETTLE = 6,
    TARAI_SOLVED = 7,
    TARAI_DONE = 21
};

enum tile_mode {
    TILE_UP = 0,
    TILE_PRESSING = 9,
    TILE_RISING = 10,
    TILE_RAISED = 11, /* risen from a shuffle: count stays 0 until it's left */
    TILE_DOWN_AWAY = 12, /* down and left, and solid no more */
    TILE_DOWN = 13
};

/* a tile's submode: TILE_PRESSED from the start of a press until the tile
 * rises again, and TILE_UNPRESSED otherwise. nothing in this file reads it */
enum tile_submode { TILE_PRESSED = 14, TILE_UNPRESSED = 15 };

/* the modes of the three kinds of basin, an enum for each. a basin starts in
 * its WAIT mode. in its GONE mode it sets the board's mode to TARAI_LANDED
 * and kills itself */
enum flip_mode {
    FLIP_FALL = 16,
    FLIP_AWAY = 17,
    FLIP_BOUNCE = 18,
    FLIP_WAIT = 19,
    FLIP_GONE = 21
};
enum swing_mode {
    SWING_FALL = 16,
    SWING_WAIT = 17,
    SWING_SWING = 18,
    SWING_OFF = 19,
    SWING_GONE = 21
};
enum squash_mode {
    SQUASH_FALL = 16,
    SQUASH_PAUSE,
    SQUASH_TIP,
    SQUASH_FADE,
    SQUASH_WAIT,
    SQUASH_GONE
};

/* a ripple's modes, in work->mode */
enum ripple_mode { RIPPLE_GROW, RIPPLE_SWAY, RIPPLE_STILL };

/* how far above player 0's head a basin starts */
static Float tarai_drop_height = 195.0F;
/* each tile's place on the board: the outer ring, then the inner */
static NJS_POINT3 tile_points[TILES] = {
#include "adv0130/tarai/tile_points.inc"
};
/* each face's texture in ec_tarai_texlist */
static Uint32 face_textures[] = {14, 15, 16, 3, 17};
/* the faces in the order they must be stood on */
static Uint8 face_order[TILES] = {0, 1, 1, 2, 3, 4};
/* a tile's cylinder, and two spheres after it. collision_init takes only the
 * first */
static struct collision_shape tile_shapes[] = {
    {SHAPE_CYLINDER,
     0,
     PUSH_ALL | PUSH_FIRM(PUSH_ALL),
     {0.0F, 0.0F, 0.0F},
     9.0F,
     3.0F,
     0.0F,
     {0},
     0,
     NULL},
    {SHAPE_SPHERE,
     0,
     PUSH_ALL | PUSH_FIRM(PUSH_ALL),
     {0.0F, 2.0F, 0.0F},
     4.0F,
     0.0F,
     0.0F,
     {0},
     0,
     NULL},
    {SHAPE_SPHERE,
     0,
     PUSH_ALL | PUSH_FIRM(PUSH_ALL),
     {0.0F, 1.3F, 0.0F},
     4.0F,
     0.0F,
     0.0F,
     {0},
     0,
     NULL},
};

/* the face each tile shows, as shuffled */
static Uint8 tile_faces[TILES];

static void tarai_step(struct task *task);
static void tarai_draw(struct task *task);
static void tarai_destroy(struct task *task);
static void tile_init(struct task *task);
static void tile_step(struct task *task);
static void tile_draw(struct task *task);
static void basin_flip_init(struct task *task);
static void basin_flip_step(struct task *task);
static void basin_flip_draw(struct task *task);
static void basin_swing_init(struct task *task);
static void basin_swing_step(struct task *task);
static void basin_swing_draw(struct task *task);
static void basin_squash_init(struct task *task);
static void basin_squash_step(struct task *task);
static void basin_squash_draw(struct task *task);
static void tile_glow(Float x, Float y, Float z);
static void glow_step(struct task *task);
static void glow_draw(struct task *task);

/* how far above player 0's feet a basin lands: the character's height and
 * 0.2 more, or 2 more for E-102 and Big. 0 with no player 0, and for Eggman
 * and Tikal */
static Float tarai_head_height(void)
{
    struct player *player = player_get(0);

    /* @hack redundant: the switch in an else after the return, which loads
     * player_get_character's address into R2 for its call (0x0c923690);
     * without it, R3 takes the address and 2 of 43 words differ */
    if (player == NULL) {
        return 0.0F;
    } else {
        switch (player_get_character(0)) {
        case CHARACTER_SONIC:
        case CHARACTER_TAILS:
        case CHARACTER_KNUCKLES:
        case CHARACTER_AMY:
            return player->consts.height + 0.2F;
        case CHARACTER_E102:
            return player->consts.height + 2.0F;
        case CHARACTER_BIG:
            return player->consts.height + 2.0F;
        }
    }
    return 0.0F;
}

/* deal face_order's faces out to the tiles, sorted by a random key each */
static void tarai_shuffle(struct task *task)
{
    Float keys[TILES];
    Uint8 faces[TILES];
    int i;
    int j;
    Float key;
    Uint8 face;

    for (i = 0; i < TILES; i++) {
        keys[i] = rand() * RAND_SCALE;
        faces[i] = face_order[i];
    }
    for (i = 0; i < TILES; i++) {
        for (j = i + 1; j < TILES; j++) {
            if (keys[i] < keys[j]) {
                key = keys[i];
                keys[i] = keys[j];
                keys[j] = key;
                face = faces[i];
                faces[i] = faces[j];
                faces[j] = face;
            }
        }
    }
    for (i = 0; i < TILES; i++) {
        tile_faces[i] = faces[i];
    }
}

/* set the board up. outside the set editor, put it in the world, shuffle
 * the faces and make the tiles */
void egg_carrier_tarai_step(struct task *task)
{
    struct task_work *work = task->work;
    struct task_force *force = task->force;
    NJS_OBJECT *solid;
    struct task *child;
    struct task_work *tile;
    NJS_COLOR *uv;
    Angle ang;
    int i;

    work->mode = TARAI_WAIT;
    work->timer = 0;
    work->tick = 0;
    BOARD(work)->count = 0;
    work->scl.x = 0.0F;
    work->scl.y = 0.0F;
    work->scl.z = 0.0F;
    force->turn.x = 0;
    force->turn.y = 0;
    force->turn.z = 0;
    force->speed.x = 0.0F;
    force->speed.y = 0.0F;
    force->speed.z = 0.0F;
    tile_faces[0] = 4;
    tile_faces[1] = 3;
    tile_faces[2] = 2;
    tile_faces[3] = 1;
    tile_faces[4] = 1;
    tile_faces[5] = 0;
    if (!placement_editing(task)) {
        solid = world_object_alloc();
        BOARD(work)->solid = solid;
        solid->evalflags = NJD_EVAL_BREAK;
        solid->scl[0] = 1.0F;
        solid->scl[1] = 1.0F;
        solid->scl[2] = 1.0F;
        solid->ang[0] = 0;
        solid->ang[1] = work->ang.y;
        solid->ang[2] = 0;
        solid->pos[0] = work->pos.x;
        solid->pos[1] = work->pos.y;
        solid->pos[2] = work->pos.z;
        solid->model = tarai_board_solid_object.model;
        solid->child = NULL;
        solid->sibling = NULL;
        world_add(WORLD_LARGE | WORLD_SOLID, task, solid);
        tarai_shuffle(task);
        njPushMatrix(NULL);
        njUnitMatrix(NULL);
        njTranslateV(NULL, &work->pos);
        TURN_VIA(njRotateY, work->ang.y, ang);
        for (i = 0; i < TILES; i++) {
            child = task_create_child(TASK_WORK, tile_init, task);
            tile = child->work;
            njCalcPoint(NULL, &tile_points[i], &tile->pos);
            tile->ang.x = 0;
            /* @hack redundant: the same store on both sides of a test of i.
             * SHC still holds tarai_shuffle's literals here, and writes them
             * after the BRA over the else (0x0c92391a), from 0x0c923920.
             * stored once, they go between tarai_shuffle and this function,
             * which then starts 42 bytes later, and 388 of 390 words differ */
            if (i < 3) {
                tile->ang.y = work->ang.y;
            } else {
                tile->ang.y = work->ang.y;
            }
            tile->ang.z = 0;
            TILE_SPEED(tile) = 0.2F;
            TILE_TOP(tile) = tile->pos.y;
            TILE_BOTTOM(tile) = tile->pos.y + -3.0F;
            TILE(tile) = i;
        }
        njPopMatrix(1);
    }
    uv = tarai_tile_object.child->model->meshsets->vertuv;
    uv[0].tex.u = 255;
    uv[0].tex.v = 254;
    uv[1].tex.u = 255;
    uv[1].tex.v = 0;
    uv[2].tex.u = 0;
    uv[2].tex.v = 254;
    uv[3].tex.u = 0;
    uv[3].tex.v = 0;
    task->step = tarai_step;
    task->draw = tarai_draw;
    task->destructor = tarai_destroy;
}

/* the board's step, which steps the tiles and the basin too. when a tile goes
 * down out of order, the board holds player 0 still and drops a basin on it.
 * once the basin is gone, the board lets player 0 go, shuffles the faces and
 * raises the tiles. six tiles in order set FLAG_TARAI_SOLVED */
static void tarai_step(struct task *task)
{
    struct task_work *work = task->work;

    if (placement_try_leave(task)) {
        return;
    }
    if (event_in_control()) {
        return;
    }
    if (!placement_editing(task)) {
        switch (work->mode) {
        case TARAI_WAIT:
            if (BOARD(work)->count >= TILES) {
                work->mode = TARAI_SOLVED;
                work->timer = 0;
                event_flag_set(FLAG_TARAI_SOLVED);
            } else if (BOARD(work)->wrong < 0) {
                BOARD(work)->count = 0;
                BOARD(work)->wrong = 0;
                player_set_mode(0, PLAYER_MODE_STILL);
                work->mode = TARAI_DROP;
            }
            break;
        case TARAI_DROP: {
            Float roll = rand() * RAND_SCALE;

            if (roll < FLIP_ROLL) {
                task_create_child(TASK_WORK | TASK_MOTION, basin_flip_init,
                                  task);
            } else if (roll < SWING_ROLL) {
                task_create_child(TASK_WORK | TASK_MOTION, basin_swing_init,
                                  task);
            } else {
                task_create_child(TASK_WORK | TASK_MOTION, basin_squash_init,
                                  task);
            }
            work->mode = TARAI_FALLING;
        } break;
        case TARAI_LANDED:
            player_set_mode(0, PLAYER_MODE_RESET);
            work->timer++;
            work->mode = TARAI_SHUFFLE;
            break;
        case TARAI_SHUFFLE: {
            struct task *child;

            tarai_shuffle(task);
            for (child = task->child; child != NULL; child = child->next) {
                child->work->mode = TILE_RISING;
            }
            BOARD(work)->count = 1;
            work->mode = TARAI_SETTLE;
        } break;
        case TARAI_SETTLE:
            /* the shuffle left count at 1, so the board waits here a step
             * for the tiles to start rising */
            if (BOARD(work)->count == 1) {
                work->mode = TARAI_WAIT;
                BOARD(work)->count = 0;
            } else {
                BOARD(work)->count = 1;
            }
            break;
        case TARAI_SOLVED:
            work->timer++;
            if (TASK_TIMER(work) == SOLVED_STEPS) {
                sound_play(SE_NOTICE, 0, 0, 0);
                work->timer = 0;
                work->mode = TARAI_DONE;
            }
            break;
        case TARAI_FALLING:
        case TARAI_DONE:
            break;
        }
        task_step_children(task);
        if (player_in_range(&work->pos, BOARD(work)->solid->model->r + 20.0F)) {
            work->flags |= WORLD_TESTED;
        } else {
            work->flags &= ~WORLD_TESTED;
        }
    }
    tarai_draw(task);
}

static void tarai_draw(struct task *task)
{
    struct task_work *work = task->work;
    Angle ang;
    int i;

    njSetTexture(&ec_tarai_texlist);
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    TURN_VIA(njRotateY, work->ang.y, ang);
    draw_tree(&tarai_board_object);
    if (placement_editing(task)) {
        for (i = 0; i < TILES; i++) {
            njPushMatrix(NULL);
            njTranslateV(NULL, &tile_points[i]);
            draw_tree(&tarai_tile_object);
            njPopMatrix(1);
        }
    }
    njPopMatrix(1);
}

static void tarai_destroy(struct task *task)
{
    struct task_work *work = task->work;

    if (BOARD(work)->solid != NULL) {
        world_remove(task, BOARD(work)->solid);
        world_object_free(BOARD(work)->solid);
    }
    task_destroy_children(task);
}

static void tile_init(struct task *task)
{
    struct task_work *work = task->work;

    work->mode = TILE_UP;
    work->submode = TILE_UNPRESSED;
    work->timer = 0;
    collision_init(task, tile_shapes, COLLISION_OBJECT, 1, 1);
    task->step = tile_step;
    task->draw = tile_draw;
}

/* a pressed tile goes down only while it's stood on, and rises if it is left
 * before the bottom. at the bottom, a face that comes next in face_order
 * counts and glows, and any other sets wrong. a tile down stays down until a
 * shuffle, and is solid only until it is left */
static void tile_step(struct task *task)
{
    struct task_work *work = task->work;

    switch (work->mode) {
    case TILE_UP:
        if (work->flags & TILE_STOOD_ON) {
            work->flags &= ~TILE_STOOD_ON;
            work->timer++;
            if (TASK_TIMER(work) > PRESS_STEPS) {
                work->mode = TILE_PRESSING;
                work->submode = TILE_PRESSED;
                work->timer = 0;
            }
        } else {
            work->submode = TILE_UNPRESSED;
        }
        collision_register(work);
        break;
    case TILE_PRESSING:
        if (work->flags & TILE_STOOD_ON) {
            work->flags &= ~TILE_STOOD_ON;
            work->pos.y -= TILE_SPEED(work);
            if (work->pos.y <= TILE_BOTTOM(work)) {
                work->pos.y = TILE_BOTTOM(work);
                work->mode = TILE_DOWN;
                if (face_order[BOARD(task->parent->work)->count] ==
                    tile_faces[TILE(work)]) {
                    BOARD(task->parent->work)->count++;
                    sound_play(SE_TILE_RIGHT, 0, 0, 0);
                    tile_glow(work->pos.x, work->pos.y, work->pos.z);
                } else {
                    sound_play(SE_TILE_WRONG, 0, 0, 0);
                    BOARD(task->parent->work)->wrong = -1;
                }
            }
        } else {
            work->submode = TILE_UNPRESSED;
            work->mode = TILE_RISING;
        }
        collision_register(work);
        break;
    case TILE_DOWN:
        if (work->flags & TILE_STOOD_ON) {
            work->flags &= ~TILE_STOOD_ON;
            collision_register(work);
        } else {
            work->mode = TILE_DOWN_AWAY;
        }
        break;
    case TILE_RISING:
        work->pos.y += TILE_SPEED(work);
        if (work->pos.y >= TILE_TOP(work)) {
            work->pos.y = TILE_TOP(work);
            work->mode = TILE_RAISED;
        }
        work->submode = TILE_UNPRESSED;
        collision_register(work);
        break;
    case TILE_RAISED:
        if (work->flags & TILE_STOOD_ON) {
            work->flags &= ~TILE_STOOD_ON;
            BOARD(task->parent->work)->count = 0;
            collision_register(work);
        } else {
            work->mode = TILE_UP;
        }
        break;
    case TILE_DOWN_AWAY:
        break;
    }
    tile_draw(task);
}

/* draw the tile with its face's texture, and its own lit once it is down */
static void tile_draw(struct task *task)
{
    struct task_work *work = task->work;
    Angle ang;

    njSetTexture(&ec_tarai_texlist);
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    TURN_VIA(njRotateY, work->ang.y, ang);
    tarai_tile_object.child->model->mats->attr_texId =
        face_textures[tile_faces[TILE(work)]];
    if (work->mode == TILE_DOWN || work->mode == TILE_DOWN_AWAY) {
        tarai_tile_object.model->mats->attr_texId = TILE_LIT_TEXTURE;
    } else {
        tarai_tile_object.model->mats->attr_texId = TILE_TEXTURE;
    }
    draw_tree(&tarai_tile_object);
    njPopMatrix(1);
}

/* a basin that bounces off player 0's head, and flies off spinning and
 * fading until it falls below the board */
static void basin_flip_init(struct task *task)
{
    struct task_work *work = task->work;

    work->pos.y = tarai_drop_height;
    work->mode = FLIP_WAIT;
    work->timer = 0;
    task->step = basin_flip_step;
    task->draw = basin_flip_draw;
}

static void basin_flip_step(struct task *task)
{
    struct task_work *work = task->work;
    struct basin_motion *motion = (struct basin_motion *)task->motion;
    NJS_POINT3 player;
    Angle ang;

    switch (work->mode) {
    case FLIP_WAIT:
        work->timer++;
        if (TASK_TIMER(work) > BASIN_WAIT_STEPS) {
            NJS_VECTOR away = {0.0F, 0.0F, 0.0F};

            work->timer = 0;
            if (!player_get_history(0, 0, &player, NULL)) {
                player = work->pos;
            }
            BASIN(work)->head = player.y + tarai_head_height();
            BASIN(work)->floor = task->parent->work->pos.y;
            BASIN(work)->fade = 1.0F;
            /* the basin falls up to 4 from player 0's middle, in a random
             * direction. the factor is just under 4 times RAND_SCALE: SHC
             * rounds 0.00012207031F down, to H'38FFFFFF at 0x0c924390 */
            away.z = -(rand() * 0.00012207031F);
            motion->ang.y = (Angle)RANDOM_ANGLE();
            njPushMatrix(NULL);
            njUnitMatrix(NULL);
            TURN_VIA(njRotateY, motion->ang.y, ang);
            njCalcPoint(NULL, &away, &motion->fall);
            njPopMatrix(1);
            work->pos.x = player.x + motion->fall.x;
            work->pos.y = BASIN(work)->head + tarai_drop_height;
            work->pos.z = player.z + motion->fall.z;
            motion->speed.x = 0.0F;
            motion->speed.y = -1.0F;
            motion->speed.z = 0.0F;
            motion->gravity = -0.1F;
            motion->tilt = 0.0F;
            motion->tilt_speed = 0.0F;
            motion->ang.x = 0;
            motion->spin.x = 0;
            sound_play(SE_BASIN_FALL, 0, 0, 0);
            work->mode = FLIP_FALL;
        }
        break;
    case FLIP_FALL:
        motion->speed.y += motion->gravity;
        work->pos.y += motion->speed.y;
        if (work->pos.y <= BASIN(work)->head) {
            work->pos.y = BASIN(work)->head;
            work->timer = 0;
            work->mode = FLIP_BOUNCE;
            njUnitVector(&motion->fall);
            motion->speed.x = motion->fall.x * 0.6F;
            motion->speed.y = 0.8F;
            motion->speed.z = motion->fall.z * 0.6F;
            BASIN(work)->head = 0.0F;
            motion->spin.x = 0;
            motion->spin.y = 0;
            motion->spin.z = 0;
            motion->tilt = 0.0F;
            motion->tilt_speed = 0.6F;
        }
        break;
    case FLIP_BOUNCE:
        work->mode = FLIP_AWAY;
        break;
    case FLIP_AWAY:
        work->timer = 0;
        motion->speed.y += motion->gravity;
        njAddVector(&work->pos, &motion->speed);
        motion->spin.y += 10000;
        motion->spin.z = 2000;
        /* the lean whose tangent is a tenth of the tilt */
        motion->spin.x =
            -(Angle)(atan2f(motion->tilt * 0.1F, 1.0F) * ANGLE_PER_RADIAN);
        motion->tilt += motion->tilt_speed;
        BASIN(work)->fade -= BASIN_FADE;
        if (work->pos.y <= BASIN(work)->floor) {
            work->mode = FLIP_GONE;
        }
        break;
    case FLIP_GONE:
        task->parent->work->mode = TARAI_LANDED;
        task_kill(task);
        break;
    }
    basin_flip_draw(task);
}

static void basin_flip_draw(struct task *task)
{
    struct task_work *work = task->work;
    struct basin_motion *motion = (struct basin_motion *)task->motion;
    Angle ang;

    njSetTexture(&ec_tarai_texlist);
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    TURN_VIA(njRotateY, motion->ang.y, ang);
    TURN_VIA(njRotateX, motion->spin.x, ang);
    TURN_VIA(njRotateY, motion->spin.y, ang);
    TURN_VIA(njRotateZ, motion->spin.z, ang);
    draw_set_material(BASIN(work)->fade, 0.0F, 0.0F, 0.0F);
    draw_tree(&tarai_basin_object);
    draw_set_material(0.0F, 0.0F, 0.0F, 0.0F);
    njPopMatrix(1);
}

/* a basin that lands on player 0's head, swings over on it, and falls off
 * fading until it is below the board */
static void basin_swing_init(struct task *task)
{
    struct task_work *work = task->work;

    work->pos.y = tarai_drop_height;
    work->timer = 0;
    work->mode = SWING_WAIT;
    task->step = basin_swing_step;
    task->draw = basin_swing_draw;
}

static void basin_swing_step(struct task *task)
{
    struct task_work *work = task->work;
    struct basin_motion *motion = (struct basin_motion *)task->motion;
    NJS_POINT3 player;

    switch (work->mode) {
    case SWING_WAIT:
        work->timer++;
        if (TASK_TIMER(work) > BASIN_WAIT_STEPS) {
            if (!player_get_history(0, 0, &player, NULL)) {
                player = work->pos;
            }
            BASIN(work)->head = player.y + tarai_head_height();
            BASIN(work)->floor = task->parent->work->pos.y;
            BASIN(work)->fade = 1.0F;
            work->pos.x = player.x;
            work->pos.y = BASIN(work)->head + tarai_drop_height;
            work->pos.z = player.z;
            work->ang.x = 0;
            work->ang.y = 0;
            work->ang.z = 0;
            motion->ang.z = 0xc000;
            motion->gravity = 5.0F;
            motion->speed.x = 0.0F;
            motion->speed.y = -1.0F;
            motion->speed.z = 0.0F;
            motion->fall.x = 0.0F;
            motion->fall.y = -0.1F;
            motion->fall.z = 0.0F;
            motion->ang.y = (Angle)RANDOM_ANGLE();
            work->mode = SWING_FALL;
            work->timer = 0;
            sound_play(SE_BASIN_FALL, 0, 0, 0);
        }
        break;
    case SWING_FALL:
        motion->speed.y += motion->fall.y;
        work->pos.y += motion->speed.y;
        if (work->pos.y <= BASIN(work)->head) {
            work->pos.y = BASIN(work)->head;
            work->timer = 0;
            work->mode = SWING_SWING;
            work->ang.z = 0;
            motion->spin.x = 0;
            motion->spin.y = 100;
        }
        break;
    case SWING_SWING:
        work->timer++;
        if (TASK_TIMER(work) == 10) {
            sound_stop(SE_BASIN_FALL);
        }
        motion->spin.x += motion->spin.y;
        work->ang.z += (Angle)(motion->spin.x * njSin(work->ang.z)) + 10;
        if (work->ang.z >= 0x6000) {
            work->mode = SWING_OFF;
            motion->fall.x = 0.0F;
            motion->fall.y = -0.1F;
            motion->fall.z = 0.0F;
            motion->speed.x = -0.8F;
            /* the middle's speed as the basin lets go: the swing's rate
             * there, in radians a step, times SWING_ARM */
            motion->speed.y = ((motion->spin.x * njSin(work->ang.z)) + 10.0F) *
                              (SWING_ARM * 3.141592F / 32768.0F);
            motion->speed.z = 0.0F;
        }
        break;
    case SWING_OFF:
        njAddVector(&motion->speed, &motion->fall);
        njAddVector(&work->pos, &motion->speed);
        BASIN(work)->fade -= BASIN_FADE;
        work->ang.z += 0x400;
        if (work->pos.y <= BASIN(work)->floor) {
            work->mode = SWING_GONE;
        }
        break;
    case SWING_GONE:
        task->parent->work->mode = TARAI_LANDED;
        task_kill(task);
        break;
    }
    basin_swing_draw(task);
}

/* the basin, on its side, stands SWING_ARM above a pivot 1.5 along -x from
 * pos, and tips over about it by ang.z */
static void basin_swing_draw(struct task *task)
{
    struct task_work *work = task->work;
    struct basin_motion *motion = (struct basin_motion *)task->motion;
    Angle ang;

    njSetTexture(&ec_tarai_texlist);
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    njTranslate(NULL, -1.5F, 0.0F, 0.0F);
    TURN_VIA(njRotateZ, work->ang.z, ang);
    njTranslate(NULL, 0.0F, SWING_ARM, 0.0F);
    TURN_VIA(njRotateZ, motion->ang.z, ang);
    draw_set_material(BASIN(work)->fade, 0.0F, 0.0F, 0.0F);
    draw_tree(&tarai_basin_object);
    draw_set_material(0.0F, 0.0F, 0.0F, 0.0F);
    njPopMatrix(1);
}

/* a big, flat basin that lands over player 0, tips and fades */
static void basin_squash_init(struct task *task)
{
    struct task_work *work = task->work;

    work->pos.y = tarai_drop_height;
    work->timer = 0;
    work->mode = SQUASH_WAIT;
    task->step = basin_squash_step;
    task->draw = basin_squash_draw;
}

static void basin_squash_step(struct task *task)
{
    struct task_work *work = task->work;
    struct basin_motion *motion = (struct basin_motion *)task->motion;
    NJS_POINT3 player;

    switch (work->mode) {
    case SQUASH_WAIT:
        work->timer++;
        if (TASK_TIMER(work) > BASIN_WAIT_STEPS) {
            if (!player_get_history(0, 0, &player, NULL)) {
                player = work->pos;
            }
            work->scl.x = 4.0F;
            work->scl.y = 1.8F;
            work->scl.z = 4.0F;
            BASIN(work)->head = player.y + tarai_head_height();
            BASIN(work)->floor = task->parent->work->pos.y;
            BASIN(work)->fade = 1.0F;
            work->pos.x = player.x;
            work->pos.y = BASIN(work)->head + tarai_drop_height;
            work->pos.z = player.z;
            work->ang.x = 0;
            work->ang.y = 0;
            work->ang.z = 0;
            motion->speed.x = 0.0F;
            motion->speed.y = -1.0F;
            motion->speed.z = 0.0F;
            motion->fall.x = 0.0F;
            motion->fall.y = -0.1F;
            motion->fall.z = 0.0F;
            work->mode = SQUASH_FALL;
            work->timer = 0;
            sound_play(SE_BASIN_FALL, 0, 0, 0);
        }
        break;
    case SQUASH_FALL:
        motion->speed.y += motion->fall.y;
        work->pos.y += motion->speed.y;
        if (work->pos.y <= BASIN(work)->head) {
            work->timer = 0;
            work->mode = SQUASH_PAUSE;
            work->ang.y = (Angle)(rand() * (65535.0F * RAND_SCALE));
            work->ang.z = 0;
            motion->spin.z = 0;
            motion->spin.x = 2;
            sound_stop(SE_BASIN_FALL);
            sound_play(SE_BASIN_SQUASH, 0, 0, 0);
        }
        break;
    case SQUASH_PAUSE:
        work->timer++;
        if (TASK_TIMER(work) > SQUASH_PAUSE_STEPS) {
            work->timer = 0;
            work->mode = SQUASH_TIP;
        }
        break;
    case SQUASH_TIP:
        if (motion->spin.z < SQUASH_TIP_SPEED) {
            motion->spin.z += motion->spin.x;
        }
        work->ang.z += motion->spin.z;
        if (work->ang.z >= SQUASH_TIP_ANGLE) {
            work->mode = SQUASH_FADE;
        }
        break;
    case SQUASH_FADE:
        BASIN(work)->fade -= BASIN_FADE;
        if (BASIN(work)->fade < -1.0F) {
            work->mode = SQUASH_GONE;
        }
        break;
    case SQUASH_GONE:
        task->parent->work->mode = TARAI_LANDED;
        task_kill(task);
        break;
    }
    basin_squash_draw(task);
}

static void basin_squash_draw(struct task *task)
{
    struct task_work *work = task->work;
    Angle ang;

    njSetTexture(&ec_tarai_texlist);
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    TURN_VIA(njRotateY, work->ang.y, ang);
    TURN_VIA(njRotateZ, work->ang.z, ang);
    njScale(NULL, work->scl.x, work->scl.y, work->scl.z);
    draw_set_material(BASIN(work)->fade, 0.0F, 0.0F, 0.0F);
    draw_tree(&tarai_basin_object);
    draw_set_material(0.0F, 0.0F, 0.0F, 0.0F);
    njPopMatrix(1);
}

/* a glow at the tile's place, which brightens and fades over 40 steps */
static void tile_glow(Float x, Float y, Float z)
{
    struct task *task = task_create(TASK_WORK, 3, glow_step);
    struct task_work *work = task->work;

    work->pos.x = x;
    work->pos.y = y;
    work->pos.z = z;
    GLOW(work)->alpha = -1.0F;
    GLOW(work)->speed = 0.05F;
    /* @bug logic: gives the glow the board's draw, tarai_draw, where
     * glow_draw is meant, so while the game is paused a glow draws the board
     * at the tile and no glow. the MOV.L at 0x0c924d48 loads tarai_draw's
     * address, and the MOV.L at 0x0c924d6a stores it into task->draw */
    task->draw = tarai_draw;
}

static void glow_step(struct task *task)
{
    struct task_work *work = task->work;

    GLOW(work)->alpha += GLOW(work)->speed;
    if (GLOW(work)->speed > 0.0F) {
        if (GLOW(work)->alpha >= 0.0F) {
            GLOW(work)->alpha = 0.0F;
            GLOW(work)->speed = -0.05F;
        }
    } else {
        if (GLOW(work)->alpha <= -1.0F) {
            GLOW(work)->alpha = -1.0F;
            task_kill(task);
        }
    }
    glow_draw(task);
}

static void glow_draw(struct task *task)
{
    struct task_work *work = task->work;

    njSetTexture(&ec_tarai_texlist);
    draw_set_material(GLOW(work)->alpha, 0.0F, 0.0F, 0.0F);
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    draw_tree(&tarai_glow_object);
    njPopMatrix(1);
    draw_set_material(0.0F, 0.0F, 0.0F, 0.0F);
}

/* the ripple keeps its sway in GLOW(work)->alpha. in RIPPLE_GROW, the base
 * grows by 0.05 a step to its full size. in RIPPLE_SWAY, the sway falls to 1.
 * until it gets there, the middle's 11 pairs of points sway across x by up to
 * it. the draw sets RIPPLE_SWAY, so the base grows for one step alone */
void tarai_ripple_step(struct task *task)
{
    struct task_work *work = task->work;

    switch (work->mode) {
    case RIPPLE_GROW:
        work->scl.x += 0.05F;
        if (work->scl.x >= 1.0F) {
            work->scl.x = 1.0F;
            work->mode = RIPPLE_SWAY;
        }
        break;
    case RIPPLE_SWAY:
        if (GLOW(work)->speed >= -0.05F) {
            GLOW(work)->alpha -= 0.05F;
        } else {
            GLOW(work)->speed += 0.1F;
            GLOW(work)->alpha += GLOW(work)->speed;
        }
        work->ang.x += 1000;
        if (GLOW(work)->alpha <= 1.0F) {
            GLOW(work)->alpha = 1.0F;
            work->mode = RIPPLE_STILL;
        } else {
            NJS_POINT3 *point = tarai_ripple_object.model->points;
            Angle ang = work->ang.x;
            int i;

            for (i = 0; i < 11; i++) {
                Float sway = njSin(ang) * GLOW(work)->alpha;

                /* each pair's x as the model has it, moved by the sway */
                point[0].x = 36.189972F + sway;
                point[1].x = -36.189972F + sway;
                point += 2;
                ang += 5000;
            }
        }
        break;
    }
    tarai_ripple_draw(task);
}

void tarai_ripple_draw(struct task *task)
{
    struct task_work *work = task->work;

    njSetTexture(&ec_tarai_texlist);
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    njPushMatrix(NULL);
    njScale(NULL, work->scl.x, 1.0F, work->scl.x);
    draw_tree(&tarai_ripple_base_object);
    njPopMatrix(1);
    work->mode = RIPPLE_SWAY;
    njTranslate(NULL, 0.0F, 10.0F, 0.0F);
    draw_tree(&tarai_ripple_object);
    njPopMatrix(1);
}
