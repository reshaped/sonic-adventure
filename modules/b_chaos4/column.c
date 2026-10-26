/* Chaos 4's columns of water, and the wakes he leaves as he swims. a
 * column bursts up out of the swamp in two parts, the lower and then the
 * upper, which sink back in turn. a column that touches a flying Tails or a
 * gliding Knuckles brings him down. a wake is a ring that spreads out on
 * the swamp */
#include <ninja.h>
#include <sg_xpt.h>

#include "b_chaos4/column.h"
#include "b_chaos4/swamp.h"
#include "collision/collision.h"
#include "draw/draw.h"
#include "effect/splash.h"
#include "enemy/enemy.h"
#include "player/knuckles.h"
#include "player/tails.h"
#include "player/player.h"
#include "sound/sound.h"
#include "stage/stage.h"
#include "system/frame.h"
#include "system/task.h"
#include "system/util.h"
#include "vmu/vibration.h"

#define TEXT_1_LENGTH 11
#define TEXT_2_LENGTH 15
#define TEXT_3_LENGTH 15
#define TEXT_4_LENGTH 15
#define TEXT_5_LENGTH 15
#define TEXT_6_LENGTH 15
#define TEXT_7_LENGTH 6
#define TEXT_8_LENGTH 6
#include "b_chaos4/column/text.inc"

/* column_create makes no column while COLUMNS or more stand, and
 * wake_create no wake while more than WAKES spread */
#define COLUMNS 5
#define WAKES 20

/* the sound a column bursts up with, which the sound queue stops
 * COLUMN_SOUND_TIME + 1 of its updates later */
#define SE_COLUMN 0x177
#define COLUMN_SOUND_TIME 30

/* a part rises PART_RISE of its full height a step, and sinks at
 * PART_SINK of that speed. at the lower part's full height, the column's
 * shape stands twice COLUMN_HEIGHT tall */
#define PART_RISE 0.1F
#define PART_SINK 0.5F
#define COLUMN_HEIGHT 40.0F

/* a part leaves a ripple once it has sunk back in. the ripple starts at
 * RIPPLE_SIZE, grows RIPPLE_GROWTH a step, and fades past RIPPLE_FADE */
#define RIPPLE_GROWTH 0.1F
#define RIPPLE_FADE 3.0F
#define RIPPLE_SIZE 1.0F

/* a part's shade, its constant material's color, is RISE_SHADE while it
 * rises. as it sinks, the shade falls by SINK_DARKEN times the share of its
 * full height it has lost */
#define RISE_SHADE (-0.5F)
#define SINK_DARKEN 0.5F

/* a wake's growth a step, the size it ends at, and how much its shade
 * falls for each unit it has grown */
#define WAKE_GROWTH 0.05F
#define WAKE_SIZE 10.0F
#define WAKE_FADE 0.2F

/* the vibration pack's power as a column bursts up and as it brings the
 * player down, and its frequency */
#define BURST_POWER 3
#define BRING_DOWN_POWER 11
#define SHAKE_FREQ 30

#define THREE_QUARTER_TURN NJM_DEG_ANG(270)

struct column_part {
    Float height; /* a share of its full height */
    Float speed; /* how much its height changes a step */
    Uint8 mode;
};

enum column_part_mode { PART_WAIT, PART_RISE_UP, PART_SINK_BACK, PART_DONE };

/* a column's record, at its task's extra */
struct column {
    NJS_VECTOR speed; /* its move a step */
    struct column_part parts[2]; /* the lower, then the upper */
};

enum column_mode { COLUMN_START, COLUMN_LOWER, COLUMN_UPPER, COLUMN_END };

enum wake_mode { WAKE_START, WAKE_SPREAD };

static void column_step(struct task *task);
static void column_bring_down(struct task_work *work);
static void column_draw(struct task *task);
static void column_destroy(struct task *task);
static void wake_step(struct task *task);
static void wake_draw(struct task *task);
static void wake_destroy(struct task *task);

