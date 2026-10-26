/* O TUTUA, O TUTUB and O TUTUC tubes, which turn, carrying player 0 round */
#include <mathf.h>
#include <ninja.h>
#include <sg_xpt.h>

#include "collision/collision.h"
#include "collision/world.h"
#include "debug/editor.h"
#include "draw/model.h"
#include "draw/tree.h"
#include "object/placement.h"
#include "player/knuckles.h"
#include "player/player.h"
#include "stg09/casino.h"
#include "stg09/gear.h"
#include "stg09/helpers.h"
#include "stg09/tube.h"
#include "system/task.h"
#include "system/util.h"

/* each object is a base, with the tube that turns as its child */
extern NJS_OBJECT tube_a_object;
extern NJS_OBJECT tube_b_object;
extern NJS_OBJECT tube_c_object;

/* the tubes are destroyed beyond about 250 units of the camera and player 0.
 * RANGE is that distance squared */
#define RANGE 62510.0F

/* the radius tube_world_add and tube_world_test_near take for a tube's copy
 * in the collision world. each adds NEAR_MARGIN to it */
#define REACH 200.0F
#define NEAR_MARGIN 16.0F

/* the flags a tube's copy takes in the collision world, besides WORLD_SOLID
 * and the size tube_world_add adds */
#define COPY_FLAGS WORLD_GRIP_HIGH

/* while player 0 is more than TUBE_A_TOP above O TUTUA, its run clears
 * WORLD_TESTED. the collision world then doesn't test the tube's copy */
#define TUBE_A_TOP 140.0F

/* in work->flags: Knuckles is climbing the tube */
#define TUBE_CLIMBED 0x8000

struct tube_vars {
    NJS_OBJECT *copy; /* the tube's copy in the collision world, or NULL */
    Uint8 unk_04[4];
    Sint16 base; /* the base's y angle */
    Sint16 climb; /* a climbing Knuckles' angle around the tube */
};

#define VARS(work) ((struct tube_vars *)&(work)->vars)

/* the distance from the tube's axis at which Knuckles climbs it */
static Float tube_climb_radius = 20.0F;
/* of the three tubes, only O TUTUA takes a collision shape */
static struct collision_shape tube_a_shape = {
    SHAPE_CYLINDER,
    0,
    PUSH_ALL | PUSH_FIRM(PUSH_ALL),
    {0.0F, 172.5F, 0.0F},
    15.0F,
    18.5F,
    0.0F,
    {0},
    0,
    NULL,
};

/* put a copy of object in the collision world, as helpers.h's
 * casino_world_add does */
static void tube_world_add(struct task *task, NJS_OBJECT *object, Float radius,
                           Sint32 flags)
{
    struct task_work *work = task->work;
    NJS_OBJECT *copy;
    Float reach;

    VARS(work)->copy = copy = world_object_alloc();
    copy->evalflags = 0;
    copy->scl[0] = 1.0F;
    copy->scl[1] = 1.0F;
    copy->scl[2] = 1.0F;
    copy->ang[0] = 0;
    copy->ang[1] = work->ang.y;
    copy->ang[2] = 0;
    copy->pos[0] = work->pos.x;
    copy->pos[1] = work->pos.y;
    copy->pos[2] = work->pos.z;
    copy->model = object->model;
    copy->child = object->child;
    copy->sibling = NULL;
    work->flags &= ~WORLD_TESTED;
    flags |= WORLD_SOLID;
    reach = radius + NEAR_MARGIN;
    if (reach <= WORLD_SMALL_REACH) {
        flags |= WORLD_SMALL;
    } else if (reach <= WORLD_MEDIUM_REACH) {
        flags |= WORLD_MEDIUM;
    } else if (reach <= WORLD_LARGE_REACH) {
        flags |= WORLD_LARGE;
    }
    world_add(flags, task, copy);
}

/* take the copy out of the collision world and give it back to the pool.
 * unlike helpers.h's casino_world_remove, this clears the pointer */
static void tube_world_remove(struct task *task)
{
    struct task_work *work = task->work;

    if (VARS(work)->copy) {
        world_remove(task, VARS(work)->copy);
        world_object_free(VARS(work)->copy);
        VARS(work)->copy = NULL;
    }
}

