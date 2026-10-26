/* the O PIER boardwalk sections, which break apart as the chasing orca
 * reaches them */
#include <ninja.h>
#include <sg_xpt.h>
#include <stdlib.h>

#include "collision/collision.h"
#include "collision/world.h"
#include "debug/editor.h"
#include "draw/model.h"
#include "draw/tree.h"
#include "object/placement.h"
#include "player/player.h"
#include "sound/sound.h"
#include "stg01/orca_control.h"
#include "stg01/pier.h"
#include "stg01/sea.h"
#include "system/task.h"
#include "system/texture.h"
#include "system/util.h"

extern NJS_OBJECT beach_pier_deck_object;
extern NJS_OBJECT beach_pier_straight_object;
extern NJS_OBJECT beach_pier_straight_piece_1_object;
extern NJS_OBJECT beach_pier_straight_piece_2_object;
extern NJS_OBJECT beach_pier_straight_piece_3_object;
extern NJS_OBJECT beach_pier_straight_piece_4_object;
extern NJS_OBJECT beach_pier_straight_piece_5_object;
extern NJS_OBJECT beach_pier_straight_piece_6_object;
extern NJS_OBJECT beach_pier_straight_piece_7_object;
extern NJS_OBJECT beach_pier_straight_piece_8_object;
extern NJS_OBJECT beach_pier_straight_piece_9_object;
extern NJS_OBJECT beach_pier_straight_piece_10_object;
extern NJS_OBJECT beach_pier_straight_piece_11_object;
extern NJS_OBJECT beach_pier_straight_piece_12_object;
extern NJS_OBJECT beach_pier_straight_piece_13_object;
extern NJS_OBJECT beach_pier_straight_piece_14_object;
extern NJS_OBJECT beach_pier_straight_piece_15_object;
extern NJS_OBJECT beach_pier_straight_piece_16_object;
extern NJS_OBJECT beach_pier_straight_piece_17_object;
extern NJS_OBJECT beach_pier_straight_piece_18_object;
extern NJS_OBJECT beach_pier_straight_piece_19_object;
extern NJS_OBJECT beach_pier_straight_piece_20_object;
extern NJS_OBJECT beach_pier_straight_piece_21_object;
extern NJS_OBJECT beach_pier_straight_piece_22_object;
extern NJS_OBJECT beach_pier_straight_piece_23_object;
extern NJS_OBJECT beach_pier_straight_piece_24_object;
extern NJS_OBJECT beach_pier_straight_piece_25_object;
extern NJS_OBJECT beach_pier_corner_object;
extern NJS_OBJECT beach_pier_corner_piece_1_object;
extern NJS_OBJECT beach_pier_corner_piece_2_object;
extern NJS_OBJECT beach_pier_corner_piece_3_object;
extern NJS_OBJECT beach_pier_corner_piece_4_object;
extern NJS_OBJECT beach_pier_corner_piece_5_object;
extern NJS_OBJECT beach_pier_corner_piece_6_object;
extern NJS_OBJECT beach_pier_corner_piece_7_object;
extern NJS_OBJECT beach_pier_corner_piece_8_object;
extern NJS_OBJECT beach_pier_corner_piece_9_object;
extern NJS_OBJECT beach_pier_corner_piece_10_object;
extern NJS_OBJECT beach_pier_corner_piece_11_object;
extern NJS_OBJECT beach_pier_corner_piece_12_object;
extern NJS_OBJECT beach_pier_corner_piece_13_object;
extern NJS_OBJECT beach_pier_corner_piece_14_object;
extern NJS_OBJECT beach_pier_corner_piece_15_object;
extern NJS_OBJECT beach_pier_corner_piece_16_object;
extern NJS_OBJECT beach_pier_corner_piece_17_object;
extern NJS_OBJECT beach_pier_corner_piece_18_object;
extern NJS_OBJECT beach_pier_corner_piece_19_object;
extern NJS_OBJECT beach_pier_corner_piece_20_object;
extern NJS_OBJECT beach_pier_corner_piece_21_object;
extern NJS_OBJECT beach_pier_corner_piece_22_object;
extern NJS_OBJECT beach_pier_corner_piece_23_object;
extern NJS_OBJECT beach_pier_corner_piece_24_object;

/* the kinds of section: straight and corner */
#define PIER_KINDS 2
/* a broken section keeps its pieces until its timer passes PIECE_STEPS */
#define PIECE_STEPS 300

