/* CANNON_S, the cannon that fires the players out. while a player touches it,
 * it holds both players at its position, their pads off. after enough touches,
 * its motion plays while it holds them. it then puts them along the world's z,
 * where they look either way. it fires them up and along the world's z. its
 * barrel kicks out along the cannon's own z. it gives their pads back once
 * nothing touches it, and fires no more */
#include <ninja.h>
#include <sg_xpt.h>

#include "collision/collision.h"
#include "collision/world.h"
#include "debug/editor.h"
#include "draw/draw.h"
#include "draw/model.h"
#include "draw/tree.h"
#include "object/placement.h"
#include "player/player.h"
#include "sound/sound.h"
#include "stage/stage.h"
#include "stg06/cannon_s.h"
#include "stg06/models.h"
#include "system/pad.h"
#include "system/task.h"
#include "system/util.h"

extern NJS_OBJECT cannon_s_object;
extern NJS_MOTION cannon_s_motion;

/* it loads on the next touch once held has passed HOLD_STEPS. the players
 * look about while its timer is at most LOOK_STEPS, and it fires on the
 * step after */
#define HOLD_STEPS 60
#define LOOK_STEPS 54
/* the players look up to 70 degrees either way and back. the swing's phase
 * gains LOOK_PACE degrees a step */
#define LOOK_PACE 10
#define LOOK_SWING (70.0F * 65536.0F / 360.0F)
/* how far along the world's z from the cannon players 0 and 1 stand to look
 * about */
#define AIM_Z0 90.0F
#define AIM_Z1 80.0F
/* each player flies off at FIRE_SPEED. as it fires, the barrel stands KICK
 * out along the cannon's own z */
#define FIRE_SPEED 6.0F
#define KICK 40.0F
/* a spring takes KICK_PULL of the barrel's offset from its speed each
 * step. the speed keeps KICK_DAMP of itself. a barrel that would pass
 * rest stops there, and its speed takes KICK_BOUNCE of itself */
#define KICK_PULL 0.3F
#define KICK_DAMP 0.95F
#define KICK_BOUNCE (-0.65F)
/* the barrel's mouth is MOUTH_AHEAD ahead of the cannon. the world tests
 * the barrel's shape only while a player is within MOUTH_NEAR of it */
#define MOUTH_AHEAD 55.0F
#define MOUTH_NEAR 76.0F
/* the reach its collision record takes, in place of its shape's. it is a
 * plain distance, as collision_work's radius is, not a square */
#define CANNON_REACH 10000.0F

/* its sounds as its motion starts, as it moves the players, and as it
 * fires */
#define SE_LOAD 161
#define SE_AIM 162
#define SE_FIRE 184
/* the voice as it fires, for Sonic and for any other character */
#define VOICE_SONIC_FIRE 1421
#define VOICE_FIRE 1381

/* the cannon's own layout of task_work */
struct cannon_s_work {
    Sint8 mode;
    Sint8 unk_01;
    Uint8 unk_02;
    Uint8 held; /* the steps a player has touched it on, while it waits */
    Sint16 flags;
    Uint16 timer;
    NJS_OBJECT *world; /* the barrel's shape, from world_object_alloc */
    Float kick_speed;
    Float kick; /* the barrel's offset from rest along the cannon's z */
    struct angles ang;
    NJS_POINT3 pos;
    NJS_POINT3 scl;
    struct collision_work *collision;
    struct event_work *event;
};

enum cannon_s_mode {
    CANNON_WAIT, /* until a player has touched it long enough */
    CANNON_LOAD, /* its motion plays */
    CANNON_AIM, /* for one step: it puts the players along z */
    CANNON_LOOK, /* the players look about */
    CANNON_FIRE, /* it fires, and the pads stay off while anything touches it */
    CANNON_DONE
};

/* @hack redundant: the mode ORed into PAD_BOTH_PLAYERS, whose low byte
 * stays 0xff. SHC then passes it as MOV #-1, as at 0x0c90dafc, where
 * PAD_BOTH_PLAYERS alone loads 0xff from a pool at each call; without it,
 * cannon_s_run runs 32 bytes long and 329 of 462 words differ */
#define BOTH_PLAYERS(work) ((Sint8)((work)->mode | PAD_BOTH_PLAYERS))

static struct collision_shape cannon_s_shape = {
    SHAPE_CYLINDER2 | SHAPE_NO_PUSH,
    0,
    PUSH_FIRM(PUSH_ALL),
    {0.0F, 0.0F, 0.0F},
    30.0F,
    90.0F,
    0.0F,
    {0x4000, 0x4000, 0x4000},
    0,
    NULL,
};
/* a box that nothing uses */
static struct collision_shape dat_0cae6a7c = {
    SHAPE_RECTANGLE,
    0,
    PUSH_ALL | PUSH_FIRM(PUSH_ALL),
    {0.0F, 0.0F, 30.0F},
    151.0F,
    73.0F,
    30.0F,
    {0, 0, 0},
    0,
    NULL,
};

static void cannon_s_init(struct task *task);
static void cannon_s_destroy(struct task *task);
static void cannon_s_run(struct task *task);
static void cannon_s_draw(struct task *task);

