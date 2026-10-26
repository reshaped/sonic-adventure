/* streamers, the ribbons a cracker fires. a streamer is a string of nodes,
 * from its head to its tail. for its first STREAMER_HOLD_STEPS steps, the head
 * rides a point that flies from the cracker, and each node pulls on its
 * neighbours. the tail stays at the cracker for the first STREAMER_TAIL_STEPS.
 * after that the ribbon falls free */
#include <ninja.h>
#include <sg_xpt.h>
#include <stdlib.h>

#include "al_race/streamer.h"
#include "system/frame.h"
#include "system/math.h"
#include "system/task.h"
#include "system/util.h"

#define STREAMER_NODES 10
/* two points for each node, one each side of it */
#define STREAMER_POINTS (STREAMER_NODES * 2)

/* the ribbon's half width, from a node to each edge */
#define STREAMER_HALF_WIDTH 0.3F
#define STREAMER_PALETTES 4

#define STREAMER_TAIL_STEPS 20
#define STREAMER_HOLD_STEPS 100
/* the step at which a streamer ends, unless both its ends fall below
 * STREAMER_FLOOR first */
#define STREAMER_LIFE 1000
/* the height at or below which head, the point the head rides, stops
 * flying. a streamer ends once both its ends are below it */
#define STREAMER_FLOOR 10.0F
/* how much a node's speed falls each step, and head_speed's while head flies */
#define STREAMER_FALL 0.003F
#define STREAMER_HEAD_FALL 0.005F

struct streamer_node {
    NJS_POINT3 pos;
    NJS_VECTOR speed;
    NJS_VECTOR accel; /* cleared once a step has used it */
    NJS_VECTOR side; /* from the node to one edge; the other is opposite */
};

/* a streamer's state, which its task's extra holds, from the heap */
struct streamer {
    int timer; /* the steps it has lived */
    int palette; /* its index in streamer_palettes */
    /* the point it was fired from, where the tail stays at first */
    NJS_POINT3 base;
    NJS_POINT3 head; /* the point the head rides, which flies from base */
    NJS_VECTOR head_speed;
    struct streamer_node nodes[STREAMER_NODES]; /* from the head to the tail */
};

static void streamer_start(struct task *task, NJS_POINT3 *pos,
                           NJS_VECTOR *speed);
static void streamer_destroy(struct task *task);
static void streamer_draw(struct task *task);
static void streamer_step(struct task *task);

static NJS_COLOR streamer_red[STREAMER_POINTS];
static NJS_COLOR streamer_green[STREAMER_POINTS];
static NJS_COLOR streamer_blue[STREAMER_POINTS];
static NJS_COLOR streamer_yellow[STREAMER_POINTS];
/* each node's two edge points, as streamer_draw last set them for
 * streamer_strip */
static NJS_POINT3 streamer_points[STREAMER_POINTS];

static NJS_POINT3COL streamer_strip = {
#include "al_race/streamer/streamer_strip.inc"
};
static NJS_COLOR *streamer_palettes[STREAMER_PALETTES] = {
    streamer_red, streamer_green, streamer_blue, streamer_yellow};

void streamers_init(void)
{
    int i;

    for (i = 0; i < STREAMER_POINTS; i++) {
        streamer_red[i].color = 0x00c02020;
        streamer_green[i].color = 0x0020c020;
        streamer_blue[i].color = 0x002020c0;
        streamer_yellow[i].color = 0x00c0c020;
    }
}

void streamer_create(NJS_POINT3 *pos, NJS_VECTOR *speed)
{
    struct task *task;
    struct streamer *streamer;

    if ((task = task_create(0, 4, streamer_step)) != NULL) {
        if ((streamer = heap_alloc(sizeof(struct streamer))) == NULL) {
            task_destroy(task);
        } else {
            task->extra = streamer;
            streamer_start(task, pos, speed);
            task->draw = streamer_draw;
            task->destructor = streamer_destroy;
        }
    }
}