/* move the copy to the work's position and set WORLD_TESTED or clear it, as
 * helpers.h's casino_world_test_near does */
static void tube_world_test_near(struct task *task, Float radius)
{
    struct task_work *work = task->work;
    NJS_OBJECT *copy = VARS(work)->copy;
    /* @hack redundant: an NJS_POINT3 never used, whose 12 bytes of frame give
     * the image's ADD #-28,R15 (0x0c90a90a); without it, 2 of 79 words differ
     */
    NJS_POINT3 unused;
    NJS_POINT3 center;
    Angle ang;

    if (copy) {
        center = copy->model->center;
        ang = work->ang.y;
        njPushMatrix(&_unit_matrix_);
        njTranslateV(NULL, &work->pos);
        TURN(njRotateY, ang);
        njCalcPoint(NULL, &center, &center);
        njPopMatrix(1);
        /* @hack type: copy->pos, a Float[3], stored in place as an
         * NJS_POINT3; without it, as three element stores, 33 of 79 words
         * differ */
        *(NJS_POINT3 *)copy->pos = work->pos;
        if (player_in_range(&center, radius + NEAR_MARGIN)) {
            work->flags |= WORLD_TESTED;
        } else {
            work->flags &= ~WORLD_TESTED;
        }
    } else {
        work->flags &= ~WORLD_TESTED;
    }
}

/* turn the tube by turn, and carry player 0 with it. Knuckles, climbing the
 * tube, is held at radius from its axis as the tube turns. otherwise the task's
 * force takes player 0 round while he is on land.
 *
 * copy is the tube's copy from the step before, which the caller has already
 * freed. player 0's record may still name it as his ground, so tube_turn
 * compares only its address */
static void tube_turn(struct task *task, Angle turn, Float radius,
                      NJS_OBJECT *copy)
{
    struct task_work *work = task->work;
    struct task_work *player = player_work[0];
    struct player *record;
    struct task_force *force;
    NJS_VECTOR offset;
    NJS_VECTOR before;
    Float distance;
    Angle ang;
    Float *out;
    Float *player_pos;
    Float *tube_pos;

    if (player && knuckles_get_activity(0) == KNUCKLES_CLIMBING) {
        record = player_get(0);
        if (record && record->ground_object == copy) {
            /* @hack spelling: offset formed through Float cursors from
             * VECTOR_FLOATS; as byte cursors, 50 of 286 words differ, and
             * through the members, 168 */
            out = VECTOR_FLOATS(&offset);
            player_pos = VECTOR_FLOATS(&player->pos);
            tube_pos = VECTOR_FLOATS(&work->pos);
            *out++ = *player_pos++ - *tube_pos++;
            *out++ = *player_pos++ - *tube_pos++;
            *out = *player_pos - *tube_pos;
            distance = njSqrt((offset.x * offset.x) + (offset.z * offset.z));
            if (work->flags & TUBE_CLIMBED) {
                offset.x = 0.0F;
                offset.z = radius;
                VARS(work)->climb += turn;
                njPushMatrix(&_unit_matrix_);
                TURN_VIA(njRotateY, VARS(work)->climb, ang);
                njCalcVector(NULL, &offset, &offset);
                njPopMatrix(1);
                /* @hack spelling: the tube's pos added into offset a Float at a
                 * time through byte cursors; through the members, the function
                 * runs 32 bytes long and 206 of 286 words differ */
                {
                    char *into = (char *)&offset;
                    char *from = (char *)&work->pos;

                    /* @hack redundant: a step of from that never runs, which
                     * keeps the image's order of the additions and of their
                     * cursors in R4 and R5 (0x0c90ab04 to 0x0c90ab0c); without
                     * it, the cursors trade and 30 of 286 words differ */
                    if (0) {
                        from += sizeof(Float);
                    }
                    *(Float *)into += *(Float *)from;
                    from += sizeof(Float);
                    into += sizeof(Float);
                    *(Float *)into += *(Float *)from;
                    from += sizeof(Float);
                    into += sizeof(Float);
                    *(Float *)into += *(Float *)from;
                }
                player_set_position(0, offset.x, offset.y, offset.z);
                work->ang.y += turn;
                return;
            } else if (distance > 0.1F &&
                       _builtin_fabsf(radius - distance) < 3.0F) {
                VARS(work)->climb =
                    (Angle)(atan2f(offset.z, offset.x) * ANGLE_PER_RADIAN);
                work->flags |= TUBE_CLIMBED;
                work->ang.y += turn;
                return;
            }
        }
    }
    work->flags &= ~TUBE_CLIMBED;
    force = task->force;
    force->turn.y = 0;
    if (player && (player->flags & PLAYER_ON_LAND)) {
        /* @hack spelling: offset formed the same way; as byte cursors, 97 of
         * 286 words differ, and through the members, 128 */
        out = VECTOR_FLOATS(&offset);
        player_pos = VECTOR_FLOATS(&player->pos);
        tube_pos = VECTOR_FLOATS(&work->pos);
        *out++ = *player_pos++ - *tube_pos++;
        *out++ = *player_pos++ - *tube_pos++;
        *out = *player_pos - *tube_pos;
        before = offset;
        njPushMatrix(&_unit_matrix_);
        TURN(njRotateY, turn);
        njCalcVector(NULL, &offset, &offset);
        njPopMatrix(1);
        /* @hack spelling: before taken from offset a Float at a time through
         * byte cursors; through the members, 56 of 286 words differ */
        {
            char *into = (char *)&offset;
            char *from = (char *)&before;

            *(Float *)into -= *(Float *)from;
            from += sizeof(Float);
            into += sizeof(Float);
            *(Float *)into -= *(Float *)from;
            from += sizeof(Float);
            into += sizeof(Float);
            *(Float *)into -= *(Float *)from;
        }
        force->turn.y = -turn;
    } else {
        /* @hack spelling: offset cleared through VECTOR_FLOATS; as a byte
         * cursor, 15 of 286 words differ, and through the members, 28 */
        out = VECTOR_FLOATS(&offset);
        *out++ = 0.0F;
        *out++ = 0.0F;
        *out = 0.0F;
    }
    force->speed = offset;
    work->ang.y += turn;
}

