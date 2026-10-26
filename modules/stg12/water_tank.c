/* O KOWARESUISOU, a water tank that bursts when an attack or an enemy
 * touches it */
#include <mathf.h>
#include <ninja.h>
#include <sg_xpt.h>
#include <stdlib.h>

#include "camera/camera.h"
#include "collision/collision.h"
#include "debug/editor.h"
#include "draw/draw.h"
#include "draw/model.h"
#include "draw/tree.h"
#include "object/placement.h"
#include "sound/sound.h"
#include "stg12/water_column.h"
#include "stg12/water_tank.h"
#include "system/frame.h"
#include "system/task.h"
#include "system/texlists.h" /* NOLINT(misc-include-cleaner) */
#include "system/texture.h"
#include "system/util.h"

extern NJS_OBJECT water_tank_object; /* the tank's top */
extern NJS_OBJECT water_tank_glass_object;
/* the glass once burst. its model stays on the tank, and its children are
 * the pieces */
extern NJS_OBJECT water_tank_broken_object;

/* @hack type: an array, not the single NJS_ACTION it is. SHC pools the
 * action's address at 0x0c90c854, and reads the motion through it at
 * 0x0c90c752. as a single object, SHC pools the motion field's address
 * instead, and 11 of water_tank_run's 384 words and a pool word differ */
extern NJS_ACTION shelter_blue_light_action[];

#define SE_BURST 309

/* destroyed beyond 1000 units of the camera and player 0 */
#define RANGE (1000.0F * 1000.0F)

/* the light bobs LIGHT_BOB above and below LIGHT_HEIGHT over the tank's
 * foot. its phase in the bob turns LIGHT_BOB_SPEED each step */
#define LIGHT_HEIGHT 35.0F
#define LIGHT_BOB 6.0F
#define LIGHT_BOB_SPEED 100

/* once the tank bursts, the light's action runs LIGHT_BURST_SPEED frames a
 * step, while TANK_BROKEN shrinks the light. until then, it runs one */
#define LIGHT_BURST_SPEED 5.19F

/* work->timer counts down to the next bubble, which rises at 0. it starts
 * at FIRST_BUBBLE_WAIT, and after each bubble at a random count under
 * BUBBLE_WAIT */
#define FIRST_BUBBLE_WAIT 60
#define BUBBLE_WAIT 100.0F

/* the burst pushes every piece BURST_PUSH a step across x and z, away from
 * what touched the tank */
#define BURST_PUSH 0.7F

/* the children of water_tank_broken_object, the pieces the glass bursts
 * into */
#define PIECES 19

/* the speed a flying piece loses upward each step */
#define PIECE_GRAVITY 0.14F

/* a landed piece fades once TASK_TIMER(work) counts past REST_STEPS */
#define REST_STEPS 30

enum water_tank_mode { TANK_WAIT = 0, TANK_BURST = 1, TANK_BROKEN = 6 };
enum water_tank_piece_mode {
    PIECE_RISE = 3,
    PIECE_FALL = 4,
    PIECE_REST = 5,
    PIECE_FADE = 6
};

/* work->ang.x turns the light to face the camera, and work->ang.z holds
 * its phase in the bob. work->scl.y scales the light's width, which shrinks
 * once the tank bursts. work->scl.x and scl.z hold the burst's push */
struct water_tank_vars {
    Float frame; /* of shelter_blue_light_action, which the draw shows */
    Float speed; /* the action's frames a step */
    Float lift; /* the light's height above the tank's foot */
};

/* work->scl.x and scl.z hold a piece's own push, a unit vector out from the
 * tank's center. the piece's speed across x and z starts as that push plus
 * the burst's */
struct water_tank_piece_vars {
    Float floor; /* the height of the tank's foot, where the piece lands */
    NJS_OBJECT *node; /* the piece's child of water_tank_broken_object */
    Float alpha; /* for draw_set_material, from 0 down to -1 */
};

/* task->motion as a piece lays it out. speed is its velocity each step,
 * and the spins its turns each step about x and y. the spins take the words
 * struct task_motion leaves as unk_28 and unk_2c */
struct water_tank_piece_motion {
    Uint8 unk_00[4];
    NJS_VECTOR speed;
    Uint8 unk_10[24];
    Angle pitch_spin;
    Angle yaw_spin;
};

struct water_tank_bubble_vars {
    Float top; /* the height at which the bubble ends */
    Float speed; /* its rise each step */
};

#define VARS(work) ((struct water_tank_vars *)&(work)->vars)
#define PIECE(work) ((struct water_tank_piece_vars *)&(work)->vars)
#define BUBBLE_VARS(work) ((struct water_tank_bubble_vars *)&(work)->vars)

