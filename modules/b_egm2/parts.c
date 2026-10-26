/* the Egg Walker's parts: twenty points on its posed model and its turret.
 * each has a ball in the boss's collision record */
#include <ninja.h>
#include <sg_xpt.h>

#include "b_egm2/egg_walker.h"
#include "b_egm2/parts.h"
#include "collision/collision.h"
#include "draw/pose.h"
#include "player/player.h"
#include "system/task.h"
#include "system/util.h"

/* where a part lies: at offset in node's frame, as the boss is posed. a part
 * on_turret lies at offset from part 0 instead, turned with the boss and then
 * the turret. its node goes unread */
struct part {
    Uint8 node;
    NJS_POINT3 offset;
    Bool on_turret;
};

/* the boss's parts, and its foot parts from PART_FEET. the foot parts are
 * each foot, then a point fore or aft of each */
#define PARTS 20
#define FEET 8

/* how far the boss's record reaches from its position */
#define SHAPES_REACH 150.0F

/* a foot part's ball attacks only while its bottom is more than FOOT_CLEAR
 * above player 0's position */
#define FOOT_CLEAR 5.0F

#define PART_SHAPE(levels, radius)                                             \
    {SHAPE_SPHERE | SHAPE_IN_WORLD | SHAPE_HURTABLE,                           \
     (levels),                                                                 \
     PUSH_ALL | PUSH_FIRM(PUSH_ALL),                                           \
     {0.0F, 0.0F, 0.0F},                                                       \
     (radius),                                                                 \
     0.0F,                                                                     \
     0.0F,                                                                     \
     {0, 0, 0},                                                                \
     0,                                                                        \
     NULL}

/* the body, and the cockpit ahead of it on the turret; a point on each leg;
 * each foot, then a point fore or aft of each; the top of each leg; and two
 * more on the turret, behind. part 0 comes first, so the turret's parts
 * find it placed */
static const struct part parts[PARTS] = {
    {0, {0.0F, -4.0F, 0.0F}, FALSE},    {0, {0.0F, 0.0F, 15.0F}, TRUE},
    {7, {-12.0F, -4.5F, 0.0F}, FALSE},  {15, {12.0F, -4.5F, 0.0F}, FALSE},
    {23, {-12.0F, -4.5F, 0.0F}, FALSE}, {31, {12.0F, -4.5F, 0.0F}, FALSE},
    {8, {0.0F, -2.0F, 0.0F}, FALSE},    {16, {0.0F, -2.0F, 0.0F}, FALSE},
    {24, {0.0F, -2.0F, 0.0F}, FALSE},   {32, {0.0F, -2.0F, 0.0F}, FALSE},
    {8, {0.0F, -9.0F, 15.0F}, FALSE},   {16, {0.0F, -9.0F, 15.0F}, FALSE},
    {24, {0.0F, -9.0F, -15.0F}, FALSE}, {32, {0.0F, -9.0F, -15.0F}, FALSE},
    {6, {0.0F, 0.0F, 0.0F}, FALSE},     {14, {0.0F, 0.0F, 0.0F}, FALSE},
    {22, {0.0F, 0.0F, 0.0F}, FALSE},    {30, {0.0F, 0.0F, 0.0F}, FALSE},
    {0, {20.0F, 18.0F, -15.0F}, TRUE},  {0, {-20.0F, 18.0F, -15.0F}, TRUE},
};

/* how far below each foot part's center egg_walker_feet_attack takes its
 * ball's bottom to be */
static const Float foot_heights[FEET] = {
    13.6F, 13.6F, 13.6F, 13.6F, 6.6F, 6.6F, 6.6F, 6.6F,
};