/* each part keeps a one-name list of its own, which names the column's
 * first texture. nothing sets either list: both parts draw with
 * column_texlist */
static NJS_TEXNAME column_lower_textures[1] = {{TEXT_1, 0, 0}};
static NJS_TEXLIST column_lower_texlist = {
#include "b_chaos4/column/column_lower_texlist.inc"
};

static NJS_MATERIAL column_lower_materials[1] = {
#include "b_chaos4/column/column_lower_materials.inc"
};

static Sint16 column_lower_meshes[32] = {
#include "b_chaos4/column/column_lower_meshes.inc"
};

static NJS_COLOR column_lower_uvs[32] = {
#include "b_chaos4/column/column_lower_uvs.inc"
};

static NJS_MESHSET column_lower_meshsets[1] = {
#include "b_chaos4/column/column_lower_meshsets.inc"
};

static NJS_POINT3 column_lower_points[16] = {
#include "b_chaos4/column/column_lower_points.inc"
};

static NJS_VECTOR column_lower_normals[16] = {
#include "b_chaos4/column/column_lower_normals.inc"
};

static NJS_MODEL column_lower_model = {
#include "b_chaos4/column/column_lower_model.inc"
};

static NJS_OBJECT column_lower_object = {
#include "b_chaos4/column/column_lower_object.inc"
};

static NJS_TEXNAME column_upper_textures[1] = {{TEXT_1, 0, 0}};
static NJS_TEXLIST column_upper_texlist = {
#include "b_chaos4/column/column_upper_texlist.inc"
};

static NJS_MATERIAL column_upper_materials[1] = {
#include "b_chaos4/column/column_upper_materials.inc"
};

static Sint16 column_upper_meshes[32] = {
#include "b_chaos4/column/column_upper_meshes.inc"
};

static NJS_COLOR column_upper_uvs[32] = {
#include "b_chaos4/column/column_upper_uvs.inc"
};

static NJS_MESHSET column_upper_meshsets[1] = {
#include "b_chaos4/column/column_upper_meshsets.inc"
};

static NJS_POINT3 column_upper_points[16] = {
#include "b_chaos4/column/column_upper_points.inc"
};

static NJS_VECTOR column_upper_normals[16] = {
#include "b_chaos4/column/column_upper_normals.inc"
};

static NJS_MODEL column_upper_model = {
#include "b_chaos4/column/column_upper_model.inc"
};

static NJS_OBJECT column_upper_object = {
#include "b_chaos4/column/column_upper_object.inc"
};

/* the column's five textures, which the stage loads from the
 * CHAOS4_HASHIRA pvm. both parts draw with them */
static NJS_TEXNAME column_textures[5] = {
    {TEXT_2, 0, 0}, {TEXT_3, 0, 0}, {TEXT_4, 0, 0},
    {TEXT_5, 0, 0}, {TEXT_6, 0, 0},
};
NJS_TEXLIST column_texlist = {
#include "b_chaos4/column/column_texlist.inc"
};

/* an upright cylinder standing on the surface, which the step sizes to
 * the lower part. its levels name objects as what it hurts. but its attack
 * is 0, and one shape hurts another only when its attack exceeds the
 * other's guard, so it hurts nothing. it pushes nothing, and nothing pushes
 * it */
static struct collision_shape column_shape[1] = {
    {SHAPE_CYLINDER | SHAPE_HURTABLE,
     SHAPE_HURTS_OBJECTS,
     PUSH_FIRM(PUSH_ALL),
     {0.0F, COLUMN_HEIGHT, 0.0F},
     9.0F,
     COLUMN_HEIGHT,
     0.0F,
     {0, 0, 0},
     0,
     NULL},
};

static NJS_MATERIAL wake_materials[1] = {
#include "b_chaos4/column/wake_materials.inc"
};

static Sint16 wake_meshes[32] = {
#include "b_chaos4/column/wake_meshes.inc"
};