/* @hack redundant: register on task, which keeps it in R14 across the range
 * check (0x0c90d908); without it, task is kept at sp+0, 11 of 14 words
 * differ and cannon_s_init starts 36 bytes late */
void cannon_s_step(register struct task *task)
{
    if (placement_try_leave(task)) {
        return;
    }
    cannon_s_init(task);
}

/* outside the set editor, it takes a collision record. it also puts the
 * barrel's shape in the world where the cannon stands */
static void cannon_s_init(struct task *task)
{
    struct cannon_s_work *work = (struct cannon_s_work *)task->work;
    NJS_OBJECT *world;
    NJS_OBJECT *shape;

    if (!placement_editing(task)) {
        collision_init(task, &cannon_s_shape, COLLISION_OBJECT, 1, 1);
        work->collision->radius = CANNON_REACH;
        work->world = world = world_object_alloc();
        shape = &cannon_s_object;
        /* @hack redundant: evalflags set from a select whose condition is a
         * constant, which SHC folds to a 0 in R2 (0x0c90d95c), apart from
         * the 0 in R13 that the angles and the sibling take; set to a plain
         * 0, 56 of 87 words differ */
        /* @hack spelling: the store in one statement with the scale's first,
         * which loads the scale's 1 (0x0c90d956) ahead of its first offset;
         * in two statements, the two loads trade and 2 of 87 words differ */
        /* NOLINTNEXTLINE(misc-redundant-expression) */
        world->evalflags = 0.0F ? 0 : 0, world->scl[0] = 1.0F;
        world->scl[1] = 1.0F;
        world->scl[2] = 1.0F;
        world->ang[0] = 0;
        world->ang[1] = work->ang.y;
        world->ang[2] = 0;
        world->pos[0] = work->pos.x;
        world->pos[1] = work->pos.y;
        world->pos[2] = work->pos.z;
        world->model = shape->child->model;
        world->child = shape->child->child;
        world->sibling = NULL;
        work->flags |= WORLD_TESTED;
        world_add(WORLD_MEDIUM | WORLD_SOLID, task, world);
    }
    task->draw = cannon_s_draw;
    task->step = cannon_s_run;
    task->destructor = cannon_s_destroy;
    work->timer = 0;
    work->mode = CANNON_WAIT;
    work->ang.x = 0;
    work->ang.z = 0;
    work->held = 0;
}

static void cannon_s_destroy(struct task *task)
{
    struct cannon_s_work *work = (struct cannon_s_work *)task->work;

    if (work->world) {
        world_remove(task, work->world);
        world_object_free(work->world);
    }
}