static void tube_a_init(struct task *task);
static void tube_a_destroy(struct task *task);
static void tube_a_run(struct task *task);
static void tube_a_draw(struct task *task);
static void tube_b_init(struct task *task);
static void tube_b_destroy(struct task *task);
static void tube_b_run(struct task *task);
static void tube_b_draw(struct task *task);
static void tube_c_init(struct task *task);
static void tube_c_destroy(struct task *task);
static void tube_c_run(struct task *task);
static void tube_c_draw(struct task *task);

void tube_a_step(struct task *task)
{
    /* @hack redundant: work set and never read, which keeps task in R14
     * across the range check (MOV R4,R14 at 0x0c90acc8); without it, 12 of 16
     * words differ */
    struct task_work *work = task->work;

    if (!placement_try_leave_by(task, RANGE)) {
        tube_a_init(task);
    }
}

static void tube_a_init(struct task *task)
{
    struct task_work *work = task->work;

    if (!placement_editing(task)) {
        collision_init(task, &tube_a_shape, COLLISION_OBJECT, 1, 1);
    }
    task->draw = tube_a_draw;
    task->step = tube_a_run;
    task->destructor = tube_a_destroy;
    work->timer = 0;
    work->mode = 0;
    VARS(work)->base = work->ang.y;
    work->flags &= ~TUBE_CLIMBED;
}

static void tube_a_destroy(struct task *task)
{
    tube_world_remove(task);
}

