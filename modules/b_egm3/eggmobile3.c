/* the Egg Viper's stage: its stage and sky steps, the floor's panels, and
 * the boss's tasks */
#include <ninja.h>
#include <sg_xpt.h>

#include "b_egm3/egg_viper.h"
#include "b_egm3/eggmobile3.h"
#include "b_egm3/floor.h"
#include "collision/world.h"
#include "enemy/enemy.h"
#include "event/play.h"
#include "main.h"
#include "sound/stream.h"
#include "system/task.h"

/* EV0024, where Eggman flies off in the Egg Viper and Sonic runs after him.
 * the stage plays it before the fight, unless it has played before */
#define EVENT_OPENING 36

enum eggmobile3_mode {
    EGGMOBILE3_START = 0,
    EGGMOBILE3_SETUP = 1,
    EGGMOBILE3_OPENING = 2,
    EGGMOBILE3_OPENING_WAIT = 4,
    EGGMOBILE3_OPENING_END = 5,
    EGGMOBILE3_BOSS = 6,
    EGGMOBILE3_MUSIC = 7,
    EGGMOBILE3_FIGHT = 8,
    EGGMOBILE3_END = 9 /* no code in the module sets it */
};

/* SKY_START blacks out the back, then the sky idles. SKY_RESET starts it
 * over */
enum sky_mode { SKY_START, SKY_IDLE, SKY_RESET };

#define SOLID_FLAGS (WORLD_LARGE | WORLD_MOVES | WORLD_SOLID)

/* make a black fog from 5000 to 8000 away, but leave fog off. nothing past
 * 2000 away is drawn */
static void eggmobile3_fog_init(void)
{
    njSetFogColor(0);
    njGenerateFogTable3(fog_table, -5000.0F, -8000.0F);
    njSetFogTable(fog_table);
    njFogDisable();
    njClipZ(-1.0F, -2000.0F);
}

static void egg_viper_create(struct task *stage);
static void eggmobile3_create_floor(void);

/* section HEAD puts the stage's step and its sky's at the module's start.
 * 1ST_READ.BIN's stage_steps and sky_steps reach them there by address. the
 * change of section ends eggmobile3_fog_init's code with its pool */
#pragma section HEAD
/* @hack spelling: EGGMOBILE3_END's call of stream_destroy, made through a
 * helper that SHC expands in eggmobile3_step. the call stays a JSR
 * (0x0c900092) ahead of the step's own exit. written in the step's case,
 * it becomes a tail jump and 13 of the step's 78 words differ */
static void eggmobile3_end(void)
{
    stream_destroy();
}

void eggmobile3_step(struct task *task)
{
    struct task_work *work = task->work;

    switch (work->mode) {
    case EGGMOBILE3_START:
        work->mode = EGGMOBILE3_SETUP;
        break;
    case EGGMOBILE3_SETUP:
        eggmobile3_fog_init();
        eggmobile3_create_floor();
        if (!event_was_played(EVENT_OPENING)) {
            work->mode = EGGMOBILE3_OPENING;
        } else {
            work->mode = EGGMOBILE3_OPENING_END;
        }
        break;
    case EGGMOBILE3_OPENING:
        event_load(EVENT_OPENING);
        event_start();
        work->mode = EGGMOBILE3_OPENING_WAIT;
        break;
    case EGGMOBILE3_OPENING_WAIT:
        if (!event_in_control()) {
            work->mode = EGGMOBILE3_OPENING_END;
        }
        break;
    case EGGMOBILE3_OPENING_END:
        work->mode = EGGMOBILE3_BOSS;
        break;
    case EGGMOBILE3_BOSS:
        egg_viper_create(task);
        work->mode = EGGMOBILE3_MUSIC;
        break;
    case EGGMOBILE3_MUSIC:
        stream_request_music(MUSIC_EGGMBL23);
        work->mode = EGGMOBILE3_FIGHT;
        break;
    case EGGMOBILE3_FIGHT:
        break;
    case EGGMOBILE3_END:
        eggmobile3_end();
        break;
    }
}

