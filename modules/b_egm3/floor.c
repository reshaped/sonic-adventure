/* the floor's panels, and the four shards each shatters into */
#include <mathf.h>
#include <ninja.h>
#include <sg_xpt.h>
#include <stdlib.h>

#include "b_egm3/egg_viper.h"
#include "b_egm3/floor.h"
#include "collision/collision.h"
#include "collision/world.h"
#include "draw/model.h"
#include "draw/tree.h"
#include "effect/dust.h"
#include "player/player.h"
#include "sound/sound.h"
#include "system/frame.h"
#include "system/task.h"
#include "system/util.h"

/* the floor's texlists. a panel of either kind draws with entry 0, and the
 * shards with FLOOR_SHARDS and FLOOR_END_SHARDS. all four hold the same
 * texlist */
extern NJS_TEXLIST *floor_panel_texlists[4];
/* the models of nodes 1 to 4 of the trees a panel and an end panel break
 * into */
extern NJS_MODEL shard1_model;
extern NJS_MODEL shard2_model;
extern NJS_MODEL shard3_model;
extern NJS_MODEL shard4_model;
extern NJS_MODEL end_shard1_model;
extern NJS_MODEL end_shard2_model;
extern NJS_MODEL end_shard3_model;
extern NJS_MODEL end_shard4_model;

/* a box 240 across and 20 high, which nothing in floor.c uses */
/* clang-format off */
static struct collision_shape dat_0c939340 = {
    SHAPE_RECTANGLE, 0, PUSH_FIRM(PUSH_ALL) | PUSH_ALL, {0.0F, 0.0F, 0.0F},
    120.0F, 10.0F, 120.0F, {0}, 0, NULL,
};
/* clang-format on */

/* the shards each panel breaks into */
#define SHARDS 4

/* by shard: its node in the tree its panel breaks into, its speed, its spin
 * each step and its model. an end panel's shards have nodes and models of
 * their own */
static Sint8 shard_nodes[SHARDS] = {1, 2, 3, 4};
static Sint8 end_shard_nodes[SHARDS] = {1, 2, 3, 4};
static NJS_VECTOR shard_speeds[SHARDS] = {
    {2.0F, 4.0F, 1.0F},
    {2.0F, 4.0F, -1.0F},
    {-2.0F, 4.0F, 1.0F},
    {-2.0F, 4.0F, -1.0F},
};
static struct angles shard_spins[SHARDS] = {
    {1024, 512, 2368},
    {-1472, 1536, 320},
    {-2048, 1024, -1152},
    {384, -448, -640},
};
static NJS_MODEL *shard_models[SHARDS] = {
    &shard1_model,
    &shard2_model,
    &shard3_model,
    &shard4_model,
};
static NJS_MODEL *end_shard_models[SHARDS] = {
    &end_shard1_model,
    &end_shard2_model,
    &end_shard3_model,
    &end_shard4_model,
};
/* the speed of the dust a falling shard leaves, which stays where it puffs */
static NJS_VECTOR shard_dust_speed = {0.0F, 0.0F, 0.0F};

enum panel_mode { PANEL_INIT, PANEL_STAND, PANEL_RISE, PANEL_BREAK };

/* what floor_panel_rise and floor_panel_break tell a panel, in its work's
 * tick. a standing panel takes it and clears it */
#define TOLD_RISE 1
#define TOLD_BREAK 2

/* the world tests a panel's solid while a player is within the solid's
 * radius plus this */
#define SOLID_REACH 16.0F

/* a panel rises RISE_SPEED a step for RISE_STEPS steps */
#define RISE_SPEED 4.0F
#define RISE_STEPS 15

/* a shard's extra. selector holds the shard's number, with SHARD_END for an
 * end panel's shard */
struct shard {
    Uint8 unk_00[8];
    NJS_VECTOR speed;
    struct angles spin; /* how far it turns each step */
    Sint32 selector;
};

#define SHARD_END 0x10
#define SHARD_NUMBER 0xf

enum shard_mode { SHARD_INIT, SHARD_FALL };

/* rand() times this runs from 0 to under 1024. a shard's spin about each
 * axis exceeds its shard_spins entry by that much, at random */
#define SPIN_SPREAD (1024 * RAND_SCALE)

/* a shard's upward speed drops by SHARD_GRAVITY each step, and the shard
 * goes once it is below FALLEN_Y. it puffs dust every fourth step */
#define SHARD_GRAVITY 0.10888889F
#define FALLEN_Y (-200.0F)

#define SE_SHATTER 0x1f6

static void floor_panel_draw(struct task *task);
static void floor_panel_destroy(struct task *task);
static void shard_step(struct task *task);
static void shard_draw(struct task *task);
static void shard_destroy(struct task *task);

Sint8 floor_standing[FLOOR_ROWS][FLOOR_COLUMNS];
struct task *floor_panels[FLOOR_ROWS][FLOOR_COLUMNS];
/* each node's matrix in the tree the last panel to shatter broke into,
 * placed where the panel stood. its shards start from them */