static NJS_COLOR wake_uvs[32] = {
#include "b_chaos4/column/wake_uvs.inc"
};

static NJS_MESHSET wake_meshsets[1] = {
#include "b_chaos4/column/wake_meshsets.inc"
};

static NJS_POINT3 wake_points[18] = {
#include "b_chaos4/column/wake_points.inc"
};

static NJS_VECTOR wake_normals[18] = {
#include "b_chaos4/column/wake_normals.inc"
};

static NJS_MODEL wake_model = {
#include "b_chaos4/column/wake_model.inc"
};

static NJS_OBJECT wake_object = {
#include "b_chaos4/column/wake_object.inc"
};

/* the wake's two textures, which the stage loads from the CHAOS4_WAVE
 * pvm */
static NJS_TEXNAME wake_textures[2] = {{TEXT_7, 0, 0}, {TEXT_8, 0, 0}};
NJS_TEXLIST wake_texlist = {
#include "b_chaos4/column/wake_texlist.inc"
};

/* each part's object, the lower first */
static NJS_OBJECT *const column_objects[2] = {
    &column_lower_object,
    &column_upper_object,
};

void column_create(NJS_POINT3 *pos, NJS_VECTOR *speed)
{
    struct task *task;
    struct task_work *work;
    struct column *column;

    if (column_count() >= COLUMNS) {
        return;
    }
    /* @hack spelling: the task tested in the statement that stores it,
     * which copies it to R14 in the slot of the test's branch (BT/S at
     * 0x0c905e04); stored and then tested, 3 of 92 words differ */
    if ((task = task_create(TASK_WORK, 3, column_step)) == NULL) {
        return;
    }
    work = task->work;
    enemy_alloc_record(task, work);
    /* @hack spelling: the allocation tested in the statement that stores it,
     * which copies it to R4 in the slot of the test's branch (BF/S at
     * 0x0c905e1c); stored and then tested, 3 of 92 words differ */
    if ((column = heap_calloc(1, sizeof(struct column))) == NULL) {
        task_kill(task);
        return;
    }
    task->extra = column;
    VECTOR_COPY(&work->pos, pos);
    work->pos.y = SWAMP_Y;
    if (speed) {
        VECTOR_COPY(&column->speed, speed);
    }
    task->step = column_step;
    task->draw = column_draw;
    task->destructor = column_destroy;
    collision_init(task, column_shape, COLLISION_ENEMY, 1, 1);
    work->collision->flags &= ~COLLISION_TARGET;
}

/* the lower part rises and sinks back, and the upper rises as the lower
 * starts to sink. once both are back in, the column ends */