static void cannon_s_run(struct task *task)
{
    struct cannon_s_work *work = (struct cannon_s_work *)task->work;
    NJS_OBJECT *world;
    Angle ang;

    if (placement_try_leave(task)) {
        return;
    }
    cannon_s_draw(task);
    if (placement_editing(task)) {
        return;
    }
    if (work->mode) {
        work->timer++;
    }
    /* @hack redundant: never-run calls and tests, which SHC writes none of
     * but counts in choosing registers. they keep player_work's address in
     * R13, 0 in R12, sound_play's and pad_disable_player's addresses in R11
     * and R10, player_set_position's in R9, and in R8 the 2 that the hit test
     * and CANNON_AIM's store take (0x0c90da7e to 0x0c90da96); without them,
     * 81 of 462 words differ */
    if (0) {
        player_set_position(0, 0.0F, 0.0F, 0.0F);
        if (player_work[1]) {
        }
        if (player_work[1]) {
        }
        sound_play(0, 0, 0, 0);
        sound_play(0, 0, 0, 0);
        pad_disable_player(0);
        pad_disable_player(0);
        work->mode = CANNON_AIM;
        work->mode = CANNON_AIM;
        work->mode = CANNON_AIM;
        work->mode = CANNON_AIM;
    }
    switch (work->mode) {
    case CANNON_WAIT:
        if ((work->collision->flags & COLLISION_HIT) &&
            work->collision->hit->kind == COLLISION_PLAYER) {
            pad_disable_player(BOTH_PLAYERS(work));
            player_set_position(0, work->pos.x, work->pos.y, work->pos.z);
            if (player_work[1]) {
                player_set_position(1, work->pos.x, work->pos.y, work->pos.z);
            }
            if (work->held++ > HOLD_STEPS) {
                work->mode = CANNON_LOAD;
                work->timer = 0;
                sound_play(SE_LOAD, 0, 0, 0);
            }
        }
        break;
    case CANNON_LOAD:
        if (work->timer > cannon_s_motion.nbFrame) {
            work->mode = CANNON_AIM;
            work->timer = 0;
            sound_play(SE_AIM, 0, 0, 0);
        }
        pad_disable_player(BOTH_PLAYERS(work));
        player_set_position(0, work->pos.x, work->pos.y, work->pos.z);
        if (player_work[1]) {
            player_set_position(1, work->pos.x, work->pos.y, work->pos.z);
        }
        break;
    case CANNON_AIM:
        work->mode = CANNON_LOOK;
        work->timer = 0;
        pad_disable_player(BOTH_PLAYERS(work));
        player_set_position(0, work->pos.x, work->pos.y, work->pos.z + AIM_Z0);
        if (player_work[1]) {
            player_set_position(1, work->pos.x, work->pos.y,
                                work->pos.z + AIM_Z1);
        }
        break;
    case CANNON_LOOK: {
        struct angles turn;
        /* @hack spelling: turn reached through look, which holds its address in
         * R9 from 0x0c90dc06 for the stores and the calls; through &turn, 210
         * of 462 words differ */
        struct angles *look = &turn;

        look->x = 0;
        look->y = (Angle)(njSin((Angle)(work->timer * LOOK_PACE *
                                        (65536.0F / 360.0F))) *
                          LOOK_SWING);
        look->z = 0;
        pad_disable_player(BOTH_PLAYERS(work));
        if (work->timer > LOOK_STEPS) {
            if (get_character() == CHARACTER_SONIC) {
                sound_play(VOICE_SONIC_FIRE, 0, 0, 0);
            } else {
                sound_play(VOICE_FIRE, 0, 0, 0);
            }
            work->mode = CANNON_FIRE;
            work->timer = 0;
            sound_play(SE_FIRE, 0, 0, 0);
        } else {
            player_event_stop(0);
            player_set_look_angles(0, look);
            if (player_work[1]) {
                player_event_stop(1);
                player_set_look_angles(1, look);
            }
        }
    } break;
    case CANNON_FIRE:
        pad_disable_player(BOTH_PLAYERS(work));
        if (work->timer == 1) {
            NJS_VECTOR line;

            line.x = 0.0F;
            line.y = 2.5F;
            line.z = 5.0F;
            njUnitVector(&line);
            player_fling(0, FIRE_SPEED, &line);
            if (player_work[1]) {
                line.x = 0.5F;
                line.y = 2.5F;
                line.z = 5.0F;
                njUnitVector(&line);
                player_fling(1, FIRE_SPEED, &line);
            }
            work->kick = KICK;
        } else if (!(work->collision->flags & COLLISION_HIT)) {
            work->mode = CANNON_DONE;
            work->timer = 0;
            pad_enable_player(BOTH_PLAYERS(work));
        }
        break;
    case CANNON_DONE:
        break;
    default:
        work->timer = 0;
        break;
    }
    work->kick_speed += -(work->kick * KICK_PULL);
    work->kick_speed *= KICK_DAMP;
    if (work->kick + work->kick_speed <= 0.0F) {
        work->kick = 0.0F;
        work->kick_speed *= KICK_BOUNCE;
    } else {
        work->kick += work->kick_speed;
    }
    collision_register((struct task_work *)work);
    world = work->world;
    if (world) {
        NJS_POINT3 mouth;

        mouth.x = 0.0F;
        mouth.y = 0.0F;
        mouth.z = MOUTH_AHEAD;
        njPushMatrix(&_unit_matrix_);
        njTranslateV(NULL, &work->pos);
        TURN_VIA(njRotateY, work->ang.y, ang);
        njCalcPoint(NULL, &mouth, &mouth);
        njPopMatrix(1);
        world->pos[0] = work->pos.x;
        world->pos[1] = work->pos.y;
        world->pos[2] = work->pos.z;
        world->ang[1] = work->ang.y;
        if (player_in_range(&mouth, MOUTH_NEAR)) {
            work->flags |= WORLD_TESTED;
        } else {
            work->flags &= ~WORLD_TESTED;
        }
    }
}

/* while it waits and loads, it draws its motion. otherwise it draws its
 * parts: the body, the barrel out by its kick, and the rest */
static void cannon_s_draw(struct task *task)
{
    struct cannon_s_work *work = (struct cannon_s_work *)task->work;
    NJS_OBJECT *object = &cannon_s_object;
    NJS_MOTION *motion = &cannon_s_motion;
    Angle ang;
    int frame;

    if (placement_try_leave(task)) {
        return;
    }
    njSetTexture(&obj_skydeck_texlist);
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    TURN_VIA(njRotateY, work->ang.y, ang);
    switch (work->mode) {
    case CANNON_WAIT:
    case CANNON_LOAD:
        frame =
            work->timer >= motion->nbFrame ? motion->nbFrame - 1 : work->timer;
        draw_motion(object, motion, (Float)frame);
        break;
    default:
        draw_model(object->model);
        object = object->child;
        njPushMatrix(NULL);
        njTranslate(NULL, 0.0F, 0.0F, work->kick);
        draw_model(object->model);
        draw_tree(object->child);
        njPopMatrix(1);
        while ((object = object->sibling) != NULL) {
            if (object->evalflags & NJD_EVAL_UNIT_POS) {
                draw_model(object->model);
            } else {
                njPushMatrix(NULL);
                njTranslateV(NULL, (NJS_VECTOR *)object->pos);
                draw_model(object->model);
                njPopMatrix(1);
            }
        }
        break;
    }
    njPopMatrix(1);
}