static void streamer_start(struct task *task, NJS_POINT3 *pos,
                           NJS_VECTOR *speed)
{
    struct streamer *streamer = task->extra;
    struct streamer_node *node;
    NJS_VECTOR side;
    NJS_VECTOR head_speed;
    Float sine;
    Float cosine;
    int i;
    Angle ang = angle_arc_tan2(speed->z, speed->x);

    /* the ribbon lies across its flight */
    _builtin_fsca(ang + NJM_DEG_ANG(90), &sine, &cosine);
    side.x = cosine * STREAMER_HALF_WIDTH;
    side.z = sine * STREAMER_HALF_WIDTH;
    node = streamer->nodes;
    i = 0;
    do {
        node->pos.x = pos->x;
        node->pos.y = pos->y;
        node->pos.z = pos->z;
        node->speed.x = node->speed.y = node->speed.z = 0.0F;
        node->accel.x = node->accel.y = node->accel.z = 0.0F;
        /* @hack spelling: side through (&side)->, which forms its address from
         * R15 in the loop (MOV R15,R3 and ADD #20,R3 at 0x0cb98096 and
         * 0x0cb980c4); as side.x and side.z, 126 of 165 words differ */
        node->side.x = (&side)->x;
        node->side.z = (&side)->z;
        node++;
    } while (++i < STREAMER_NODES);
    streamer->base.x = pos->x;
    streamer->base.y = pos->y;
    streamer->base.z = pos->z;
    streamer->head.x = pos->x;
    streamer->head.y = pos->y;
    streamer->head.z = pos->z;
    head_speed.x = speed->x + (rand() * (0.4F * RAND_SCALE)) - 0.2F;
    head_speed.y = speed->y + (rand() * (0.4F * RAND_SCALE)) - 0.2F;
    head_speed.z = speed->z + (rand() * (0.4F * RAND_SCALE)) - 0.2F;
    /* @hack spelling: head_speed through (&head_speed)->, which forms its
     * address from R15 for each read (MOV R15,R3 and ADD #8,R3 at 0x0cb9813c to
     * 0x0cb9815e); as head_speed.x and the rest, the function runs 32 bytes
     * short and 160 of 165 words differ */
    streamer->head_speed.x = (&head_speed)->x;
    streamer->head_speed.y = (&head_speed)->y;
    streamer->head_speed.z = (&head_speed)->z;
    streamer->palette = rand() % STREAMER_PALETTES;
    streamer->timer = 0;
}

static void streamer_destroy(struct task *task)
{
}

static void streamer_draw(struct task *task)
{
    struct streamer *streamer = task->extra;
    struct streamer_node *node = streamer->nodes;
    NJS_POINT3 *point;
    int i;

    if (frame_step != 0) {
        return;
    }
    for (i = 0, point = streamer_points; i < STREAMER_NODES; i++, node++) {
        point->x = node->pos.x + node->side.x;
        point->z = node->pos.z + node->side.z;
        point->y = node->pos.y;
        point++;
        point->x = node->pos.x - node->side.x;
        point->z = node->pos.z - node->side.z;
        point->y = node->pos.y;
        point++;
    }
    streamer_strip.col = streamer_palettes[streamer->palette];
    njDrawTriangle3D(&streamer_strip, STREAMER_POINTS - 2,
                     NJD_FILL | NJD_DRAW_CONNECTED);
}

/* add to node's accel force times the way from node to other */
static void streamer_pull(struct streamer_node *node,
                          struct streamer_node *other, Float force)
{
    NJS_VECTOR way;

    /* @hack spelling: way through (&way)->, which forms its address from R15 at
     * each use (MOV R15,R3 at 0x0cb9827c to 0x0cb982d4); as way.x and the
     * rest, the function runs 22 bytes short and 53 of 57 words differ */
    (&way)->x = other->pos.x - node->pos.x;
    (&way)->y = other->pos.y - node->pos.y;
    (&way)->z = other->pos.z - node->pos.z;
    (&way)->x = (&way)->x * force;
    (&way)->y = (&way)->y * force;
    (&way)->z = (&way)->z * force;
    /* @hack spelling: the sums written out, which read way's y and z by
     * FMOV.S @(R0,R3), their offsets in R0 (0x0cb982ce, 0x0cb982dc); with +=,
     * SHC forms their addresses in R1, and 21 of 57 words differ */
    node->accel.x = node->accel.x + (&way)->x;
    node->accel.y = node->accel.y + (&way)->y;
    node->accel.z = node->accel.z + (&way)->z;
}

/* take drag's share of the node's speed and STREAMER_FALL off its accel.
 * then add its accel to its speed, add its speed to its position, and clear
 * its accel */
static void streamer_move(struct streamer_node *node, Float drag)
{
    NJS_VECTOR slow;

    /* @hack spelling: slow through (&slow)->, which forms its address from R15
     * at each use (MOV R15,R3 at 0x0cb982ee to 0x0cb98324); as slow.x and
     * the rest, 79 of 89 words differ */
    (&slow)->x = node->speed.x * drag;
    (&slow)->y = node->speed.y * drag;
    (&slow)->z = node->speed.z * drag;
    /* @hack spelling: the sums written out, which read node's fields by
     * FMOV.S @(R0,R4), their offsets in R0 (0x0cb98312 to 0x0cb98384); with
     * += and -=, SHC forms some of their addresses in R1, and streamer.c's P
     * runs 32 bytes past its row, count unmeasured */
    node->accel.x = node->accel.x - (&slow)->x;
    node->accel.y = node->accel.y - (&slow)->y;
    node->accel.z = node->accel.z - (&slow)->z;
    node->accel.y += -STREAMER_FALL;
    node->speed.x = node->speed.x + node->accel.x;
    node->speed.y = node->speed.y + node->accel.y;
    node->speed.z = node->speed.z + node->accel.z;
    node->pos.x = node->pos.x + node->speed.x;
    node->pos.y = node->pos.y + node->speed.y;
    node->pos.z = node->pos.z + node->speed.z;
    node->accel.x = node->accel.y = node->accel.z = 0.0F;
}