static void water_tank_run(struct task *task);
static void water_tank_draw(struct task *task);
static void water_tank_destroy(struct task *task);
static void water_tank_piece_start(struct task *task);
static void water_tank_piece_step(struct task *task);
static void water_tank_piece_draw(struct task *task);
static void water_tank_piece_destroy(struct task *task);
static void water_tank_bubble_start(struct task *task);
static void water_tank_bubble_step(struct task *task);
static void water_tank_bubble_draw(struct task *task);
static void water_tank_bubble_destroy(struct task *task);

static struct collision_shape water_tank_shape = {
    SHAPE_CYLINDER | SHAPE_HURTABLE | SHAPE_UNTURNED,
    0,
    PUSH_PLAYERS | PUSH_FIRM(PUSH_ENEMIES),
    {0.0F, 35.0F, 0.0F},
    15.0F,
    34.0F,
    0.0F,
    {0},
    0,
    NULL,
};

static NJS_TEXLIST water_tank_bubble_texlist = {
#include "stg12/water_tank/water_tank_bubble_texlist.inc"
};
static NJS_TEXANIM water_tank_bubble_texanim[1] = {
#include "stg12/water_tank/water_tank_bubble_texanim.inc"
};
/* one sprite draws every bubble. the draw moves it to each bubble in
 * turn */
static NJS_SPRITE water_tank_bubble_sprite = {
#include "stg12/water_tank/water_tank_bubble_sprite.inc"
};

/* each piece's push starts as a copy of this, so its y stays 0 */
static const NJS_VECTOR water_tank_zero = {0.0F, 0.0F, 0.0F};

void water_tank_start(struct task *task)
{
    struct task_work *work = task->work;

    work->timer = 0;
    work->mode = TANK_WAIT;
    VARS(work)->frame = 0.0F;
    VARS(work)->speed = 1.0F;
    VARS(work)->lift = 0.0F;
    work->scl.y = 1.0F;
    work->ang.x = 0;
    work->ang.z = RANDOM_ANGLE();
    work->timer = FIRST_BUBBLE_WAIT;
    if (!placement_editing(task)) {
        collision_init(task, &water_tank_shape, COLLISION_OBJECT, 1, 1);
    }
    task->step = water_tank_run;
    task->draw = water_tank_draw;
    task->destructor = water_tank_destroy;
}

