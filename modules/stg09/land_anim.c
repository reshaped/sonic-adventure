/* O TIKEIANIM, which animates the land of acts 0 and 1 */
#include <ninja.h>
#include <sg_xpt.h>

#include "collision/world.h"
#include "debug/editor.h"
#include "draw/model.h"
#include "draw/tree.h"
#include "player/player.h"
#include "sound/sound.h"
#include "stage/stage.h"
#include "stg09/casino01_textures.h"
#include "stg09/fan.h"
#include "stg09/helpers.h"
#include "stg09/land_anim.h"
#include "stg09/mud.h"
#include "system/frame.h"
#include "system/task.h"

extern struct land_table casino_act0_land;
extern NJS_OBJECT casino_water_object;
extern NJS_MATERIAL casino_water_materials[];
extern NJS_MATERIAL casino_act0_land_137_materials[];
extern NJS_MATERIAL casino_act1_land_13_materials[];
extern NJS_MATERIAL casino_act1_land_14_materials[];
extern NJS_MATERIAL casino_act1_land_16_materials[];
extern NJS_MATERIAL casino_act1_land_17_materials[];
extern NJS_MATERIAL casino_act1_land_141_materials[];
extern NJS_MATERIAL casino_act1_land_142_materials[];
extern NJS_OBJECT casino_act0_land_0_object;
extern NJS_OBJECT casino_act0_land_1_object;
extern NJS_OBJECT casino_act0_land_2_object;
extern NJS_OBJECT casino_act0_land_4_object;
extern NJS_OBJECT casino_act0_land_5_object;
extern NJS_OBJECT casino_act0_land_6_object;
extern NJS_OBJECT casino_act0_land_8_object;
extern NJS_OBJECT casino_act0_land_9_object;
extern NJS_OBJECT casino_act0_land_10_object;
extern NJS_OBJECT casino_act0_land_22_object;
extern NJS_OBJECT casino_act0_land_25_object;
extern NJS_OBJECT casino_act0_land_26_object;
extern NJS_OBJECT casino_act0_land_29_object;
extern NJS_OBJECT casino_act0_land_38_object;
extern NJS_OBJECT casino_act0_land_39_object;
extern NJS_OBJECT casino_act0_land_40_object;
extern NJS_OBJECT casino_act0_land_41_object;
extern NJS_OBJECT casino_act0_land_42_object;
extern NJS_OBJECT casino_act0_land_43_object;
extern NJS_OBJECT casino_act0_land_44_object;
extern NJS_OBJECT casino_act0_land_46_object;
extern NJS_OBJECT casino_act0_land_47_object;
extern NJS_OBJECT casino_act0_land_49_object;
extern NJS_OBJECT casino_act0_land_51_object;
extern NJS_OBJECT casino_act0_land_52_object;
extern NJS_OBJECT casino_act0_land_53_object;
extern NJS_OBJECT casino_act0_land_57_object;
extern NJS_OBJECT casino_act0_land_58_object;
extern NJS_OBJECT casino_act0_land_59_object;
extern NJS_OBJECT casino_act0_land_60_object;
extern NJS_OBJECT casino_act0_land_76_object;
extern NJS_OBJECT casino_act0_land_80_object;
extern NJS_OBJECT casino_act0_land_84_object;
extern NJS_OBJECT casino_act0_land_98_object;
extern NJS_OBJECT casino_act0_land_100_object;
extern NJS_OBJECT casino_act0_land_103_object;
extern NJS_OBJECT casino_act0_land_104_object;
extern NJS_OBJECT casino_act0_land_105_object;
extern NJS_OBJECT casino_act0_land_108_object;
extern NJS_OBJECT casino_act0_land_110_object;
extern NJS_OBJECT casino_act0_land_111_object;
extern NJS_OBJECT casino_act0_land_112_object;
extern NJS_OBJECT casino_act0_land_123_object;
extern NJS_OBJECT casino_act0_land_124_object;
extern NJS_OBJECT casino_act0_land_125_object;
extern NJS_OBJECT casino_act0_land_126_object;
extern NJS_OBJECT casino_act0_land_127_object;
extern NJS_OBJECT casino_act0_land_128_object;
extern NJS_OBJECT casino_act0_land_132_object;
extern NJS_OBJECT casino_act0_land_133_object;
extern NJS_OBJECT casino_act0_land_134_object;
extern NJS_OBJECT casino_act0_land_135_object;
extern NJS_OBJECT casino_act0_land_136_object;
extern NJS_OBJECT casino_act0_land_138_object;
extern NJS_OBJECT casino_act0_land_139_object;
extern NJS_OBJECT casino_act0_land_140_object;
extern NJS_OBJECT casino_act0_land_141_object;
extern NJS_OBJECT casino_act0_land_142_object;
extern NJS_OBJECT casino_act0_land_143_object;
extern NJS_OBJECT casino_act0_land_144_object;
extern NJS_OBJECT casino_act0_land_145_object;
extern NJS_OBJECT casino_act0_land_147_object;
extern NJS_OBJECT casino_act0_land_213_object;
extern NJS_OBJECT casino_act0_land_214_object;
extern NJS_OBJECT casino_act0_land_215_object;
extern NJS_OBJECT casino_act0_land_216_object;
extern NJS_OBJECT casino_act0_land_217_object;
extern NJS_OBJECT casino_act0_land_218_object;
extern NJS_OBJECT casino_act0_land_219_object;
extern NJS_OBJECT casino_act0_land_220_object;
extern NJS_OBJECT casino_act0_land_221_object;
extern NJS_OBJECT casino_act0_land_222_object;
extern NJS_OBJECT casino_act0_land_223_object;
extern NJS_OBJECT casino_act0_land_224_object;
extern NJS_OBJECT casino_act0_land_225_object;
extern NJS_OBJECT casino_act0_land_226_object;
extern NJS_OBJECT casino_act0_land_227_object;
extern NJS_OBJECT casino_act0_land_228_object;
extern NJS_OBJECT casino_act0_land_229_object;
extern NJS_OBJECT casino_act0_land_230_object;
extern NJS_OBJECT casino_act0_land_231_object;
extern NJS_OBJECT casino_act0_land_232_object;
extern NJS_OBJECT casino_act0_land_233_object;
extern NJS_OBJECT casino_act0_land_234_object;