static void tube_a_run(struct task *task)
{
    struct task_work *work = task->work;
    NJS_OBJECT *copy;
    Angle turn;
    Sint16 *low;
    Float scale;
    Float speed;
    Float scaled;

    if (placement_try_leave_by(task, RANGE)) {
        return;
    }
    tube_a_draw(task);
    if (placement_editing(task)) {
        njPrintC(EDITOR_LABEL_RIGHT(EDITOR_ROW_SCL_X),
                 TURNS_RIGHT(work) ? "RotDir:Migi" : "RotDir:Hidari");
        njPrintC(EDITOR_LABEL_RIGHT(EDITOR_ROW_SCL_Y), "RotSpd:");
        njPrintD(EDITOR_NUMBER(EDITOR_ROW_SCL_Y), (int)njRoundOff(work->scl.y),
                 4);
        return;
    }
    copy = VARS(work)->copy;
    tube_world_remove(task);
    tube_world_add(task, tube_a_object.child, REACH, COPY_FLAGS);
    turn = casino_gear_turn;
    scale = casino_gear_scale;
    if (TURNS_RIGHT(work)) {
        speed = scale * njRoundOff(work->scl.y);
    } else {
        speed = -(scale * njRoundOff(work->scl.y));
    }
    /* @hack type: turn's low halfword, the only half scaled, read and written
     * through a Sint16 pointer; through util.h's union angle_halves, 125 of
     * 140 words differ */
    low = (Sint16 *)&turn;
    scaled = *low;
    scaled *= speed;
    *low = scaled;
    tube_turn(task, turn, tube_climb_radius, copy);
    tube_world_test_near(task, REACH);
    if ((work->flags & WORLD_TESTED) &&
        player_work[0]->pos.y > work->pos.y + TUBE_A_TOP) {
        work->flags &= ~WORLD_TESTED;
    }
    collision_register(work);
    work->timer++;
}

static void tube_a_draw(struct task *task)
{
    struct task_work *work = task->work;
    NJS_OBJECT *object = &tube_a_object;
    Angle ang;

    if (!placement_try_leave_by(task, RANGE)) {
        casino_set_texture(CASINO_TEXTURES_ACTS_12);
        njPushMatrix(NULL);
        njTranslateV(NULL, &work->pos);
        TURN_VIA(njRotateY, VARS(work)->base, ang);
        draw_model(object->model);
        TURN_VIA(njRotateY, work->ang.y - VARS(work)->base, ang);
        draw_tree(object->child);
        njPopMatrix(1);
    }
}

void tube_b_step(struct task *task)
{
    /* @hack redundant: work set and never read, which keeps task in R14
     * across the range check (MOV R4,R14 at 0x0c90afbe); without it, 11 of 16
     * words differ */
    struct task_work *work = task->work;

    if (!placement_try_leave_by(task, RANGE)) {
        tube_b_init(task);
    }
}

static void tube_b_init(struct task *task)
{
    struct task_work *work = task->work;

    task->draw = tube_b_draw;
    task->step = tube_b_run;
    task->destructor = tube_b_destroy;
    work->timer = 0;
    work->mode = 0;
    VARS(work)->base = work->ang.y;
    work->flags &= ~TUBE_CLIMBED;
}

static void tube_b_destroy(struct task *task)
{
    tube_world_remove(task);
}

static void tube_b_run(struct task *task)
{
    struct task_work *work = task->work;
    NJS_OBJECT *copy;
    Angle turn;
    Sint16 *low;
    Float scale;
    Float speed;
    Float scaled;

    if (placement_try_leave_by(task, RANGE)) {
        return;
    }
    tube_b_draw(task);
    if (placement_editing(task)) {
        njPrintC(EDITOR_LABEL_RIGHT(EDITOR_ROW_SCL_X),
                 TURNS_RIGHT(work) ? "RotDir:Migi" : "RotDir:Hidari");
        njPrintC(EDITOR_LABEL_RIGHT(EDITOR_ROW_SCL_Y), "RotSpd:");
        njPrintD(EDITOR_NUMBER(EDITOR_ROW_SCL_Y), (int)njRoundOff(work->scl.y),
                 4);
        return;
    }
    copy = VARS(work)->copy;
    tube_world_remove(task);
    tube_world_add(task, tube_b_object.child, REACH, COPY_FLAGS);
    turn = casino_gear_turn;
    scale = casino_gear_scale;
    if (TURNS_RIGHT(work)) {
        speed = scale * njRoundOff(work->scl.y);
    } else {
        speed = -(scale * njRoundOff(work->scl.y));
    }
    /* @hack type: turn's low halfword, the only half scaled, read and written
     * through a Sint16 pointer; through util.h's union angle_halves, 84 of
     * 119 words differ */
    low = (Sint16 *)&turn;
    scaled = *low;
    scaled *= speed;
    *low = scaled;
    tube_turn(task, turn, tube_climb_radius, copy);
    tube_world_test_near(task, REACH);
    work->timer++;
}

