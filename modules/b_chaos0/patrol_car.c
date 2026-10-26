/* O PATO0, Chaos 0's patrol car: a solid in the world, parked with its
 * light bar turning. its headlights glow toward the camera and light two
 * pools on the road ahead */
#include <ninja.h>
#include <sg_xpt.h>

#include "b_chaos0/patrol_car.h"
#include "b_chaos0/textures.h"
#include "camera/camera.h"
#include "chaos/chaos.h"
#include "collision/world.h"
#include "draw/draw.h"
#include "draw/motion.h"
#include "player/player.h"
#include "system/task.h"
#include "system/texture.h"
#include "system/util.h"

/* @hack type: an array, whose address patrol_car_run keeps in R4 to read
 * the motion at offset 4 (MOV.L @(4,R4),R3 at 0x0c904fca). declared as one
 * NJS_ACTION, SHC pools the motion's address instead, and 27 of the 243
 * words of patrol_car_draw and patrol_car_run differ */
extern NJS_ACTION chaos0_patrol_car_action[];
extern NJS_MODEL chaos0_patrol_car_solid_model;

/* world_collect_near_players takes the car's solid only while a player is
 * within its model's radius and TEST_MARGIN more */
#define TEST_MARGIN 20.0F
/* the frames of its motion the car plays a step, and the turn its light bar
 * takes a step */
#define CAR_PACE 0.4F
#define BEACON_TURN 0x600

/* car_texanims' entries: a lamp's glow, and a headlight's pool on the road */
#define GLOW 0
#define POOL 1

/* a headlight's glow is drawn HEADLIGHT_NEAR nearer the camera than its
 * lamp, and a beacon's BEACON_NEAR. a glow's scale is its brightness times
 * HEADLIGHT_SIZE or BEACON_SIZE */
#define HEADLIGHT_NEAR 16.0F
#define BEACON_NEAR 6.0F
#define HEADLIGHT_SIZE 1.5F
#define BEACON_SIZE 0.6F
/* the light bar's lamps, the first two red and the rest blue */
#define BEACONS 4
/* the pitch that lays a pool of light flat on the road */
#define LIE_FLAT 0x4000

struct patrol_car_vars {
    /* the car's solid, from world_object_alloc. patrol_car_destroy takes
     * it out of the world and frees it */
    NJS_OBJECT *solid;
};

#define VARS(work) ((struct patrol_car_vars *)&(work)->vars)
/* its motion's frame, and its light bar's turn */
#define FRAME(work) ((work)->scl.x)
#define BEACON(work) ((work)->ang.z)

static void patrol_car_draw_headlights(struct task_work *car_work,
                                       NJS_MATRIX *car);
static void patrol_car_draw_beacons(struct task_work *work, NJS_MATRIX *matrix);
static void patrol_car_draw(struct task *task);
static void patrol_car_run(struct task *task);
static void patrol_car_destroy(struct task *task);

static NJS_TEXANIM car_texanims[2] = {
#include "b_chaos0/patrol_car/car_texanims.inc"
};
/* the lamps, in the car's frame. the two headlights come first, then the
 * tail lights, which nothing draws */
static NJS_POINT3 headlight_plus_z = {-21.51F, 6.6F, 9.0F};
static NJS_POINT3 headlight_minus_z = {-21.5F, 6.6F, -9.0F};
static NJS_POINT3 tail_lights[2] = {
    {24.9F, 8.0F, 7.6F},
    {24.9F, 8.0F, -7.6F},
};
/* the light bar's lamps, and the angle each faces at, added to the bar's
 * turn */
static NJS_POINT3 beacon_lamps[BEACONS] = {
    {3.7F, 18.3F, 6.0F},
    {3.7F, 17.5F, 3.5F},
    {3.7F, 17.5F, -3.5F},
    {3.7F, 18.3F, -6.0F},
};
static Angle beacon_angles[BEACONS] = {0, 0x4000, 0x2000, 0x6000};

/* the glows' and the pools' sprite, which chaos0_patrol_car_step sets up */
static NJS_SPRITE car_sprite;
/* the car's matrix, which patrol_car_draw builds from its pos and y angle */
static NJS_MATRIX car_matrix;