#define SE_WATER 975
#define WATER_TOP (-17.5F) /* act 0's water surface */
/* how far past the water model's sphere land_anim_draw_water looks for a
 * player */
#define NEAR_MARGIN 16.0F

/* the rows of each table of swaps */
#define LAMPS 4
#define ACT1_ROWS 28
#define WATER_ROWS 42

#define SOLID_OBJECTS 85 /* act 0's, made solid while player 0 isn't Sonic */
/* the objects aren't solid while player 0 is below SOLID_FLOOR, or inside an
 * upright cylinder about the spot. the cylinder reaches SOLID_SPOT_HEIGHT
 * above and below the spot */
#define SOLID_SPOT_X 32.83F
#define SOLID_SPOT_Y 24.6F
#define SOLID_SPOT_Z (-78.76F)
#define SOLID_SPOT_RADIUS 160.0F
#define SOLID_SPOT_HEIGHT 260.0F
#define SOLID_FLOOR (-110.0F)

/* a texture animation that land_anim_play shows. frames is a list of values
 * that ends in -1. each value names count rows of swaps, the rows from
 * rows + value * count */
struct texture_anim {
    int period; /* the calls of land_anim_play each value lasts */
    int count;
    struct texture_swap *rows;
    Sint8 *frames;
    Sint8 frame; /* the index of the value in use */
};

struct land_anim_vars {
    NJS_OBJECT *water; /* casino_water_object's copy in the collision world */
    /* the stage and act the task started in. the low byte is the act */
    Sint16 stage_act;
};

#define VARS(work) ((struct land_anim_vars *)&(work)->vars)

/* added to player 0's mud level each frame player 0 is under water in act 1 */
static Float land_anim_mud_rise = 0.0007F;
/* the lamps of act 1's fans 1 and 2, two for each. a fan's lamps take these
 * textures while the fan is idle */
static struct texture_swap land_anim_lamps[LAMPS] = {
    {casino_act1_land_13_materials, 57, 0},
    {&casino_act1_land_16_materials[3], 57, 0},
    {casino_act1_land_14_materials, 56, 0},
    {&casino_act1_land_17_materials[3], 56, 0},
};
static struct texture_swap land_anim_act1_rows[ACT1_ROWS] = {
#include "stg09/land_anim/land_anim_act1_rows.inc"
};
static Sint8 land_anim_act1_frames[15] = {
#include "stg09/land_anim/land_anim_act1_frames.inc"
};
static struct texture_anim land_anim_act1 = {
#include "stg09/land_anim/land_anim_act1.inc"
};
static struct texture_swap land_anim_water_rows[WATER_ROWS] = {
#include "stg09/land_anim/land_anim_water_rows.inc"
};
static Sint8 land_anim_water_frames[15] = {
#include "stg09/land_anim/land_anim_water_frames.inc"
};
static struct texture_anim land_anim_water = {
#include "stg09/land_anim/land_anim_water.inc"
};
/* the tasks running, 0 or 1. a task waits in land_anim_step while one runs,
 * or while the set editor is open */
