/* O FROG: a frog in a bubble, which clears the stage when player 0 touches
 * it */
#include <ninja.h>

#include "animal/barrier.h"
#include "collision/collision.h"
#include "debug/editor.h"
#include "draw/model.h"
#include "draw/tree.h"
#include "object/placement.h"
#include "player/big_kaeru_textures.h"
#include "player/player.h"
#include "progress/progress.h"
#include "stage/stage.h"
#include "stg01/frog.h"
#include "system/task.h"
#include "system/util.h"

/* the frog Big fishes for: its whole tree, and the nodes beach_frog_nodes
 * lists */
extern NJS_OBJECT frog_object;
extern NJS_OBJECT frog_node_1_object;
extern NJS_OBJECT frog_node_2_object;
extern NJS_OBJECT frog_node_3_object;
extern NJS_OBJECT frog_node_4_object;
extern NJS_OBJECT frog_node_5_object;
extern NJS_OBJECT frog_node_6_object;
extern NJS_OBJECT frog_node_7_object;
extern NJS_OBJECT frog_node_8_object;
extern NJS_OBJECT frog_node_9_object;
extern NJS_OBJECT frog_node_10_object;
extern NJS_OBJECT frog_node_11_object;
extern NJS_OBJECT frog_node_12_object;

#define FROG_NODES 12

/* the nodes beach_frog_draw_nodes draws. the frog's tree has fourteen nodes
 * with models, and this leaves out two: a node and its child */
static NJS_OBJECT *beach_frog_nodes[FROG_NODES] = {
    &frog_node_1_object,  &frog_node_2_object,  &frog_node_3_object,
    &frog_node_4_object,  &frog_node_5_object,  &frog_node_6_object,
    &frog_node_7_object,  &frog_node_8_object,  &frog_node_9_object,
    &frog_node_10_object, &frog_node_11_object, &frog_node_12_object,
};

enum beach_frog_mode { FROG_INIT = 0, FROG_WAIT = 2, FROG_CAUGHT = 3 };

/* player 0 catches the frog within (scale x + 1) * REACH of it */
#define REACH 14.0F

/* how far the frog bobs above and below its place */
#define BOB_HEIGHT 2.5F

static void beach_frog_draw_nodes(struct task *task);
static void beach_frog_draw_whole(struct task *task);
static void beach_frog_bob(struct task *task);
static void beach_frog_draw_reach(struct task *task);
static void beach_frog_touch(struct task *task);
static void beach_frog_draw(struct task *task);

void beach_frog_step(struct task *task)
{
    struct task_work *work = task->work;

    if (placement_try_leave(task)) {
        return;
    }
    switch (work->mode) {
    case FROG_INIT:
        work->mode = FROG_WAIT;
        task->draw = beach_frog_draw;
        break;
    case FROG_WAIT:
        if (!placement_editing(task)) {
            beach_frog_touch(task);
        }
        if (placement_editing(task)) {
            njPrintC(EDITOR_LABEL(EDITOR_ROW_SCL_X), "<- SCALE");
            beach_frog_draw_reach(task);
        }
        beach_frog_bob(task);
        beach_frog_draw(task);
        break;
    case FROG_CAUGHT:
        break;
    }
}

/* draw each node of beach_frog_nodes moved by its own pos and turned with
 * the frog. the node's own angles and its parents' moves don't apply. then
 * draw the bubble, turned TASK_TIMER(work) degrees about y. work->scl.y
 * raises both by the bob */
static void beach_frog_draw_nodes(struct task *task)
{
    struct task_work *work = task->work;
    NJS_OBJECT **nodes;
    NJS_OBJECT *node;
    Angle ang;
    int i;

    njPushMatrix(NULL);
    njTranslate(NULL, work->pos.x, work->pos.y + work->scl.y, work->pos.z);
    TURN_VIA(njRotateZ, work->ang.z, ang);
    TURN_VIA(njRotateX, work->ang.x, ang);
    TURN_VIA(njRotateY, work->ang.y, ang);
    njSetTexture(&big_kaeru_texlist);
    /* @hack spelling: the cursor nodes, which reads each node by MOV.L @R8+,R3
     * at 0x0c90d97c. as beach_frog_nodes[i], 40 of 156 words differ */
    for (i = 0, nodes = beach_frog_nodes; i < FROG_NODES; i++) {
        node = *nodes++;
        njPushMatrix(NULL);
        njTranslate(NULL, node->pos[0], node->pos[1], node->pos[2]);
        draw_model(node->model);
        njPopMatrix(1);
    }
    njSetTexture(&al_barria_texlist);
    TURN_VIA(njRotateY, (Angle)(TASK_TIMER(work) * ANGLE_PER_DEGREE), ang);
    draw_tree(&al_barria_object);
    njPopMatrix(1);
}

static void beach_frog_draw_whole(struct task *task)
{
    struct task_work *work = task->work;
    NJS_OBJECT *object;
    Angle ang;

    njSetTexture(&big_kaeru_texlist);
    njPushMatrix(NULL);
    object = &frog_object;
    njTranslate(NULL, work->pos.x, work->pos.y + work->scl.y, work->pos.z);
    TURN_VIA(njRotateZ, work->ang.z, ang);
    TURN_VIA(njRotateX, work->ang.x, ang);
    TURN_VIA(njRotateY, work->ang.y, ang);
    njSimpleDrawObject(object);
    njSetTexture(&al_barria_texlist);
    TURN_VIA(njRotateY, (Angle)(TASK_TIMER(work) * ANGLE_PER_DEGREE), ang);
    draw_tree(&al_barria_object);
    njPopMatrix(1);
}

/* advance TASK_TIMER(work) a degree, which turns the bubble too. then set
 * work->scl.y to the bob, which goes round once each 360 steps */
static void beach_frog_bob(struct task *task)
{
    struct task_work *work = task->work;

    TASK_TIMER(work)++;
    work->scl.y =
        njSin((Angle)(TASK_TIMER(work) * ANGLE_PER_DEGREE)) * BOB_HEIGHT;
}

/* in the set editor, draw marker_object scaled by scale x + 1. it shows a
 * sphere of radius (scale x + 1) * 10. the touch reaches farther, to (scale
 * x + 1) * REACH */
static void beach_frog_draw_reach(struct task *task)
{
    struct task_work *work = task->work;
    /* @hack type: a vector, of which only x is used, whose 12 bytes give the
     * image's ADD #-12,R15 (0x0c90db9c). as one Float, 40 of 41 words
     * differ */
    NJS_VECTOR scale;

    scale.x = work->scl.x + 1.0F;
    njPushMatrix(NULL);
    njTranslate(NULL, work->pos.x, work->pos.y, work->pos.z);
    njScale(NULL, scale.x, scale.x, scale.x);
    draw_tree(&marker_object);
    njPopMatrix(1);
}

static void beach_frog_touch(struct task *task)
{
    struct task_work *work = task->work;

    if (player_in_range(&work->pos, (work->scl.x + 1.0F) * REACH) == 1) {
        stage_clear();
        work->mode = FROG_CAUGHT;
    }
}

/* draw the frog node by node once E-102 has cleared Emerald Coast, and its
 * whole tree before */
static void beach_frog_draw(struct task *task)
{
    if (event_flag_check(FLAG_E102_CLEAR_BEACH)) {
        beach_frog_draw_nodes(task);
    } else {
        beach_frog_draw_whole(task);
    }
}