/* the section stays while the camera or player 0 is within 500 of it */
#define PIER_RANGE_SQUARED (500.0F * 500.0F)
/* the world tests the deck only while player 0 or 1 is within DECK_RANGE */
#define DECK_RANGE 50.0F
/* a section breaks once the orca's head is within 75 of it */
#define BREAK_RANGE_SQUARED (75.0F * 75.0F)

/* sound of a breaking section; after it plays, the next CRASH_WAIT sections
 * break silently */
#define SE_CRASH 42
#define CRASH_WAIT 2

/* a piece's fall acceleration, its sink speed in the sea, and the share of
 * its x and z speed it keeps each step there */
#define PIECE_GRAVITY 0.1F
#define SINK_SPEED (-0.7F)
#define PIECE_DRAG 0.93F

/* a piece stops being drawn once it has sunk for more than SINK_STEPS */
#define SINK_STEPS 120

/* a breaking section pushes its pieces SIDE_PUSH out to its side of the
 * orca's path. it pushes them AWAY_PUSH away from the orca's head */
#define SIDE_PUSH 2.9F
#define AWAY_PUSH 1.2F
/* a piece flies out at up to PIECE_PUSH times the push. it flies up at
 * RISE_MIN to RISE_MAX a step */
#define PIECE_PUSH 1.8F
#define RISE_MIN 2.5F
#define RISE_MAX 4.0F
/* a piece spins up to half SPIN_X about x and half SPIN_Y about y a step,
 * either way */
#define SPIN_X 3000.0F
#define SPIN_Y 1800.0F
/* a piece flies at once at a chance above FLY_CHANCE, two in five. the rest
 * wait WAIT_STEPS */
#define FLY_CHANCE 0.6F
#define WAIT_STEPS 5

/* player 0, thrown off a breaking section, takes THROW_SPEED along it and
 * THROW_RISE up */
#define THROW_SPEED 8.0F
#define THROW_RISE 2.0F

/* the section's kind, scl.y modulo PIER_KINDS, kept in submode */
#define KIND(work) ((work)->submode)

/* the section's deck object in the collision world */
struct pier_vars {
    NJS_OBJECT *world;
};

struct piece_vars {
    Uint8 unk_00[4];
    NJS_OBJECT *part; /* the piece's model */
    Float unk_08; /* the start clears it, and nothing reads it */
};

/* a piece's speed and spins, laid over its task's motion. a spin is the turn
 * the piece adds each step */
struct piece_motion {
    Uint8 unk_00[4];
    NJS_VECTOR speed;
    Uint8 unk_10[24];
    Angle spin_x;
    Angle spin_y;
};

#define VARS(work) ((struct pier_vars *)&(work)->vars)
#define PIECE(work) ((struct piece_vars *)&(work)->vars)
#define PIECE_MOTION(task) ((struct piece_motion *)(task)->motion)

enum pier_mode { PIER_STAND = 0, PIER_BROKEN = 1, PIER_GONE = 6 };

enum piece_mode {
    PIECE_WAIT = 2,
    PIECE_FLY = 3,
    PIECE_FALL = 4,
    PIECE_SINK = 5,
    PIECE_GONE = 6
};

static void beach_pier_step(struct task *task);
static void beach_pier_draw(struct task *task);
static void beach_pier_destroy(struct task *task);
static void beach_pier_piece_start(struct task *task);
static void beach_pier_piece_step(struct task *task);
static void beach_pier_piece_draw(struct task *task);
static void beach_pier_piece_destroy(struct task *task);