static int land_anim_count = 0;
static NJS_OBJECT *land_anim_solid_objects[SOLID_OBJECTS + 1] = {
    &casino_act0_land_136_object, &casino_act0_land_2_object,
    &casino_act0_land_4_object,   &casino_act0_land_5_object,
    &casino_act0_land_6_object,   &casino_act0_land_0_object,
    &casino_act0_land_8_object,   &casino_act0_land_9_object,
    &casino_act0_land_10_object,  &casino_act0_land_22_object,
    &casino_act0_land_25_object,  &casino_act0_land_26_object,
    &casino_act0_land_29_object,  &casino_act0_land_38_object,
    &casino_act0_land_39_object,  &casino_act0_land_40_object,
    &casino_act0_land_41_object,  &casino_act0_land_42_object,
    &casino_act0_land_43_object,  &casino_act0_land_44_object,
    &casino_act0_land_46_object,  &casino_act0_land_47_object,
    &casino_act0_land_49_object,  &casino_act0_land_51_object,
    &casino_act0_land_52_object,  &casino_act0_land_53_object,
    &casino_act0_land_57_object,  &casino_act0_land_58_object,
    &casino_act0_land_59_object,  &casino_act0_land_60_object,
    &casino_act0_land_76_object,  &casino_act0_land_80_object,
    &casino_act0_land_84_object,  &casino_act0_land_98_object,
    &casino_act0_land_100_object, &casino_act0_land_103_object,
    &casino_act0_land_104_object, &casino_act0_land_105_object,
    &casino_act0_land_108_object, &casino_act0_land_110_object,
    &casino_act0_land_111_object, &casino_act0_land_112_object,
    &casino_act0_land_213_object, &casino_act0_land_214_object,
    &casino_act0_land_215_object, &casino_act0_land_216_object,
    &casino_act0_land_217_object, &casino_act0_land_218_object,
    &casino_act0_land_219_object, &casino_act0_land_220_object,
    &casino_act0_land_221_object, &casino_act0_land_222_object,
    &casino_act0_land_223_object, &casino_act0_land_224_object,
    &casino_act0_land_225_object, &casino_act0_land_226_object,
    &casino_act0_land_227_object, &casino_act0_land_228_object,
    &casino_act0_land_229_object, &casino_act0_land_230_object,
    &casino_act0_land_231_object, &casino_act0_land_123_object,
    &casino_act0_land_124_object, &casino_act0_land_125_object,
    &casino_act0_land_126_object, &casino_act0_land_127_object,
    &casino_act0_land_128_object, &casino_act0_land_132_object,
    &casino_act0_land_133_object, &casino_act0_land_232_object,
    &casino_act0_land_233_object, &casino_act0_land_234_object,
    &casino_act0_land_134_object, &casino_act0_land_135_object,
    &casino_act0_land_1_object,   &casino_water_object,
    &casino_act0_land_138_object, &casino_act0_land_139_object,
    &casino_act0_land_140_object, &casino_act0_land_141_object,
    &casino_act0_land_142_object, &casino_act0_land_143_object,
    &casino_act0_land_144_object, &casino_act0_land_145_object,
    &casino_act0_land_147_object, NULL,
};
/* the entries land_anim_find_solid finds for land_anim_solid_objects, ending
 * in NULL */
static struct land_entry *land_anim_solid_entries[SOLID_OBJECTS + 1];
/* TRUE while land_anim_solid_entries are solid */
static Bool land_anim_solid;

static void land_anim_destroy(struct task *task);
static void land_anim_run(struct task *task);
static void land_anim_add_water(struct task *task, NJS_OBJECT *object);
static void land_anim_remove_water(struct task *task);
static void land_anim_draw_water(struct task *task);
static void land_anim_find_solid(struct task_work *work);
static void land_anim_set_solid(struct task_work *work);