void eggmobile3_sky_step(struct task *task)
{
    struct task_work *work = task->work;

    switch (work->mode) {
    case SKY_START:
        njSetBackColor(0xff000000, 0xff000000, 0xff000000);
        work->mode = SKY_IDLE;
        break;
    case SKY_IDLE:
        return;
    case SKY_RESET:
        work->mode = SKY_START;
        break;
    default:
        /* @bug memory: a mode past SKY_RESET would hand heap_free the task
         * (JMP at 0x0c90011a), which is a slot of task_init's pool. it is not
         * a block heap_alloc returned. no code sets the sky's mode past
         * SKY_RESET */
        heap_free(task);
        break;
    }
    /* @hack redundant: a return where the function ends, which leaves the
     * reset's branch to the exit an empty slot (NOP at 0x0c900112); without
     * it, the reset's store fills the slot and 15 of 35 words differ */
    return; /* NOLINT(readability-redundant-control-flow) */
}
#pragma section

/* each panel's place, kind and turn about y, by row and column. each row's
 * first panel is turned half round */
static NJS_POINT3 floor_pos[FLOOR_ROWS][FLOOR_COLUMNS] = {
    {
        {-FLOOR_ROW_X, 0.0F, -420.0F},
        {-FLOOR_ROW_X, 0.0F, -300.0F},
        {-FLOOR_ROW_X, 0.0F, -180.0F},
        {-FLOOR_ROW_X, 0.0F, -60.0F},
        {-FLOOR_ROW_X, 0.0F, 60.0F},
        {-FLOOR_ROW_X, 0.0F, 180.0F},
        {-FLOOR_ROW_X, 0.0F, 300.0F},
        {-FLOOR_ROW_X, 0.0F, 420.0F},
    },
    {
        {FLOOR_ROW_X, 0.0F, -420.0F},
        {FLOOR_ROW_X, 0.0F, -300.0F},
        {FLOOR_ROW_X, 0.0F, -180.0F},
        {FLOOR_ROW_X, 0.0F, -60.0F},
        {FLOOR_ROW_X, 0.0F, 60.0F},
        {FLOOR_ROW_X, 0.0F, 180.0F},
        {FLOOR_ROW_X, 0.0F, 300.0F},
        {FLOOR_ROW_X, 0.0F, 420.0F},
    },
};
static Sint8 floor_kinds[FLOOR_ROWS][FLOOR_COLUMNS] = {
    {FLOOR_END_PANEL, FLOOR_PANEL, FLOOR_PANEL, FLOOR_PANEL, FLOOR_PANEL,
     FLOOR_PANEL, FLOOR_PANEL, FLOOR_END_PANEL},
    {FLOOR_END_PANEL, FLOOR_PANEL, FLOOR_PANEL, FLOOR_PANEL, FLOOR_PANEL,
     FLOOR_PANEL, FLOOR_PANEL, FLOOR_END_PANEL},
};
static Angle floor_ang[FLOOR_ROWS][FLOOR_COLUMNS] = {
    {0x8000, 0, 0, 0, 0, 0, 0, 0},
    {0x8000, 0, 0, 0, 0, 0, 0, 0},
};
/* each segment's kind, which egg_viper_create ORs into its number in the
 * segment's work->tick */
/* clang-format off */
static Uint8 segment_kinds[EGG_VIPER_SEGMENTS] = {
    0, 0, EGG_VIPER_JOINT,
    0, 0, EGG_VIPER_JOINT,
    0, 0, EGG_VIPER_JOINT,
    0, 0, EGG_VIPER_JOINT,
    EGG_VIPER_TAIL,
};
/* clang-format on */

NJS_POINT3 *egg_viper_segment_pos[EGG_VIPER_SEGMENTS];
struct angles *egg_viper_segment_ang[EGG_VIPER_SEGMENTS];
struct task *egg_viper_task;
struct task *egg_viper_needles[EGG_VIPER_NEEDLES];
struct task *egg_viper_segments[EGG_VIPER_SEGMENTS];

/* put a solid of model in the world, at the task's place and turn. returns
 * the object, or NULL when world_object_alloc has none to lend */