void chaos0_patrol_car_step(struct task *task)
{
    struct task_work *work = task->work;
    NJS_OBJECT *solid;

    car_sprite.tlist = &chaos0_effect_texlist;
    car_sprite.tanim = car_texanims;
    car_sprite.sx = 0.2F;
    car_sprite.sy = 0.2F;
    car_sprite.p.x = 0.0F;
    car_sprite.p.y = 0.0F;
    car_sprite.p.z = 0.0F;
    VARS(work)->solid = solid = world_object_alloc();
    solid->evalflags = NJD_EVAL_BREAK;
    solid->scl[0] = 1.0F;
    solid->scl[1] = 1.0F;
    solid->scl[2] = 1.0F;
    solid->ang[0] = work->ang.x;
    solid->ang[1] = work->ang.y;
    solid->ang[2] = work->ang.z;
    solid->pos[0] = work->pos.x;
    solid->pos[1] = work->pos.y;
    solid->pos[2] = work->pos.z;
    solid->model = &chaos0_patrol_car_solid_model;
    solid->child = NULL;
    solid->sibling = NULL;
    world_add(0, task, solid);
    task->destructor = patrol_car_destroy;
    task->step = patrol_car_run;
    task->draw = patrol_car_draw;
}

/* draw each headlight's glow, the brighter the more the headlight faces the
 * camera, and at brightness 0 while it faces away. car is the car's matrix */
static void patrol_car_draw_headlights(struct task_work *car_work,
                                       NJS_MATRIX *car)
{
    /* @hack spelling: car_work copied to work, which SHC keeps in R12 (MOV
     * R4,R12 at 0x0c90410e). with car_work read in its place, 30 of 430
     * words differ */
    struct task_work *work = car_work;
    /* @hack type: the matrix copied to a volatile local, which SHC
     * keeps at sp+4 (MOV.L R5,@(4,R15) at 0x0c904110). without volatile, 29
     * of 430 words differ */
    NJS_MATRIX *volatile matrix = car;
    NJS_POINT3 origin;
    NJS_POINT3 pos;
    NJS_POINT3 lamp_plus_z;
    NJS_POINT3 lamp_minus_z;
    NJS_VECTOR dir;
    NJS_VECTOR normal;
    NJS_VECTOR view;
    Float facing;
    Float bright;
    Float size;
    Angle ang;
    /* @hack spelling: njUnitVector called through unit, which keeps it in R9
     * and njColorBlendingMode in R8 (0x0c904132, 0x0c90410c). called by
     * name, the two registers swap, and 11 of 430 words differ */
    Float (*unit)(NJS_VECTOR *);

    origin.x = origin.y = origin.z = 0.0F;
    njColorBlendingMode(NJD_SOURCE_COLOR, NJD_COLOR_BLENDING_SRCALPHA);
    njColorBlendingMode(NJD_DESTINATION_COLOR, NJD_COLOR_BLENDING_ONE);
    njSetTexture(&chaos0_effect_texlist);
    dir.x = -1.0F;
    dir.y = 0.0F;
    dir.z = 0.1F;
    unit = njUnitVector;
    unit(&dir);
    njPushMatrix(&_unit_matrix_);
    TURN_VIA(njRotateY, work->ang.y, ang);
    njCalcVector(NULL, &dir, &normal);
    njPopMatrix(1);
    view.x = work->pos.x - camera_work->pos.x;
    view.y = work->pos.y - camera_work->pos.y;
    view.z = work->pos.z - camera_work->pos.z;
    unit(&view);
    facing = -njInnerProduct(&normal, &view);
    if (facing < 0.0F) {
        bright = 0.0F;
    } else {
        facing = (facing - 0.5F) * 2.0F;
        bright = (facing * facing * facing) + 0.1F;
    }
    draw_set_material(bright, 1.0F, 1.0F, 0.95F);
    size = bright * HEADLIGHT_SIZE;
    njPushMatrix(matrix);
    njTranslate(NULL, headlight_plus_z.x, headlight_plus_z.y,
                headlight_plus_z.z);
    njCalcPoint(NULL, &origin, &lamp_plus_z);
    njPopMatrix(1);
    dir.x = camera_work->pos.x - lamp_plus_z.x;
    dir.y = camera_work->pos.y - lamp_plus_z.y;
    dir.z = camera_work->pos.z - lamp_plus_z.z;
    unit(&dir);
    pos.x = lamp_plus_z.x + (dir.x * HEADLIGHT_NEAR);
    pos.y = lamp_plus_z.y + (dir.y * HEADLIGHT_NEAR);
    pos.z = lamp_plus_z.z + (dir.z * HEADLIGHT_NEAR);
    njPushMatrix(NULL);
    njTranslate(NULL, pos.x, pos.y, pos.z);
    TURN_VIA(njRotateY, camera_work->ang.y, ang);
    njScale(NULL, size, size, size);
    njDrawSprite3D(&car_sprite, GLOW, NJD_SPRITE_ALPHA | NJD_SPRITE_COLOR);
    njPopMatrix(1);

    dir.x = -1.0F;
    dir.y = 0.0F;
    dir.z = -0.1F;
    unit(&dir);
    njPushMatrix(&_unit_matrix_);
    TURN_VIA(njRotateY, work->ang.y, ang);
    njCalcVector(NULL, &dir, &normal);
    njPopMatrix(1);
    facing = -njInnerProduct(&normal, &view);
    if (facing < 0.0F) {
        bright = 0.0F;
    } else {
        facing = (facing - 0.2F) * 1.25F;
        bright = (facing * facing * facing) + 0.1F;
    }
    draw_set_material(bright, 1.0F, 1.0F, 0.95F);
    size = bright * HEADLIGHT_SIZE;
    njPushMatrix(matrix);
    njTranslate(NULL, headlight_minus_z.x, headlight_minus_z.y,
                headlight_minus_z.z);
    njCalcPoint(NULL, &origin, &lamp_minus_z);
    njPopMatrix(1);
    dir.x = camera_work->pos.x - lamp_minus_z.x;
    dir.y = camera_work->pos.y - lamp_minus_z.y;
    dir.z = camera_work->pos.z - lamp_minus_z.z;
    unit(&dir);
    pos.x = lamp_minus_z.x + (dir.x * HEADLIGHT_NEAR);
    pos.y = lamp_minus_z.y + (dir.y * HEADLIGHT_NEAR);
    pos.z = lamp_minus_z.z + (dir.z * HEADLIGHT_NEAR);
    njPushMatrix(NULL);
    njTranslate(NULL, pos.x, pos.y, pos.z);
    TURN_VIA(njRotateY, camera_work->ang.y, ang);
    njScale(NULL, size, size, size);
    njDrawSprite3D(&car_sprite, GLOW, NJD_SPRITE_ALPHA | NJD_SPRITE_COLOR);
    njPopMatrix(1);
    draw_set_material(1.0F, 1.0F, 1.0F, 1.0F);
    njColorBlendingMode(NJD_SOURCE_COLOR, NJD_COLOR_BLENDING_SRCALPHA);
    njColorBlendingMode(NJD_DESTINATION_COLOR, NJD_COLOR_BLENDING_INVSRCALPHA);
}