/* each kind's models: the whole section, then its pieces */
static NJS_OBJECT *beach_pier_straight_parts[26] = {
    &beach_pier_straight_object,          &beach_pier_straight_piece_1_object,
    &beach_pier_straight_piece_2_object,  &beach_pier_straight_piece_3_object,
    &beach_pier_straight_piece_4_object,  &beach_pier_straight_piece_5_object,
    &beach_pier_straight_piece_6_object,  &beach_pier_straight_piece_7_object,
    &beach_pier_straight_piece_8_object,  &beach_pier_straight_piece_9_object,
    &beach_pier_straight_piece_10_object, &beach_pier_straight_piece_11_object,
    &beach_pier_straight_piece_12_object, &beach_pier_straight_piece_13_object,
    &beach_pier_straight_piece_14_object, &beach_pier_straight_piece_15_object,
    &beach_pier_straight_piece_16_object, &beach_pier_straight_piece_17_object,
    &beach_pier_straight_piece_18_object, &beach_pier_straight_piece_19_object,
    &beach_pier_straight_piece_20_object, &beach_pier_straight_piece_21_object,
    &beach_pier_straight_piece_22_object, &beach_pier_straight_piece_23_object,
    &beach_pier_straight_piece_24_object, &beach_pier_straight_piece_25_object,
};
static NJS_OBJECT *beach_pier_corner_parts[25] = {
    &beach_pier_corner_object,          &beach_pier_corner_piece_1_object,
    &beach_pier_corner_piece_2_object,  &beach_pier_corner_piece_3_object,
    &beach_pier_corner_piece_4_object,  &beach_pier_corner_piece_5_object,
    &beach_pier_corner_piece_6_object,  &beach_pier_corner_piece_7_object,
    &beach_pier_corner_piece_8_object,  &beach_pier_corner_piece_9_object,
    &beach_pier_corner_piece_10_object, &beach_pier_corner_piece_11_object,
    &beach_pier_corner_piece_12_object, &beach_pier_corner_piece_13_object,
    &beach_pier_corner_piece_14_object, &beach_pier_corner_piece_15_object,
    &beach_pier_corner_piece_16_object, &beach_pier_corner_piece_17_object,
    &beach_pier_corner_piece_18_object, &beach_pier_corner_piece_19_object,
    &beach_pier_corner_piece_20_object, &beach_pier_corner_piece_21_object,
    &beach_pier_corner_piece_22_object, &beach_pier_corner_piece_23_object,
    &beach_pier_corner_piece_24_object,
};

static int beach_pier_part_counts[PIER_KINDS] = {26, 25};
static NJS_OBJECT **beach_pier_parts[PIER_KINDS] = {
    beach_pier_straight_parts,
    beach_pier_corner_parts,
};

/* the rails' shapes. a straight section has one along each side, and a corner
 * one along each of two sides */
/* clang-format off */
static struct collision_shape beach_pier_straight_shapes[2] = {
    {SHAPE_RECTANGLE, 0, PUSH_FIRM(PUSH_ALL) | PUSH_ALL, {23.5F, 5.5F, 0.0F},
     1.0F, 4.0F, 25.0F, {0, 0, 0}, 0, NULL},
    {SHAPE_RECTANGLE, 0, PUSH_FIRM(PUSH_ALL) | PUSH_ALL, {-23.5F, 5.5F, 0.0F},
     1.0F, 4.0F, 25.0F, {0, 0, 0}, 0, NULL},
};
static struct collision_shape beach_pier_corner_shapes[2] = {
    {SHAPE_RECTANGLE, 0, PUSH_FIRM(PUSH_ALL) | PUSH_ALL, {0.0F, 5.5F, -23.5F},
     25.0F, 4.0F, 1.0F, {0, 0, 0}, 0, NULL},
    {SHAPE_RECTANGLE, 0, PUSH_FIRM(PUSH_ALL) | PUSH_ALL, {-23.5F, 5.5F, 0.0F},
     1.0F, 4.0F, 25.0F, {0, 0, 0}, 0, NULL},
};
/* clang-format on */

static struct collision_shape *beach_pier_shapes[PIER_KINDS] = {
    beach_pier_straight_shapes,
    beach_pier_corner_shapes,
};

/* outside the set editor, put the deck in the collision world, at the
 * section's position and y turn. the deck goes in small and solid, its mesh
 * made again each step. the rails get a collision record */
void beach_pier_init(struct task *task)
{
    struct task_work *work = task->work;
    struct task_force *force = task->force;
    NJS_OBJECT *object;

    KIND(work) = (Uint8)work->scl.y % PIER_KINDS;
    work->mode = PIER_STAND;
    work->timer = 0;
    force->turn.x = 0;
    force->turn.y = 0;
    force->turn.z = 0;
    force->speed.x = 0.0F;
    force->speed.y = 0.0F;
    force->speed.z = 0.0F;
    if (!placement_editing(task)) {
        object = world_object_alloc();
        VARS(work)->world = object;
        object->evalflags = NJD_EVAL_BREAK;
        object->scl[0] = 1.0F;
        object->scl[1] = 1.0F;
        object->scl[2] = 1.0F;
        object->ang[0] = 0;
        object->ang[1] = work->ang.y;
        object->ang[2] = 0;
        object->pos[0] = work->pos.x;
        object->pos[1] = work->pos.y;
        object->pos[2] = work->pos.z;
        object->model = beach_pier_deck_object.model;
        object->child = NULL;
        object->sibling = NULL;
        world_add(WORLD_SIZE | WORLD_MOVES | WORLD_SOLID, task, object);
        work->flags |= WORLD_TESTED;
        collision_init(task, beach_pier_shapes[KIND(work)], COLLISION_OBJECT, 1,
                       2);
    }
    task->step = beach_pier_step;
    task->draw = beach_pier_draw;
    task->destructor = beach_pier_destroy;
}