static NJS_OBJECT *add_solid(struct task *task, NJS_MODEL *model)
{
    struct task_work *work = task->work;
    NJS_OBJECT *solid;

    if ((solid = world_object_alloc()) != NULL) {
        solid->evalflags = NJD_EVAL_UNIT_SCL | NJD_EVAL_BREAK;
        solid->scl[0] = 1.0F;
        solid->scl[1] = 1.0F;
        solid->scl[2] = 1.0F;
        solid->pos[0] = work->pos.x;
        solid->pos[1] = work->pos.y;
        solid->pos[2] = work->pos.z;
        solid->ang[0] = work->ang.x;
        solid->ang[1] = work->ang.y;
        solid->ang[2] = work->ang.z;
        solid->model = model;
        solid->child = NULL;
        solid->sibling = NULL;
        world_add(SOLID_FLAGS, task, solid);
        return solid;
    }
    return NULL;
}

/* @hack redundant: stage, which egg_viper_create never reads. passing it keeps
 * the call's address in R2 in eggmobile3_step (MOV.L at 0x0c900078);
 * without it, R1 holds it and 2 of the step's 78 words differ */
static void egg_viper_create(struct task *stage)
{
    int i;
    struct task *task;
    struct task_work *work;

    for (i = 0; i < EGG_VIPER_NEEDLES; i++) {
        task = egg_viper_needles[i] =
            task_create(TASK_WORK, 1, egg_viper_needle_step);
        work = task->work;
        enemy_alloc_record(task, work);
        task->extra = heap_calloc(1, sizeof(struct egg_viper_record));
        task->work->tick = i;
    }
    for (i = EGG_VIPER_SEGMENTS - 1; i >= 0; i--) {
        egg_viper_segments[i] = task =
            task_create(TASK_WORK, 1, egg_viper_segment_step);
        work = task->work;
        enemy_alloc_record(task, work);
        task->extra = heap_calloc(1, sizeof(struct egg_viper_record));
        task->work->tick = segment_kinds[i] | i;
        egg_viper_segment_pos[i] = &task->work->pos;
        egg_viper_segment_ang[i] = &task->work->ang;
    }
    egg_viper_task = task_create(TASK_WORK, 1, egg_viper_step);
    enemy_alloc_record(egg_viper_task, egg_viper_task->work);
    egg_viper_task->extra = heap_calloc(1, sizeof(struct egg_viper_record));
    task_create(TASK_WORK, 3, egg_viper_arena_step);
    egg_viper_start_ang = 0;
    stream_reset();
}

static void eggmobile3_create_floor(void)
{
    int row;
    int column;
    struct task *task;
    struct floor_panel *panel;

    for (row = 0; row < FLOOR_ROWS; row++) {
        for (column = 0; column < FLOOR_COLUMNS; column++) {
            floor_panels[row][column] = task =
                task_create(TASK_WORK, 3, floor_panel_step);
            task->work->pos.x = floor_pos[row][column].x;
            task->work->pos.y = floor_pos[row][column].y;
            task->work->pos.z = floor_pos[row][column].z;
            task->work->ang.y = floor_ang[row][column];
            task->force = heap_calloc(TASK_FORCES, sizeof(struct task_force));
            panel = task->extra = heap_alloc(sizeof(struct floor_panel));
            /* @hack redundant: a call of add_solid that never runs. SHC
             * drops the call, but counts its use of task first, which keeps
             * task in R11 (0x0c900524) and makes the function 32 bytes
             * longer. without it, task takes R10 and 341 of 398 words
             * differ */
            if (0) {
                add_solid(task, NULL);
            }
            if (floor_kinds[row][column] == FLOOR_PANEL) {
                panel->object = floor_panel_objects[FLOOR_PANEL];
                panel->solid =
                    add_solid(task, floor_panel_solids[FLOOR_PANEL]->model);
                panel->kind = FLOOR_PANEL;
            } else {
                panel->object = floor_panel_objects[FLOOR_END_PANEL];
                panel->solid =
                    add_solid(task, floor_panel_solids[FLOOR_END_PANEL]->model);
                panel->kind = FLOOR_END_PANEL;
            }
        }
    }
    for (row = 0; row < FLOOR_ROWS; row++) {
        for (column = 0; column < FLOOR_COLUMNS; column++) {
            floor_standing[row][column] = 1;
        }
    }
}