static void water_tank_run(struct task *task)
{
    struct task_work *work = task->work;
    Float last;
    Float to_x;
    Float to_z;

    if (placement_try_leave_by(task, RANGE)) {
        return;
    }
    to_x = camera_work->pos.x - work->pos.x;
    to_z = camera_work->pos.z - work->pos.z;
    work->ang.x = (Angle)(atan2f(to_x, to_z) * ANGLE_PER_RADIAN);
    VARS(work)->frame += VARS(work)->speed;
    last = (Float)(shelter_blue_light_action->motion->nbFrame - 1);
    if (VARS(work)->frame >= last) {
        VARS(work)->frame -= last;
    }
    work->ang.z += LIGHT_BOB_SPEED;
    VARS(work)->lift = (njSin(work->ang.z) * LIGHT_BOB) + LIGHT_HEIGHT;
    switch (work->mode) {
    case TANK_WAIT:
        if ((work->flags & COLLISION_TOUCHING) &&
            work->collision->hit->kind != COLLISION_PLAYER) {
            struct task_work *other = work->collision->hit->owner->work;
            Float out_x = work->pos.x - other->pos.x;
            Float out_z = work->pos.z - other->pos.z;
            Float length = njSqrt((out_x * out_x) + (out_z * out_z));

            if (length != 0.0F) {
                length = BURST_PUSH / length;
                out_x *= length;
                out_z *= length;
            } else {
                out_z = out_x = length = 0.0F;
            }
            /* @hack redundant: a store of length to work->scl.y that never
             * runs. SHC still copies length to FR4 for it, at 0x0c90c884;
             * without it, the copy goes, the code after it moves up, and 107
             * of 384 words differ */
            if (0) {
                work->scl.y = length;
            }
            work->scl.x = out_x;
            work->scl.z = out_z;
            work->timer = 0;
            work->flags &= ~(COLLISION_DAMAGED | COLLISION_TOUCHING);
            work->mode = TANK_BURST;
            /* a count of 2 runs past the one shape collision_init copied
             * into the record. nothing shows why the burst stores it. no
             * check reads the count, since the tank registers the record only
             * in TANK_WAIT */
            work->collision->count = 2;
            task_destroy_children(task);
            sound_play(SE_BURST, 0, 0, 0);
        } else {
            if (!placement_editing(task)) {
                collision_register(work);
            }
            if (work->timer == 0) {
                struct task *child =
                    task_create_child(TASK_WORK, water_tank_bubble_start, task);
                struct task_work *bubble = child->work;

                bubble->pos.x += RANDOM(-0.5F, 0.5F) * 26.0F;
                bubble->pos.y += 9.0F;
                bubble->pos.z += RANDOM(-0.5F, 0.5F) * 26.0F;
                BUBBLE_VARS(bubble)->top = work->pos.y + 60.0F;
                work->timer = (int)(rand() * (BUBBLE_WAIT * RAND_SCALE));
            } else {
                work->timer--;
            }
            task_step_children(task);
        }
        break;
    case TANK_BURST: {
        struct task *column = task_create(TASK_WORK, 3, water_column_start);
        struct task_work *spill = column->work;
        NJS_OBJECT *node;
        Float x;
        Float y;
        Float z;
        int count;

        spill->pos.x = work->pos.x;
        spill->pos.y = work->pos.y;
        spill->pos.z = work->pos.z;
        node = water_tank_broken_object.child;
        x = work->pos.x;
        y = work->pos.y;
        z = work->pos.z;
        count = PIECES;
        do {
            struct task *child;
            struct task_work *piece;
            /* @hack spelling: node->pos read through a cursor, which SHC walks
             * with FMOV.S @R4+ (0x0c90ca02, 0x0c90ca04). read as an array,
             * it goes through @(R0,R4), water_tank_run runs 4 bytes longer,
             * and 96 of 384 words differ */
            Float *point = node->pos;
            Float dx;
            Float dy;
            Float dz;
            NJS_VECTOR out;

            out = water_tank_zero;
            dx = *point++;
            dy = *point++;
            dz = *point;
            out.x = dx * 0.1F;
            out.z = dz * 0.1F;
            /* @hack redundant: a copy of dz to out.y that never runs. it
             * keeps dx in FR13 and dz in FR14 (0x0c90ca02, 0x0c90ca0e);
             * without it, the two registers trade, and 6 of 384 words
             * differ */
            if (0) {
                out.y = dz;
            }
            njUnitVector(&out);
            child = task_create_child(TASK_WORK | TASK_MOTION,
                                      water_tank_piece_start, task);
            piece = child->work;
            piece->pos.x = x + dx;
            piece->pos.y = y + dy;
            piece->pos.z = z + dz;
            piece->scl.x = out.x;
            piece->scl.z = out.z;
            PIECE(piece)->floor = y;
            PIECE(piece)->node = node;
            node = node->sibling;
            count--;
        } while (count != 0);
        task_step_children(task);
        work->timer = 0;
        VARS(work)->speed = LIGHT_BURST_SPEED;
        work->mode = TANK_BROKEN;
    } break;
    case TANK_BROKEN:
        task_step_children(task);
        if (work->scl.y >= 0.0F) {
            work->scl.y -= 0.05F;
        }
        break;
    }
    water_tank_draw(task);
}

static void water_tank_draw(struct task *task)
{
    struct task_work *work = task->work;
    Angle ang;

    if (frame_step == 0) {
        texture_set_stage();
        njPushMatrix(NULL);
        njTranslateV(NULL, &work->pos);
        njPushMatrix(NULL);
        TURN_VIA(njRotateY, work->ang.y, ang);
        draw_tree(&water_tank_object);
        if (work->mode == TANK_WAIT) {
            draw_tree(&water_tank_glass_object);
        } else {
            draw_model(water_tank_broken_object.model);
        }
        njPopMatrix(1);
        njTranslate(NULL, 0.0F, VARS(work)->lift, 0.0F);
        TURN_VIA(njRotateY, work->ang.x, ang);
        njScale(NULL, work->scl.y, 1.0F, work->scl.y);
        draw_action(shelter_blue_light_action, VARS(work)->frame);
        njPopMatrix(1);
    }
}

static void water_tank_destroy(struct task *task)
{
    task_destroy_children(task);
}

static void water_tank_piece_start(struct task *task)
{
    struct task_work *work = task->work;
    struct water_tank_piece_motion *motion =
        (struct water_tank_piece_motion *)task->motion;
    struct task_work *tank;
    Float x;
    Float z;

    TASK_TIMER(work) = 0;
    PIECE(work)->alpha = 0.0F;
    work->mode = PIECE_RISE;
    tank = task->parent->work;
    x = tank->scl.x;
    z = tank->scl.z;
    motion->speed.x = work->scl.x + x;
    motion->speed.y = RANDOM(0.5F, 1.5F);
    motion->speed.z = work->scl.z + z;
    motion->yaw_spin = (Angle)(RANDOM(-0.5F, 0.5F) * 1800.0F);
    motion->pitch_spin = (Angle)(RANDOM(-0.5F, 0.5F) * 3000.0F);
    task->step = water_tank_piece_step;
    task->draw = water_tank_piece_draw;
    task->destructor = water_tank_piece_destroy;
    water_tank_piece_draw(task);
}