/* draw the light bar's lamps' glows, each the brighter the more the lamp
 * faces the camera, front or back. matrix is the car's matrix */
static void patrol_car_draw_beacons(struct task_work *work, NJS_MATRIX *matrix)
{
    NJS_POINT3 origin;
    NJS_POINT3 pos;
    NJS_POINT3 lamp;
    NJS_VECTOR dir;
    NJS_VECTOR normal;
    NJS_VECTOR view;
    Float facing;
    Float bright;
    Float size;
    Angle ang;
    int i;

    njColorBlendingMode(NJD_SOURCE_COLOR, NJD_COLOR_BLENDING_INVSRCCOLOR);
    njColorBlendingMode(NJD_DESTINATION_COLOR, NJD_COLOR_BLENDING_ONE);
    njSetTexture(&chaos0_effect_texlist);
    view.x = work->pos.x - camera_work->pos.x;
    view.y = work->pos.y - camera_work->pos.y;
    view.z = work->pos.z - camera_work->pos.z;
    njUnitVector(&view);
    for (i = 0; i < BEACONS; ++i) {
        dir.x = 1.0F;
        dir.y = 0.0F;
        dir.z = 0.0F;
        njPushMatrix(matrix);
        TURN_VIA(njRotateY, BEACON(work) + beacon_angles[i], ang);
        njCalcVector(NULL, &dir, &normal);
        njPopMatrix(1);
        facing = -njInnerProduct(&normal, &view);
        if (facing < 0.0F) {
            facing *= -1.0F;
        }
        bright = (facing * facing * facing) + 0.3F;
        size = bright * BEACON_SIZE;
        origin.x = origin.y = origin.z = 0.0F;
        njPushMatrix(&_unit_matrix_);
        njTranslate(NULL, work->pos.x, work->pos.y, work->pos.z);
        TURN_VIA(njRotateY, work->ang.y, ang);
        njTranslateV(NULL, &beacon_lamps[i]);
        njCalcPoint(NULL, &origin, &lamp);
        njPopMatrix(1);
        dir.x = camera_work->pos.x - lamp.x;
        dir.y = camera_work->pos.y - lamp.y;
        dir.z = camera_work->pos.z - lamp.z;
        njUnitVector(&dir);
        pos.x = lamp.x + (dir.x * BEACON_NEAR);
        pos.y = lamp.y + (dir.y * BEACON_NEAR);
        pos.z = lamp.z + (dir.z * BEACON_NEAR);
        if (i < 2) {
            draw_set_material(bright, 1.0F, 0.4F, 0.3F);
        } else {
            draw_set_material(bright, 0.4F, 0.4F, 1.0F);
        }
        njPushMatrix(NULL);
        njTranslateV(NULL, &pos);
        TURN_VIA(njRotateY, camera_work->ang.y, ang);
        TURN_VIA(njRotateX, camera_work->ang.x, ang);
        njScale(NULL, size, size, size);
        njDrawSprite3D(&car_sprite, GLOW, NJD_SPRITE_ALPHA | NJD_SPRITE_COLOR);
        njPopMatrix(1);
    }
}