void land_anim_step(struct task *task)
{
    struct task_work *work = task->work;

    if (!land_anim_count && !placement_editing(task)) {
        task->step = land_anim_run;
        task->destructor = land_anim_destroy;
        VARS(work)->stage_act = stage_get_stage_act();
        switch ((Uint8)VARS(work)->stage_act) {
        case 0:
            land_anim_find_solid(work);
            texture_swap_save(land_anim_water_rows, WATER_ROWS);
            land_anim_add_water(task, &casino_water_object);
            sound_play_held(SE_WATER, -1, 8, 0);
            break;
        case 1:
            texture_swap_save(land_anim_lamps, LAMPS);
            texture_swap_save(land_anim_act1_rows, ACT1_ROWS);
            break;
        }
        work->timer = 0;
        land_anim_count++;
    }
}

static void land_anim_destroy(struct task *task)
{
    struct task_work *work = task->work;

    switch ((Uint8)VARS(work)->stage_act) {
    case 0:
        texture_swap_restore(land_anim_water_rows, WATER_ROWS);
        land_anim_remove_water(task);
        sound_stop(SE_WATER);
        break;
    case 1:
        texture_swap_restore(land_anim_lamps, LAMPS);
        texture_swap_restore(land_anim_act1_rows, ACT1_ROWS);
        /* no break: a break moves land_anim_count's decrement from the image's
         * R2 to R3 (MOV.L @R4,R2 at 0x0c91a980). with one, 3 of
         * land_anim_destroy's 39 words differ */
    }
    land_anim_count--;
}

static void land_anim_play(struct texture_anim *anim, Sint16 timer)
{
    struct texture_swap *row;
    /* @hack spelling: i set to 0 as it is declared, which SHC does first (MOV
     * #0,R11 at 0x0c91a9d4). as for (i = 0; i < count; i++), the 0 moves to
     * the branch's slot, and 15 of land_anim_play's 59 words differ */
    int i = 0;
    int count = anim->count;
    /* NOLINTNEXTLINE(bugprone-signed-char-misuse) */
    int frame = anim->frames[anim->frame];

    row = anim->rows + (count * frame);
    while (i < count) {
        texture_swap_use(row, 1);
        /* @hack spelling: row stepped as row = row + 1, which keeps row in R13
         * (ADD #12,R13 at 0x0c91aa00). as row++, row takes R11, and 14 of
         * land_anim_play's 59 words differ */
        row = row + 1;
        i++;
    }
    if (timer % anim->period == 0) {
        if (anim->frames[++anim->frame] < 0) {
            anim->frame = 0;
        }
    }
}

static void land_anim_run(struct task *task)
{
    struct task_work *work = task->work;
    struct player *player = player_get(0);

    if (VARS(work)->stage_act != stage_get_stage_act()) {
        task_kill(task);
        return;
    }
    if (frame_step > 0) {
        return;
    }
    switch ((Uint8)VARS(work)->stage_act) {
    case 0:
        land_anim_draw_water(task);
        land_anim_play(&land_anim_water, work->timer);
        land_anim_set_solid(work);
        break;
    case 1:
        texture_swap_restore(land_anim_lamps, LAMPS);
        texture_swap_restore(land_anim_act1_rows, ACT1_ROWS);
        if (fan_idle(1)) {
            texture_swap_use(&land_anim_lamps[0], 1);
            texture_swap_use(&land_anim_lamps[1], 1);
        }
        if (fan_idle(2)) {
            texture_swap_use(&land_anim_lamps[2], 1);
            texture_swap_use(&land_anim_lamps[3], 1);
        }
        land_anim_play(&land_anim_act1, work->timer);
        if (player->shadow.last_above_flags & WORLD_WATER) {
            mud_set_level(mud_get_level() + land_anim_mud_rise);
        }
        break;
    }
    work->timer++;
}

static void land_anim_add_water(struct task *task, NJS_OBJECT *object)
{
    struct task_work *work = task->work;
    NJS_OBJECT *water = world_object_alloc();

    *water = *object;
    VARS(work)->water = water;
    work->flags &= ~WORLD_TESTED;
    world_add(LAND_DRAWN | WORLD_WATER | WORLD_SOLID, task, water);
}

static void land_anim_remove_water(struct task *task)
{
    struct task_work *work = task->work;

    if (VARS(work)->water) {
        world_remove(task, VARS(work)->water);
        world_object_free(VARS(work)->water);
        VARS(work)->water = NULL;
    }
}

/* draw the water. let the world test it only while a player is within
 * NEAR_MARGIN of the water model's sphere. both happen only while player 0 is
 * above WATER_TOP */