static void tube_b_draw(struct task *task)
{
    struct task_work *work = task->work;
    NJS_OBJECT *object = &tube_b_object;
    Angle ang;

    if (!placement_try_leave_by(task, RANGE)) {
        casino_set_texture(CASINO_TEXTURES_ACTS_12);
        njPushMatrix(NULL);
        njTranslateV(NULL, &work->pos);
        TURN_VIA(njRotateY, VARS(work)->base, ang);
        draw_model(object->model);
        TURN_VIA(njRotateY, work->ang.y - VARS(work)->base, ang);
        draw_tree(object->child);
        njPopMatrix(1);
    }
}

void tube_c_step(struct task *task)
{
    /* @hack redundant: work set and never read, which keeps task in R14
     * across the range check (MOV R4,R14 at 0x0c90b25e); without it, 11 of 16
     * words differ */
    struct task_work *work = task->work;

    if (!placement_try_leave_by(task, RANGE)) {
        tube_c_init(task);
    }
}

static void tube_c_init(struct task *task)
{
    struct task_work *work = task->work;

    task->draw = tube_c_draw;
    task->step = tube_c_run;
    task->destructor = tube_c_destroy;
    work->timer = 0;
    work->mode = 0;
    VARS(work)->base = work->ang.y;
    work->flags &= ~TUBE_CLIMBED;
}

static void tube_c_destroy(struct task *task)
{
    tube_world_remove(task);
}

static void tube_c_run(struct task *task)
{
    struct task_work *work = task->work;
    NJS_OBJECT *copy;
    Angle turn;
    Sint16 *low;
    Float scale;
    Float speed;
    Float scaled;

    if (placement_try_leave_by(task, RANGE)) {
        return;
    }
    tube_c_draw(task);
    if (placement_editing(task)) {
        njPrintC(EDITOR_LABEL_RIGHT(EDITOR_ROW_SCL_X),
                 TURNS_RIGHT(work) ? "RotDir:Migi" : "RotDir:Hidari");
        njPrintC(EDITOR_LABEL_RIGHT(EDITOR_ROW_SCL_Y), "RotSpd:");
        njPrintD(EDITOR_NUMBER(EDITOR_ROW_SCL_Y), (int)njRoundOff(work->scl.y),
                 4);
        return;
    }
    copy = VARS(work)->copy;
    tube_world_remove(task);
    tube_world_add(task, tube_c_object.child, REACH, COPY_FLAGS);
    turn = casino_gear_turn;
    scale = casino_gear_scale;
    if (TURNS_RIGHT(work)) {
        speed = scale * njRoundOff(work->scl.y);
    } else {
        speed = -(scale * njRoundOff(work->scl.y));
    }
    /* @hack type: turn's low halfword, the only half scaled, read and written
     * through a Sint16 pointer; through util.h's union angle_halves, 84 of
     * 119 words differ */
    low = (Sint16 *)&turn;
    scaled = *low;
    scaled *= speed;
    *low = scaled;
    tube_turn(task, turn, tube_climb_radius, copy);
    tube_world_test_near(task, REACH);
    work->timer++;
}

static void tube_c_draw(struct task *task)
{
    struct task_work *work = task->work;
    NJS_OBJECT *object = &tube_c_object;
    Angle ang;

    if (!placement_try_leave_by(task, RANGE)) {
        casino_set_texture(CASINO_TEXTURES_ACTS_12);
        njPushMatrix(NULL);
        njTranslateV(NULL, &work->pos);
        TURN_VIA(njRotateY, VARS(work)->base, ang);
        draw_model(object->model);
        TURN_VIA(njRotateY, work->ang.y - VARS(work)->base, ang);
        draw_tree(object->child);
        njPopMatrix(1);
    }
}