static void column_step(struct task *task)
{
    struct task_work *work = task->work;
    struct column *column = task->extra;
    struct column_part *part;
    int i;
    NJS_VECTOR growth;
    struct collision_shape *shape;

    VECTOR_SUM(&work->pos, &work->pos, &column->speed);
    switch (work->mode) {
    case COLUMN_START:
        for (i = 0; i < 2; i++) {
            part = &column->parts[i];
            part->height = 0.0F;
            part->speed = PART_RISE;
            part->mode = PART_WAIT;
        }
        part = &column->parts[0];
        part->mode = PART_RISE_UP;
        sound_play_at_for(SE_COLUMN, 0, 1, 0, work->pos.x, work->pos.y,
                          work->pos.z, COLUMN_SOUND_TIME);
        vibration_start(0, BURST_POWER, SHAKE_FREQ, 1);
        column_count_add();
        work->mode = COLUMN_LOWER;
    case COLUMN_LOWER:
        if (column->parts[0].mode == PART_SINK_BACK) {
            part = &column->parts[1];
            part->mode = PART_RISE_UP;
            part->speed = PART_RISE;
            work->mode = COLUMN_UPPER;
        }
        break;
    case COLUMN_UPPER:
        if (column->parts[0].mode == PART_DONE &&
            column->parts[1].mode == PART_DONE) {
            work->mode = COLUMN_END;
        }
        break;
    case COLUMN_END:
        column_count_remove();
        task_kill(task);
        break;
    }
    for (i = 0; i < 2; i++) {
        part = &column->parts[i];
        switch (part->mode) {
        case PART_WAIT:
            break;
        case PART_RISE_UP:
            /* @hack spelling: the height stepped in the test, which compares
             * the sum in FR2 and stores it in the slot of the test's branch
             * (BT/S at 0x0c90608a); stepped ahead of it, SHC copies the sum
             * to FR3 to compare it, column_step runs 2 bytes long and 84 of
             * 248 words differ */
            if ((part->height += part->speed) < 1.0F) {
                break;
            }
            part->speed *= PART_SINK;
            part->height = 1.0F;
            part->mode = PART_SINK_BACK;
            break;
        case PART_SINK_BACK:
            part->height -= part->speed;
            if (part->height <= 0.0F) {
                part->height = 0.0F;
                part->mode = PART_DONE;
                /* ripple_create reads only x and z */
                growth.x = RIPPLE_GROWTH;
                growth.z = RIPPLE_FADE;
                ripple_create(&work->pos, &growth, RIPPLE_SIZE);
            }
            break;
        case PART_DONE:
            break;
        }
    }
    /* @hack redundant: a never-run block of stores of PART_SINK and COLUMN_END,
     * whose uses keep 0.5 and 3 in registers, as the image does (0x0c90608e to
     * 0x0c9060bc); without it, the function runs 2 bytes long,
     * column_bring_down after it moves, and 69 of 248 words differ */
    if (0) {
        growth.z = PART_SINK;
        work->mode = COLUMN_END;
    }
    shape = work->collision->shapes;
    shape->center.y = column->parts[0].height * COLUMN_HEIGHT;
    shape->size_y = column->parts[0].height * COLUMN_HEIGHT;
    column_bring_down(work);
    collision_register(work);
    column_draw(task);
}

/* while player 0 flies, a touch of the column on a player brings player
 * 0 down: Tails stops flying, and Knuckles stands */
static void column_bring_down(struct task_work *work)
{
    if (chaos4_player_flying() && (work->collision->flags & COLLISION_HIT) &&
        work->collision->shapes->hit) {
        int kind = work->collision->shapes->hit->kind;

        if (kind != COLLISION_PLAYER) {
            return;
        }
        switch (get_character()) {
        case CHARACTER_TAILS:
            TAILS_FLAGS(player_work[0]) |= TAILS_STOP_FLYING;
            break;
        case CHARACTER_KNUCKLES:
            player_set_mode(0, PLAYER_MODE_RESET);
            break;
        }
        vibration_start(0, BRING_DOWN_POWER, SHAKE_FREQ, 1);
    }
}

int chaos4_player_flying(void)
{
    switch (get_character()) {
    case CHARACTER_TAILS:
        return TAILS_STATE(player_work[0]) & TAILS_FLYING;
        /* @hack redundant: a break after the return, which loads
         * knuckles_get_activity's address into R2 (0x0c9061e0); without it,
         * the address takes R3 and 2 of 27 words differ */
        break;
    case CHARACTER_KNUCKLES:
        return knuckles_get_activity(0) == KNUCKLES_GLIDING;
    }
    return 0;
}

/* on a frame's first step, draw each part that has height, scaled up by
 * it, and darker as it sinks */