/* once the orca's head comes within 75 of it, the section breaks. it stops
 * drawing and testing its deck, and creates its pieces */
static void beach_pier_step(struct task *task)
{
    struct task_work *work = task->work;
    Float *at;
    char *bytes;
    Float head_x;
    Float head_y;
    Float head_z;
    Float x;
    Float y;
    Float z;
    Float origin_x;
    Float origin_y;
    Float origin_z;
    /* @hack redundant: a vector declared and never used, which keeps the frame
     * at 40 bytes (ADD #-40,R15 at 0x0c9034f6). without it, 2 of 331 words
     * differ */
    NJS_VECTOR unused;
    /* @hack spelling: way, the orca's direction of motion, reused for the
     * direction from the head to the section. both take the frame's slot at
     * sp+16 (stores at 0x0c903618 and 0x0c903692). with a vector for each,
     * 192 of 331 words differ */
    NJS_VECTOR way;
    /* the way out from the orca's path to the side the section lies on */
    NJS_VECTOR side;
    Float dx;
    Float dy;
    Float dz;
    Float squared;
    Float cross;
    Float facing;
    Float off_x;
    NJS_OBJECT **parts;
    int count;
    int i;
    Float cos;
    Float sin;
    Angle ang;
    struct task *child;
    struct task_work *piece;
    NJS_OBJECT *part;

    if (placement_try_leave_by(task, PIER_RANGE_SQUARED)) {
        return;
    }
    if (placement_editing(task)) {
        KIND(work) = (Uint8)work->scl.y % PIER_KINDS;
        njPrintC(EDITOR_LABEL(EDITOR_ROW_SCL_Y), "-> KIND OF PIER");
        beach_pier_draw(task);
        return;
    }
    switch (work->mode) {
    case PIER_STAND:
        if (player_in_range(&work->pos, DECK_RANGE)) {
            work->flags |= WORLD_TESTED;
        } else {
            work->flags &= ~WORLD_TESTED;
        }
        /* @hack spelling: the head read a Float at a time through a byte
         * cursor, as the image reads it. through the members, 78 of 331 words
         * differ */
        bytes = (char *)&orca_head;
        head_x = *(Float *)bytes;
        bytes += sizeof(Float);
        head_y = *(Float *)bytes;
        bytes += sizeof(Float);
        head_z = *(Float *)bytes;
        /* @hack spelling: the section's pos walked as Floats through
         * VECTOR_FLOATS, which keeps the image's registers. as a byte cursor,
         * 10 of 331 words differ, and through the members, 81 */
        at = VECTOR_FLOATS(&work->pos);
        x = *at;
        at++;
        y = *at++;
        z = *at;
        dx = head_x - x;
        dy = head_y - y;
        dz = head_z - z;
        squared = (dx * dx) + (dy * dy) + (dz * dz);
        if (squared < BREAK_RANGE_SQUARED) {
            count = beach_pier_part_counts[KIND(work)];
            parts = beach_pier_parts[KIND(work)];
            way.x = orca_speed.x;
            way.y = 0.0F;
            way.z = orca_speed.z;
            off_x = x - head_x;
            /* @hack spelling: head_z, reused for the section's z offset from
             * the head, which keeps the offset in head_z's FR4 (0x0c903628 to
             * 0x0c90362c). with a local of its own, 59 of 331 words differ */
            head_z = z - head_z;
            cross = (way.z * off_x) - (way.x * head_z);
            if (cross < 0.0F) {
                side.x = -way.z;
                side.z = way.x;
            } else {
                side.x = way.z;
                side.z = -way.x;
            }
            way.x = off_x;
            way.y = 0.0F;
            way.z = head_z;
            side.y = 0.0F;
            njUnitVector(&side);
            njUnitVector(&way);
            /* the pieces read their push from scl.x and scl.z */
            work->scl.x = (side.x * SIDE_PUSH) + (way.x * AWAY_PUSH);
            work->scl.z = (side.z * SIDE_PUSH) + (way.z * AWAY_PUSH);
            work->mode = PIER_BROKEN;
            work->timer = 0;
            work->flags &= ~WORLD_TESTED;
            task->draw = NULL;
            /* player 0, if on the deck, is thrown along the section's
             * heading the way the orca moves */
            if (player_stands_on(0, task)) {
                cos = njCos(work->ang.y);
                sin = njSin(work->ang.y);
                facing = (sin * orca_speed.x) + (cos * orca_speed.z);
                if (facing < 0.0F) {
                    cos *= -1.0F;
                    sin *= -1.0F;
                }
                player_set_speed(0, sin * THROW_SPEED, THROW_RISE,
                                 cos * THROW_SPEED);
            }
            origin_x = work->pos.x;
            origin_y = work->pos.y;
            origin_z = work->pos.z;
            ang = work->ang.y;
            /* @hack redundant: a store of origin_x that never runs. it gives
             * origin_x its register first, as the image's allocation does
             * (0x0c903740 to 0x0c903786); without it, 6 of 331 words differ */
            if (0) {
                side.x = origin_x;
            }
            for (i = 1; i < count; i++) {
                child = task_create_child(TASK_WORK | TASK_MOTION,
                                          beach_pier_piece_start, task);
                piece = child->work;
                part = parts[i];
                PIECE(piece)->part = part;
                piece->ang.y = ang;
                piece->pos.x = origin_x + part->pos[0];
                piece->pos.y = origin_y + part->pos[1];
                piece->pos.z = origin_z + part->pos[2];
            }
            if (orca_crash_wait == 0) {
                sound_play(SE_CRASH, 0, 0, 0);
                orca_crash_wait = CRASH_WAIT;
            } else {
                orca_crash_wait--;
            }
        }
        collision_register(work);
        beach_pier_draw(task);
        break;
    case PIER_BROKEN:
        TASK_TIMER(work)++;
        if (TASK_TIMER(work) > PIECE_STEPS) {
            work->mode = PIER_GONE;
            task_destroy_children(task);
        } else {
            task_step_children(task);
        }
        break;
    case PIER_GONE:
        break;
    }
}