static struct collision_shape part_shapes[PARTS] = {
    PART_SHAPE(SHAPE_GUARD | SHAPE_HURTS_PLAYERS, 20.0F),
    PART_SHAPE(SHAPE_HURTS_PLAYERS, 20.0F),
    PART_SHAPE(SHAPE_HURTS_PLAYERS, 10.0F),
    PART_SHAPE(SHAPE_HURTS_PLAYERS, 10.0F),
    PART_SHAPE(SHAPE_HURTS_PLAYERS, 10.0F),
    PART_SHAPE(SHAPE_HURTS_PLAYERS, 10.0F),
    PART_SHAPE(SHAPE_GUARD | SHAPE_HURTS_PLAYERS, 15.0F),
    PART_SHAPE(SHAPE_GUARD | SHAPE_HURTS_PLAYERS, 15.0F),
    PART_SHAPE(SHAPE_GUARD | SHAPE_HURTS_PLAYERS, 15.0F),
    PART_SHAPE(SHAPE_GUARD | SHAPE_HURTS_PLAYERS, 15.0F),
    PART_SHAPE(SHAPE_GUARD | SHAPE_HURTS_PLAYERS, 10.0F),
    PART_SHAPE(SHAPE_GUARD | SHAPE_HURTS_PLAYERS, 10.0F),
    PART_SHAPE(SHAPE_GUARD | SHAPE_HURTS_PLAYERS, 10.0F),
    PART_SHAPE(SHAPE_GUARD | SHAPE_HURTS_PLAYERS, 10.0F),
    PART_SHAPE(SHAPE_GUARD | SHAPE_HURTS_PLAYERS, 18.0F),
    PART_SHAPE(SHAPE_GUARD | SHAPE_HURTS_PLAYERS, 18.0F),
    PART_SHAPE(SHAPE_GUARD | SHAPE_HURTS_PLAYERS, 18.0F),
    PART_SHAPE(SHAPE_GUARD | SHAPE_HURTS_PLAYERS, 18.0F),
    PART_SHAPE(SHAPE_HURTS_PLAYERS, 23.0F),
    PART_SHAPE(SHAPE_HURTS_PLAYERS, 23.0F),
};

/* each part's up and place in the world, as the last update left them */
static NJS_VECTOR part_ups[PARTS];
static NJS_POINT3 part_positions[PARTS];
static NJS_MATRIX body_matrix;

void egg_walker_shapes_init(struct task *task)
{
    collision_init(task, part_shapes, COLLISION_ENEMY, 1, PARTS);
    task->work->collision->radius = SHAPES_REACH;
}

/* put in pos where offset lies on node, as the boss is posed. a null offset
 * puts the node's own point there. put the node's up in up. pos and up can
 * be null */
static void node_point(struct task_work *work, Uint8 node, NJS_POINT3 *offset,
                       NJS_POINT3 *pos, NJS_VECTOR *up)
{
    static NJS_POINT3 node_center = {0.0F, 0.0F, 0.0F};
    static NJS_VECTOR node_up = {0.0F, 1.0F, 0.0F};
    Angle ang;
    NJS_MATRIX *matrix = heap_alloc(sizeof(NJS_MATRIX));

    njPushMatrix(&_unit_matrix_);
    njTranslateV(NULL, &work->pos);
    TURN_VIA(njRotateZ, work->ang.z, ang);
    TURN_VIA(njRotateX, work->ang.x, ang);
    TURN_VIA(njRotateY, 0x8000 - work->ang.y, ang);
    njPushMatrix(NULL);
    pose_get_matrix(node, matrix);
    njMultiMatrix(NULL, matrix);
    if (pos != NULL) {
        if (offset != NULL) {
            njCalcPoint(NULL, offset, pos);
        } else {
            njCalcPoint(NULL, &node_center, pos);
        }
    }
    if (up != NULL) {
        njCalcVector(NULL, &node_up, up);
    }
    njPopMatrix(1);
    njPopMatrix(1);
    heap_free(matrix);
}

/* put in out where offset lies from part 0, turned with the boss and the
 * turret's yaw. a null offset puts part 0 itself there. put the boss's
 * up in up. out and up can be null, and node goes unread */