static void column_draw(struct task *task)
{
    struct task_work *work = task->work;
    struct column *column = task->extra;
    struct column_part *part;
    Float shade;
    int i;

    if (frame_step == 0) {
        njSetTexture(&column_texlist);
        for (i = 0; i <= 1; i++) {
            part = &column->parts[i];
            if (part->height > 0.0F) {
                switch (part->mode) {
                case PART_WAIT:
                case PART_RISE_UP:
                    shade = RISE_SHADE;
                    break;
                case PART_SINK_BACK:
                case PART_DONE:
                    shade = RISE_SHADE - ((1.0F - part->height) * SINK_DARKEN);
                    break;
                }
                draw_set_material(0.0F, shade, shade, shade);
                njPushMatrix(NULL);
                njTranslateV(NULL, &work->pos);
                /* @hack spelling: the height read through column->parts[i], not
                 * part, which keeps work in R9 and the record in R11
                 * (0x0c906212, 0x0c906218). through part, they take R10 and
                 * R9, column_draw runs 32 bytes short, and 71 of 123 words
                 * differ */
                njScale(NULL, 1.0F, column->parts[i].height, 1.0F);
                njDrawObject(column_objects[i]);
                draw_set_material(0.0F, 0.0F, 0.0F, 0.0F);
                njPopMatrix(1);
                /* @hack redundant: a never-run store of the part's height to
                 * shade, which keeps part in a register across the calls, as
                 * the image does (0x0c906212 to 0x0c906232); without it, the
                 * registers shift and 67 of 123 words differ */
                if (0) {
                    shade = part->height;
                }
            }
        }
    }
}

static void column_destroy(struct task *task)
{
}

void wake_create(NJS_POINT3 *pos, struct angles *ang)
{
    struct task *task;
    struct task_work *work;

    if (wake_count() > WAKES) {
        return;
    }
    /* @hack spelling: the task tested in the statement that stores it,
     * which copies it to R13 in the slot of the test's branch (BT/S at
     * 0x0c906364); stored and then tested, 3 of 58 words differ */
    if ((task = task_create(TASK_WORK, 3, wake_step)) == NULL) {
        return;
    }
    work = task->work;
    enemy_alloc_record(task, work);
    VECTOR_COPY(&work->pos, pos);
    work->pos.y = SWAMP_Y;
    VECTOR_COPY(&work->ang, ang);
    task->step = wake_step;
    task->draw = wake_draw;
    task->destructor = wake_destroy;
}

/* a wake spreads by WAKE_GROWTH a step until its scale reaches WAKE_SIZE,
 * and ends */
static void wake_step(struct task *task)
{
    struct task_work *work = task->work;

    switch (work->mode) {
    case WAKE_START:
        wake_count_add();
        work->scl.x = work->scl.z = 0.0F;
        work->mode = WAKE_SPREAD;
    case WAKE_SPREAD:
        work->scl.x = work->scl.z += WAKE_GROWTH;
        if (work->scl.x >= WAKE_SIZE) {
            wake_count_remove();
            task_kill(task);
        }
        break;
    }
    wake_draw(task);
}

/* on a frame's first step, draw the wake across its heading, a quarter
 * turn off its ang.y, fading as it spreads */
static void wake_draw(struct task *task)
{
    struct task_work *work = task->work;
    Angle ang;
    Float shade;

    if (frame_step == 0) {
        njSetTexture(&wake_texlist);
        njPushMatrix(NULL);
        draw_control_save();
        draw_attr_save();
        njControl3D(NJD_CONTROL_3D_CONSTANT_ATTR |
                    NJD_CONTROL_3D_CONSTANT_MATERIAL);
        njTranslateV(NULL, &work->pos);
        TURN_VIA(njRotateY, -work->ang.y + THREE_QUARTER_TURN, ang);
        njScale(NULL, work->scl.x, 1.0F, work->scl.z);
        shade = -(work->scl.x * WAKE_FADE) + 1.0F;
        draw_set_material(0.0F, shade, shade, shade);
        draw_attr_set(0, NJD_FLAG_IGNORE_LIGHT);
        njDrawObject(&wake_object);
        draw_set_material(0.0F, 0.0F, 0.0F, 0.0F);
        draw_attr_restore();
        draw_control_restore();
        njPopMatrix(1);
    }
}

static void wake_destroy(struct task *task)
{
}