static void patrol_car_draw(struct task *task)
{
    struct task_work *work = task->work;
    Angle ang;

    /* the car isn't drawn in CAMERA_MODE_CHAOS_P, the camera Chaos 0 takes
     * high above player 0. the boss hides the land's low entries in that
     * camera too */
    if (chaos_record->camera_mode == CAMERA_MODE_CHAOS_P) {
        return;
    }
    texture_set_stage();
    njUnitMatrix(&car_matrix);
    njPushMatrix(NULL);
    njTranslate(&car_matrix, work->pos.x, work->pos.y, work->pos.z);
    TURN_MATRIX_VIA(njRotateY, &car_matrix, work->ang.y, ang);
    njMultiMatrix(NULL, &car_matrix);
    draw_action(chaos0_patrol_car_action, FRAME(work));
    njColorBlendingMode(NJD_SOURCE_COLOR, NJD_COLOR_BLENDING_INVSRCCOLOR);
    njColorBlendingMode(NJD_DESTINATION_COLOR, NJD_COLOR_BLENDING_ONE);
    njSetTexture(&chaos0_effect_texlist);
    draw_set_material(0.5F, 1.0F, 1.0F, 1.0F);
    /* the headlights' pools, laid on the road ahead of the car */
    njPushMatrix(NULL);
    njTranslate(NULL, -53.0F, 0.02F, -10.3F);
    njRotateY(NULL, 0x3b00);
    njRotateX(NULL, LIE_FLAT);
    njScale(NULL, 1.3F, 2.0F, 1.0F);
    njDrawSprite3D(&car_sprite, POOL, NJD_SPRITE_ALPHA | NJD_SPRITE_COLOR);
    njPopMatrix(1);
    njPushMatrix(NULL);
    njTranslate(NULL, -53.0F, 0.01F, 10.3F);
    njRotateY(NULL, 0x4500);
    njRotateX(NULL, LIE_FLAT);
    njScale(NULL, 1.3F, 2.0F, 1.0F);
    njDrawSprite3D(&car_sprite, POOL, NJD_SPRITE_ALPHA | NJD_SPRITE_COLOR);
    njPopMatrix(1);
    njPopMatrix(1);
    patrol_car_draw_headlights(work, &car_matrix);
    patrol_car_draw_beacons(work, &car_matrix);
}

static void patrol_car_run(struct task *task)
{
    struct task_work *work = task->work;
    struct task_force *force = task->force;
    NJS_OBJECT *solid = VARS(work)->solid;

    force->speed.x = 0.0F;
    force->speed.y = 0.0F;
    force->speed.z = 0.0F;
    if (player_in_range(&work->pos, solid->model->r + TEST_MARGIN)) {
        work->flags |= WORLD_TESTED;
    } else {
        work->flags &= ~WORLD_TESTED;
    }
    FRAME(work) += CAR_PACE;
    if (FRAME(work) >= chaos0_patrol_car_action[0].motion->nbFrame) {
        FRAME(work) -= chaos0_patrol_car_action[0].motion->nbFrame;
    }
    BEACON(work) += BEACON_TURN;
    patrol_car_draw(task);
}

static void patrol_car_destroy(struct task *task)
{
    struct task_work *work = task->work;
    NJS_OBJECT *solid = VARS(work)->solid;

    if (solid != NULL) {
        world_remove(task, solid);
        world_object_free(solid);
    }
}