static void water_tank_piece_step(struct task *task)
{
    struct task_work *work = task->work;
    struct water_tank_piece_motion *motion =
        (struct water_tank_piece_motion *)task->motion;
    /* @hack spelling: work->pos and motion->speed read and written through
     * cursors, which SHC walks with FMOV.S @R14+ and @R13+ (0x0c90ccbc,
     * 0x0c90ccca). read and written as fields, they go through @(R0,Rn), the
     * step runs 4 bytes longer, and 71 of 145 words differ */
    Float *point = &work->pos.x;
    Float *speed = &motion->speed.x;
    Float x = *point++;
    Float y = *point++;
    Float z = *point++;
    Float speed_x = *speed++;
    Float speed_y = *speed++;
    Float speed_z = *speed++;

    point -= 3;
    speed -= 3;
    switch (work->mode) {
    case PIECE_RISE:
        x += speed_x;
        y += speed_y;
        z += speed_z;
        speed_y -= PIECE_GRAVITY;
        work->ang.y += motion->yaw_spin;
        work->ang.x += motion->pitch_spin;
        if (speed_y < 0.0F) {
            work->mode = PIECE_FALL;
        }
        water_tank_piece_draw(task);
        break;
    case PIECE_FALL:
        y += speed_y;
        z += speed_z;
        x += speed_x;
        work->ang.y += motion->yaw_spin;
        work->ang.x += motion->pitch_spin;
        speed_y -= PIECE_GRAVITY;
        if (y <= PIECE(work)->floor) {
            work->mode = PIECE_REST;
        }
        water_tank_piece_draw(task);
        break;
    case PIECE_REST:
        TASK_TIMER(work)++;
        if (TASK_TIMER(work) > REST_STEPS) {
            work->mode = PIECE_FADE;
        }
        water_tank_piece_draw(task);
        break;
    case PIECE_FADE:
        PIECE(work)->alpha -= 0.05F;
        if (PIECE(work)->alpha <= -1.0F) {
            PIECE(work)->alpha = -1.0F;
            task_kill(task);
        }
        water_tank_piece_draw(task);
        break;
    }
    *point++ = x;
    *point++ = y;
    *point = z;
    *speed++ = speed_x;
    *speed++ = speed_y;
    *speed = speed_z;
}

static void water_tank_piece_draw(struct task *task)
{
    struct task_work *work = task->work;
    Angle ang;

    if (frame_step == 0) {
        texture_set_stage();
        njPushMatrix(NULL);
        njTranslateV(NULL, &work->pos);
        TURN_VIA(njRotateX, work->ang.x, ang);
        TURN_VIA(njRotateY, work->ang.y, ang);
        draw_set_material(PIECE(work)->alpha, 0.0F, 0.0F, 0.0F);
        draw_model(PIECE(work)->node->model);
        draw_set_material(0.0F, 0.0F, 0.0F, 0.0F);
        njPopMatrix(1);
    }
}

static void water_tank_piece_destroy(struct task *task)
{
}

static void water_tank_bubble_start(struct task *task)
{
    struct task_work *work = task->work;

    BUBBLE_VARS(work)->speed = (rand() * (1.0F / 65536.0F)) + 0.2F;
    task->step = water_tank_bubble_step;
    task->draw = water_tank_bubble_draw;
    task->destructor = water_tank_bubble_destroy;
}

static void water_tank_bubble_step(struct task *task)
{
    struct task_work *work = task->work;

    work->pos.y += BUBBLE_VARS(work)->speed;
    if (work->pos.y > BUBBLE_VARS(work)->top) {
        task_kill(task);
    }
    water_tank_bubble_draw(task);
}

static void water_tank_bubble_draw(struct task *task)
{
    struct task_work *work = task->work;

    if (frame_step == 0) {
        water_tank_bubble_sprite.p.x = work->pos.x;
        water_tank_bubble_sprite.p.y = work->pos.y;
        water_tank_bubble_sprite.p.z = work->pos.z;
        njColorBlendingMode(NJD_SOURCE_COLOR, NJD_COLOR_BLENDING_SRCALPHA);
        njColorBlendingMode(NJD_DESTINATION_COLOR, NJD_COLOR_BLENDING_ONE);
        draw_set_material(1.0F, 1.0F, 1.0F, 1.0F);
        njDrawSprite3D(&water_tank_bubble_sprite, 0,
                       NJD_SPRITE_COLOR | NJD_SPRITE_SCALE | NJD_SPRITE_ALPHA);
    }
}

static void water_tank_bubble_destroy(struct task *task)
{
}