static void beach_pier_draw(struct task *task)
{
    struct task_work *work = task->work;
    Angle ang;

    texture_set_stage();
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    TURN_VIA(njRotateZ, work->ang.z, ang);
    TURN_VIA(njRotateX, work->ang.x, ang);
    TURN_VIA(njRotateY, work->ang.y, ang);
    draw_tree(beach_pier_parts[KIND(work)][0]);
    njPopMatrix(1);
}

static void beach_pier_destroy(struct task *task)
{
    struct task_work *work = task->work;

    task_destroy_children(task);
    if (VARS(work)->world != NULL) {
        world_remove(task, VARS(work)->world);
        world_object_free(VARS(work)->world);
    }
}

/* start a piece flying at a random speed and spin */
static void beach_pier_piece_start(struct task *task)
{
    struct task_work *work = task->work;
    struct piece_motion *motion = PIECE_MOTION(task);
    struct task_work *parent;
    Float chance;
    Float push_x;
    Float push_z;

    work->timer = 0;
    PIECE(work)->unk_08 = 0.0F;
    chance = rand() * RAND_SCALE;
    work->mode = PIECE_WAIT;
    /* @bug logic: the waits of 2 and 3 steps test for a chance above 0.75
     * and above 0.95 (FCMP/GT at 0x0c903960 and 0x0c90396e). a chance above
     * FLY_CHANCE has already flown at once, so neither test holds */
    if (chance > FLY_CHANCE) {
        work->timer = 0;
        work->mode = PIECE_FLY;
    } else if (chance > 0.75F) {
        work->timer = 2;
    } else if (chance > 0.95F) {
        work->timer = 3;
    } else {
        work->timer = WAIT_STEPS;
    }
    parent = task->parent->work;
    push_x = parent->scl.x;
    push_z = parent->scl.z;
    motion->speed.x = (rand() * push_x) * (PIECE_PUSH * RAND_SCALE);
    motion->speed.y = RANDOM(RISE_MIN, RISE_MAX);
    motion->speed.z = (rand() * push_z) * (PIECE_PUSH * RAND_SCALE);
    motion->spin_y = RANDOM(-0.5F, 0.5F) * SPIN_Y;
    motion->spin_x = RANDOM(-0.5F, 0.5F) * SPIN_X;
    task->step = beach_pier_piece_step;
    task->draw = beach_pier_piece_draw;
    task->destructor = beach_pier_piece_destroy;
    beach_pier_piece_draw(task);
}