static void turned_point(struct task_work *work, Uint8 node, NJS_POINT3 *offset,
                         NJS_POINT3 *out, NJS_VECTOR *up, Angle yaw)
{
    static NJS_POINT3 turned_center = {0.0F, 0.0F, 0.0F};
    static NJS_VECTOR turned_up = {0.0F, 1.0F, 0.0F};
    /* @hack spelling: a copy of out, which SHC keeps in R14 and work in R13
     * (MOV R7,R14 and MOV R4,R13 at 0x0c90121a and 0x0c90121c). with out
     * alone, the two trade and 8 of 89 words differ */
    NJS_POINT3 *pos = out;
    Angle ang;
    /* the matrix is allocated and freed, and nothing uses it */
    void *matrix = heap_alloc(sizeof(NJS_MATRIX));

    njPushMatrix(&_unit_matrix_);
    /* @hack redundant: a jump to the next line, which puts njTranslateV's
     * address in R3 and work->ang.z in R2 (0x0c90122c, 0x0c901234); without it,
     * the two trade and 5 of 89 words differ */
    goto translate;
translate:
    njTranslateV(NULL, part_positions);
    TURN_VIA(njRotateZ, work->ang.z, ang);
    TURN_VIA(njRotateX, work->ang.x, ang);
    TURN_VIA(njRotateY, 0x8000 - work->ang.y, ang);
    TURN_VIA(njRotateY, -yaw, ang);
    if (pos != NULL) {
        if (offset != NULL) {
            njCalcPoint(NULL, offset, pos);
        } else {
            njCalcPoint(NULL, &turned_center, pos);
        }
    }
    if (up != NULL) {
        njCalcVector(NULL, &turned_up, up);
    }
    njPopMatrix(1);
    njPopMatrix(1);
    heap_free(matrix);
}

void egg_walker_parts_update(struct task *task)
{
    struct task_work *work = task->work;
    struct egg_walker *boss = task->extra;
    const struct part *part;
    int i;
    /* @hack redundant: yaw, read and never used, which puts the turret branch's
     * frame slots in the image's order (MOV.L R2,@(8,R15) at 0x0c9013ae);
     * without it, two slots and R4 and R5 trade, and 34 of 359 words differ */
    Angle yaw = boss->turret_yaw;

    pose_get_matrix(0, &body_matrix);
    for (i = 0; i < PARTS; i++) {
        part = &parts[i];
        if (part->on_turret != TRUE) {
            node_point(work, part->node, (NJS_POINT3 *)&part->offset,
                       &part_positions[i], &part_ups[i]);
        } else {
            /* @hack spelling: the part read as parts[i], not through part,
             * which forms its address again from i (0x0c901398 to 0x0c9013bc).
             * through part, 314 of 359 words differ */
            turned_point(work, parts[i].node, (NJS_POINT3 *)&parts[i].offset,
                         &part_positions[i], &part_ups[i], boss->turret_yaw);
        }
        work->collision->shapes[i].center.x = part_positions[i].x;
        work->collision->shapes[i].center.y = part_positions[i].y;
        work->collision->shapes[i].center.z = part_positions[i].z;
    }
}

NJS_MATRIX *egg_walker_body_matrix(void)
{
    return &body_matrix;
}

void egg_walker_feet_attack(struct task_work *work)
{
    struct collision_shape *shape;
    int i;

    for (i = PART_FEET; i < PART_FEET + FEET; i++) {
        shape = &work->collision->shapes[i];
        if (shape->center.y - foot_heights[i - PART_FEET] >
            player_work[0]->pos.y + FOOT_CLEAR) {
            shape->levels = (shape->levels & ~SHAPE_ATTACK) | SHAPE_ATTACK;
        } else {
            shape->levels = shape->levels & ~SHAPE_ATTACK;
        }
    }
}

NJS_POINT3 *egg_walker_part_pos(int part)
{
    return &part_positions[part];
}