static void land_anim_draw_water(struct task *task)
{
    struct task_work *work = task->work;
    NJS_OBJECT *water;
    NJS_MODEL *model;
    Float radius;

    water = VARS(work)->water;
    work->flags &= ~WORLD_TESTED;
    if (water) {
        /* @hack spelling: the comparison written with WATER_TOP first, which
         * has SHC load player 0's y before it (FMOV.S @(R0,R2),FR3 at
         * 0x0c91ac20). as y > WATER_TOP, the loads trade places, and 6 of
         * land_anim_draw_water's 67 words differ */
        if (WATER_TOP < player_work[0]->pos.y) {
            if (frame_step == 0 &&
                draw_in_view(water->model->center.x, water->model->center.y,
                             water->model->center.z, water->model->r)) {
                njSetTexture(&casino01_texlist);
                draw_tree(water);
            }
            radius = water->model->r + NEAR_MARGIN;
            model = water->model;
            if (player_in_range(&model->center, radius)) {
                work->flags |= WORLD_TESTED;
            }
        }
    }
}

/* object if list holds it, or else NULL. list ends in NULL */
static NJS_OBJECT *land_anim_find(NJS_OBJECT *object, NJS_OBJECT **list)
{
    while (*list && *list != object) {
        list++;
    }
    return *list;
}

/* make solid each entry of act 0's land whose object land_anim_solid_objects
 * holds, and list it in land_anim_solid_entries. a WORLD_HUGE entry stays as
 * it is */
static void land_anim_find_solid(struct task_work *work)
{
    int count = casino_act0_land.count;
    struct land_entry *entry = casino_act0_land.entries;
    struct land_entry **out = land_anim_solid_entries;

    while (count--) {
        if ((entry->flags & WORLD_SIZE) &&
            land_anim_find(entry->object, land_anim_solid_objects)) {
            entry->flags |= WORLD_SOLID;
            *out++ = entry;
        }
        entry++;
    }
    *out = NULL;
    land_anim_solid = TRUE;
}

static void land_anim_set_solid(struct task_work *work)
{
    struct land_entry **next;
    struct land_entry *entry;
    Float x;
    Float y;
    Float z;

    if (land_anim_solid) {
        /* @hack spelling: each of the three tests below clears the entries'
         * WORLD_SOLID in a loop of its own. the plain form is one loop under
         * the three tests joined by ||, with x, y and z computed ahead of
         * them. it reads player_work[0] even for Sonic, so it is equal while
         * player_work[0] is set. as that, 148 of land_anim_set_solid's 149
         * words and 51 of 56 other words differ, and 5 of
         * land_anim_find_solid's 41 */
        if (player_get_character(0) == CHARACTER_SONIC) {
            land_anim_solid = FALSE;
            next = land_anim_solid_entries;
            while ((entry = *next++) != NULL) {
                entry->flags &= ~WORLD_SOLID;
            }
        } else if (player_work[0]->pos.y < SOLID_FLOOR) {
            land_anim_solid = FALSE;
            next = land_anim_solid_entries;
            while ((entry = *next++) != NULL) {
                entry->flags &= ~WORLD_SOLID;
            }
        } else {
            x = player_work[0]->pos.x - SOLID_SPOT_X;
            y = player_work[0]->pos.y - SOLID_SPOT_Y;
            z = player_work[0]->pos.z - SOLID_SPOT_Z;
            if (_builtin_fabsf(y) < SOLID_SPOT_HEIGHT &&
                (x * x) + (z * z) < SOLID_SPOT_RADIUS * SOLID_SPOT_RADIUS) {
                land_anim_solid = FALSE;
                next = land_anim_solid_entries;
                while ((entry = *next++) != NULL) {
                    entry->flags &= ~WORLD_SOLID;
                }
            }
        }
    } else {
        if (player_get_character(0) != CHARACTER_SONIC) {
            if (player_work[0]->pos.y > SOLID_FLOOR) {
                x = player_work[0]->pos.x - SOLID_SPOT_X;
                y = player_work[0]->pos.y - SOLID_SPOT_Y;
                z = player_work[0]->pos.z - SOLID_SPOT_Z;
                if (_builtin_fabsf(y) > SOLID_SPOT_HEIGHT ||
                    (x * x) + (z * z) > SOLID_SPOT_RADIUS * SOLID_SPOT_RADIUS) {
                    land_anim_solid = TRUE;
                    next = land_anim_solid_entries;
                    while ((entry = *next++) != NULL) {
                        entry->flags |= WORLD_SOLID;
                    }
                }
            }
        }
    }
}