/* a waiting piece counts its timer down to 0, then flies. it falls once its
 * y speed turns down, and sinks once it reaches SEA_LEVEL. it sinks at
 * SINK_SPEED while PIECE_DRAG slows its x and z speeds. it stops being drawn
 * after SINK_STEPS */
static void beach_pier_piece_step(struct task *task)
{
    struct task_work *work = task->work;
    struct piece_motion *motion = PIECE_MOTION(task);
    /* @hack spelling: pos and speed read and written back a Float at a time
     * through byte cursors over the whole records. read through the members,
     * 138 of 176 words differ. written through them, the function runs 28
     * bytes short and 165 differ */
    char *pos = (char *)&work->pos;
    Float x;
    Float y;
    Float z;
    char *speed = (char *)&motion->speed;
    Float vx;
    Float vy;
    Float vz;
    /* @hack redundant: splash, set as the piece lands and never read
     * (0x0c903b74 to 0x0c903b80). without it, the function runs 32 bytes
     * short and 79 of 176 words differ */
    NJS_VECTOR splash;

    x = *(Float *)pos;
    pos += sizeof(Float);
    y = *(Float *)pos;
    pos += sizeof(Float);
    z = *(Float *)pos;
    pos += sizeof(Float);
    pos -= 3 * sizeof(Float);
    vx = *(Float *)speed;
    speed += sizeof(Float);
    vy = *(Float *)speed;
    speed += sizeof(Float);
    vz = *(Float *)speed;
    speed += sizeof(Float);
    speed -= 3 * sizeof(Float);
    switch (work->mode) {
    case PIECE_WAIT:
        TASK_TIMER(work)--;
        if (TASK_TIMER(work) <= 0) {
            work->timer = 0;
            work->mode = PIECE_FLY;
        }
        beach_pier_piece_draw(task);
        break;
    case PIECE_FLY:
        x += vx;
        y += vy;
        z += vz;
        vy -= PIECE_GRAVITY;
        work->ang.y += motion->spin_y;
        work->ang.x += motion->spin_x;
        if (vy < 0.0F) {
            work->mode = PIECE_FALL;
        }
        beach_pier_piece_draw(task);
        break;
    case PIECE_FALL:
        x += vx;
        y += vy;
        z += vz;
        work->ang.y += motion->spin_y;
        work->ang.x += motion->spin_x;
        vy -= PIECE_GRAVITY;
        if (y <= SEA_LEVEL) {
            work->mode = PIECE_SINK;
            work->timer = 0;
            vy = SINK_SPEED;
            splash.x = vx * 0.1F;
            splash.y = 0.0F;
            splash.z = vz * 0.1F;
        }
        beach_pier_piece_draw(task);
        break;
    case PIECE_SINK:
        TASK_TIMER(work)++;
        if (TASK_TIMER(work) > SINK_STEPS) {
            work->mode = PIECE_GONE;
        }
        x += vx;
        y += vy;
        z += vz;
        vx *= PIECE_DRAG;
        vz *= PIECE_DRAG;
        beach_pier_piece_draw(task);
        break;
    case PIECE_GONE:
        task->draw = NULL;
        break;
    }
    *(Float *)pos = x;
    pos += sizeof(Float);
    *(Float *)pos = y;
    pos += sizeof(Float);
    *(Float *)pos = z;
    *(Float *)speed = vx;
    speed += sizeof(Float);
    *(Float *)speed = vy;
    speed += sizeof(Float);
    *(Float *)speed = vz;
}

static void beach_pier_piece_draw(struct task *task)
{
    struct task_work *work = task->work;
    Angle ang;

    texture_set_stage();
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    TURN_VIA(njRotateX, work->ang.x, ang);
    TURN_VIA(njRotateY, work->ang.y, ang);
    draw_model(PIECE(work)->part->model);
    njPopMatrix(1);
}

static void beach_pier_piece_destroy(struct task *task)
{
}