static NJS_MATRIX shard_matrices[16];
/* the panel floor_standing_at last found, which floor_panel_rise and
 * floor_panel_break tell */
static int found_row;
static int found_column;

void floor_panel_step(struct task *task)
{
    struct task_work *work = task->work;
    struct floor_panel *panel = task->extra;
    struct task *child;
    Angle ang;
    int i;

    switch (work->mode) {
    case PANEL_INIT:
        task->destructor = floor_panel_destroy;
        task->draw = floor_panel_draw;
        work->tick = 0;
        work->mode = PANEL_STAND;
        break;
    case PANEL_STAND:
        if (player_in_range(&work->pos, panel->solid->model->r + SOLID_REACH)) {
            work->flags |= WORLD_TESTED;
        } else {
            work->flags &= ~WORLD_TESTED;
        }
        if (work->tick == TOLD_RISE) {
            work->tick = 0;
            panel->rise = RISE_SPEED;
            work->timer = 0;
            work->mode = PANEL_RISE;
        }
        if (work->tick == TOLD_BREAK) {
            work->tick = 0;
            work->mode = PANEL_BREAK;
        }
        break;
    case PANEL_RISE:
        work->pos.y += panel->rise;
        if (++TASK_TIMER(work) >= RISE_STEPS) {
            work->mode = PANEL_STAND;
        }
        break;
    case PANEL_BREAK:
        njPushMatrix(NULL);
        njUnitMatrix(NULL);
        njTranslate(NULL, work->pos.x, work->pos.y, work->pos.z);
        TURN_VIA(njRotateZ, work->ang.z, ang);
        TURN_VIA(njRotateY, work->ang.y, ang);
        TURN_VIA(njRotateX, work->ang.x, ang);
        if (panel->kind == FLOOR_PANEL) {
            egg_viper_tree_matrices(floor_panel_objects[FLOOR_SHARDS],
                                    shard_matrices);
        } else {
            egg_viper_tree_matrices(floor_panel_objects[FLOOR_END_SHARDS],
                                    shard_matrices);
        }
        njPopMatrix(1);
        for (i = 0; i < SHARDS; i++) {
            child = task_create(TASK_WORK, 3, shard_step);
            child->extra = heap_alloc(sizeof(struct shard));
            ((struct shard *)child->extra)->selector = (panel->kind << 4) | i;
        }
        sound_play(SE_SHATTER, 0, 0, 0);
        task_kill(task);
        break;
    }
    if (task->step != task_destroy) {
        floor_panel_draw(task);
    }
}

static void floor_panel_draw(struct task *task)
{
    struct task_work *work = task->work;
    struct floor_panel *panel = task->extra;
    Angle ang;

    draw_attr_save();
    njSetTexture(floor_panel_texlists[0]);
    njPushMatrix(NULL);
    njTranslate(NULL, work->pos.x, work->pos.y, work->pos.z);
    TURN_VIA(njRotateY, work->ang.y, ang);
    if (frame_step == 0) {
        draw_tree(panel->object);
    }
    njPopMatrix(1);
    draw_attr_restore();
}

static void floor_panel_destroy(struct task *task)
{
    struct floor_panel *panel = task->extra;

    if (panel->solid != NULL) {
        world_remove(task, panel->solid);
        world_object_free(panel->solid);
        panel->solid = NULL;
    }
}

/* a shard starts at its node of the tree its panel broke into, turned as
 * the node is. each part of its speed is its shard_speeds entry's, times 1
 * to 1.5 at random. it falls, turning, until it is below FALLEN_Y */