/* pull each node toward its neighbours, then move every node. the tail stays
 * at base for the first STREAMER_TAIL_STEPS, and the head stays at head. at
 * step STREAMER_HOLD_STEPS, the head takes head_speed, as it is about to fall
 * free */
static void streamer_hold(struct streamer *streamer)
{
    struct streamer_node *node;
    struct streamer_node *first = streamer->nodes;
    NJS_POINT3 *base;
    NJS_POINT3 *head;
    int i;

    base = &streamer->base;
    head = &streamer->head;
    node = first;
    for (i = 0; i < STREAMER_NODES - 1; i++) {
        node++;
        streamer_pull(node, node - 1, 0.1F);
    }
    node = streamer->nodes;
    for (i = 0; i < STREAMER_NODES - 1; i++) {
        streamer_pull(node, node + 1, 0.1F);
        node++;
    }
    node = streamer->nodes;
    for (i = 0; i < STREAMER_NODES; i++, node++) {
        streamer_move(node, 0.15F);
    }
    if (streamer->timer < STREAMER_TAIL_STEPS) {
        streamer->nodes[STREAMER_NODES - 1].pos.x = base->x;
        streamer->nodes[STREAMER_NODES - 1].pos.y = base->y;
        streamer->nodes[STREAMER_NODES - 1].pos.z = base->z;
    } else if (streamer->timer == STREAMER_HOLD_STEPS) {
        streamer->nodes[0].speed.x = streamer->head_speed.x;
        streamer->nodes[0].speed.y = streamer->head_speed.y;
        streamer->nodes[0].speed.z = streamer->head_speed.z;
    }
    streamer->nodes[0].pos.x = head->x;
    streamer->nodes[0].pos.y = head->y;
    streamer->nodes[0].pos.z = head->z;
}

/* let each node drift and fall, the ones toward the tail slower */
static void streamer_fall(struct streamer *streamer)
{
    struct streamer_node *node = streamer->nodes;
    int i;

    i = 0;
    do {
        node->speed.x *= 0.99F;
        node->speed.z *= 0.99F;
        node->speed.y *= 0.98F;
        node->speed.y += (i * 0.00002F) + -STREAMER_FALL;
        /* @hack spelling: the sums written out, which read the speed by
         * FMOV.S @(R0,R4), its offsets in R0 (0x0cb98548 to 0x0cb98560); with
         * +=, SHC forms two of its addresses in R1, and 39 of 54 words
         * differ */
        node->pos.x = node->pos.x + node->speed.x;
        node->pos.y = node->pos.y + node->speed.y;
        node->pos.z = node->pos.z + node->speed.z;
        node++;
    } while (++i < STREAMER_NODES);
}

/* move head by head_speed, then slow head_speed and take STREAMER_HEAD_FALL
 * off its y. at STREAMER_FLOOR or below, head_speed is zero */
static void streamer_fly(struct streamer *streamer)
{
    /* @hack spelling: the sums written out, which read head_speed by FMOV.S
     * @(R0,R4), its offsets in R0 (0x0cb98576 to 0x0cb98590); with +=, SHC
     * forms their addresses in R1, and 51 of 58 words differ */
    streamer->head.x = streamer->head.x + streamer->head_speed.x;
    streamer->head.y = streamer->head.y + streamer->head_speed.y;
    streamer->head.z = streamer->head.z + streamer->head_speed.z;
    streamer->head_speed.x *= 0.985F;
    streamer->head_speed.z *= 0.985F;
    streamer->head_speed.y *= 0.98F;
    if (streamer->head.y <= STREAMER_FLOOR) {
        streamer->head_speed.x = streamer->head_speed.y =
            streamer->head_speed.z = 0.0F;
    } else {
        streamer->head_speed.y += -STREAMER_HEAD_FALL;
    }
}

static void streamer_step(struct task *task)
{
    struct streamer *streamer = task->extra;

    if (++streamer->timer >= STREAMER_LIFE) {
        task_kill(task);
        return;
    }
    if (streamer->nodes[0].pos.y < STREAMER_FLOOR &&
        streamer->nodes[STREAMER_NODES - 1].pos.y < STREAMER_FLOOR) {
        task_kill(task);
        return;
    }
    streamer_fly(streamer);
    if (streamer->timer <= STREAMER_HOLD_STEPS) {
        streamer_hold(streamer);
    } else {
        streamer_fall(streamer);
    }
    streamer_draw(task);
}