static void shard_step(struct task *task)
{
    struct shard *shard = task->extra;
    struct task_work *work = task->work;
    int number = shard->selector & SHARD_NUMBER;
    NJS_MATRIX *matrix;
    /* @hack spelling: the matrices and the node through locals, which load
     * shard_matrices' address into R5 (0x0c908cda) ahead of the node
     * tables'; indexed in one expression, 31 of 266 words differ */
    NJS_MATRIX *matrices;
    int node;

    switch (work->mode) {
    case SHARD_INIT:
        task->destructor = shard_destroy;
        task->draw = shard_draw;
        work->tick = 0;
        matrices = shard_matrices;
        /* NOLINTNEXTLINE(bugprone-signed-char-misuse) */
        node = ((shard->selector & SHARD_END) == 0 ? shard_nodes
                                                   : end_shard_nodes)[number];
        matrix = &matrices[node];
        work->pos.x = (*matrix)[12];
        work->pos.y = (*matrix)[13];
        work->pos.z = (*matrix)[14];
        work->ang.x =
            (Angle)(atan2f((*matrix)[6], (*matrix)[10]) * ANGLE_PER_RADIAN);
        work->ang.y = -(Angle)(asinf((*matrix)[2]) * ANGLE_PER_RADIAN);
        work->ang.z =
            (Angle)(atan2f((*matrix)[1], (*matrix)[0]) * ANGLE_PER_RADIAN);
        shard->speed.x = shard_speeds[number].x * RANDOM(1.0F, 1.5F);
        shard->speed.y = shard_speeds[number].y * RANDOM(1.0F, 1.5F);
        shard->speed.z = shard_speeds[number].z * RANDOM(1.0F, 1.5F);
        shard->spin.x = shard_spins[number].x + (Angle)(rand() * SPIN_SPREAD);
        shard->spin.y = shard_spins[number].y + (Angle)(rand() * SPIN_SPREAD);
        shard->spin.z = shard_spins[number].z + (Angle)(rand() * SPIN_SPREAD);
        work->timer = 0;
        work->mode++;
        break;
    case SHARD_FALL:
        /* @hack spelling: -SHARD_GRAVITY added, an FADD of -0.10888889 from
         * the pool (0x0c908e8e); with -=, an FSUB of SHARD_GRAVITY, and 1 of
         * 266 words differ */
        shard->speed.y += -SHARD_GRAVITY;
        VECTOR_ADD(&work->pos, &shard->speed);
        work->ang.x = (work->ang.x + shard->spin.x) & 0xffff;
        work->ang.y = (work->ang.y + shard->spin.y) & 0xffff;
        work->ang.z = (work->ang.z + shard->spin.z) & 0xffff;
        work->timer++;
        if ((TASK_TIMER(work) & 3) == 0) {
            dust_create(&work->pos, &shard_dust_speed, 10.0F);
        }
        if (work->pos.y < FALLEN_Y) {
            work->mode++;
        }
        break;
    default:
        task_kill(task);
        break;
    }
    if (task->step != task_destroy) {
        shard_draw(task);
    }
}

static void shard_draw(struct task *task)
{
    struct task_work *work = task->work;
    struct shard *shard = task->extra;
    Angle ang;

    draw_attr_save();
    njPushMatrix(NULL);
    njTranslate(NULL, work->pos.x, work->pos.y, work->pos.z);
    TURN_VIA(njRotateZ, work->ang.z, ang);
    TURN_VIA(njRotateY, work->ang.y, ang);
    TURN_VIA(njRotateX, work->ang.x, ang);
    if ((shard->selector & SHARD_END) == 0) {
        njSetTexture(floor_panel_texlists[FLOOR_SHARDS]);
        if (frame_step == 0) {
            draw_model(shard_models[shard->selector & SHARD_NUMBER]);
        }
    } else {
        njSetTexture(floor_panel_texlists[FLOOR_END_SHARDS]);
        if (frame_step == 0) {
            draw_model(end_shard_models[shard->selector & SHARD_NUMBER]);
        }
    }
    njPopMatrix(1);
    draw_attr_restore();
}

static void shard_destroy(struct task *task)
{
}

int floor_standing_at(Float x, Float z)
{
    /* @bug logic: the column truncates toward 0 (FTRC at 0x0c90906e),
     * so a z less than FLOOR_PANEL_LENGTH below -FLOOR_ROW_END_Z, off the
     * floor's low end, reads as column 0 */
    int column = (int)((z + FLOOR_ROW_END_Z) * (1.0F / FLOOR_PANEL_LENGTH));

    if (column < 0 || column >= FLOOR_COLUMNS) {
        return 0;
    }
    if (x > -320.0F && x < -200.0F) {
        found_row = 0;
        found_column = column;
        return floor_standing[0][column];
    }
    if (x > 200.0F && x < 320.0F) {
        found_row = 1;
        found_column = column;
        return floor_standing[1][column];
    }
    return 0;
}

Float floor_row_middle(Float x)
{
    /* low climbs a panel at a time from 236 panels below the floor's low
     * end. it reaches the floor at -FLOOR_ROW_END_Z, and stops at the low end
     * of the lowest standing panel */
    Float low = -28800.0F;
    Float high;

    while (floor_standing_at(x, low) == 0) {
        low += FLOOR_PANEL_LENGTH;
        if (low > FLOOR_ROW_END_Z) {
            return FLOOR_NONE_STANDING;
        }
    }
    high = FLOOR_LAST_PANEL_Z;
    while (floor_standing_at(x, high) == 0) {
        high -= FLOOR_PANEL_LENGTH;
        if (high < -FLOOR_ROW_END_Z) {
            return FLOOR_NONE_STANDING;
        }
    }
    return (low + high) * 0.5F;
}

void floor_panel_rise(Float x, Float z)
{
    if (floor_standing_at(x, z)) {
        floor_panels[found_row][found_column]->work->tick = TOLD_RISE;
    }
}

void floor_panel_break(Float x, Float z)
{
    if (floor_standing_at(x, z)) {
        floor_panels[found_row][found_column]->work->tick = TOLD_BREAK;
        floor_standing[found_row][found_column] = 0;
    }
}
