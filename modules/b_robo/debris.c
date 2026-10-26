/* Zero's debris. each node of the model, as it was posed, becomes a piece.
 * a piece waits a moment, then flies out from a point, falling and
 * spinning. the debris goes once every piece has flown FLIGHT_STEPS
 * steps */
#include <ninja.h>
#include <sg_xpt.h>
#include <stdlib.h>

#include "b_robo/banks.h"
#include "b_robo/debris.h"
#include "draw/model.h"
#include "draw/pose.h"
#include "system/frame.h"
#include "system/math.h"
#include "system/task.h"
#include "system/util.h"

/* a piece waits up to DELAY_MAX steps, then flies FLIGHT_STEPS */
#define DELAY_MAX 60.0F
#define FLIGHT_STEPS 250

/* added to a flying piece's upward speed each step */
#define GRAVITY (-0.02F)

struct debris_piece {
    NJS_MATRIX matrix; /* its node's, as posed */
    NJS_POINT3 offset; /* how far it has flown */
    NJS_VECTOR speed;
    /* its turn a step about each axis */
    Angle spin_x;
    Angle spin_y;
    Angle spin_z;
    int steps; /* the steps since it took off, below 0 while it waits */
};

/* the debris's record, at its task's extra */
struct debris {
    NJS_POINT3 origin;
    struct debris_piece *pieces;
    NJS_OBJECT *object; /* the tree the pieces are drawn with */
    int count;
    int steps; /* the steps since the debris was made, which nothing reads */
};

static void debris_destroy(struct task *task);
static void debris_count(NJS_OBJECT *object, int *count);
static void debris_pose(NJS_ACTION *action, struct debris_piece *pieces,
                        Float frame, int count);
static void debris_draw(struct task *task);
static void debris_step(struct task *task);

void erobo_debris_create(NJS_POINT3 *origin, NJS_ACTION *action, Float frame)
{
    struct task *task;
    struct debris *debris;
    int count;

    if ((task = task_create(0, 4, debris_step)) == NULL) {
        return;
    }
    debris = heap_alloc(sizeof(*debris));
    if (debris == NULL) {
        task_destroy(task);
        return;
    }
    count = 0;
    debris_count(action->object, &count);
    task->extra = debris;
    VECTOR_COPY(&debris->origin, origin);
    debris->object = action->object;
    debris->pieces = heap_alloc(count * sizeof(struct debris_piece));
    debris_pose(action, debris->pieces, frame, count);
    debris->count = count;
    debris->steps = 0;
    task->draw = debris_draw;
    task->destructor = debris_destroy;
}

static void debris_destroy(struct task *task)
{
    struct debris *debris = task->extra;

    heap_free(debris->pieces);
}

/* add to *count the nodes of object, its siblings and their children */
static void debris_count(NJS_OBJECT *object, int *count)
{
    do {
        ++*count;
        if (object->child != NULL) {
            debris_count(object->child, count);
        }
        object = object->sibling;
    } while (object != NULL);
}

/* give each piece its node's matrix as the action poses it at frame. give
 * each a wait of up to DELAY_MAX steps too */
static void debris_pose(NJS_ACTION *action, struct debris_piece *pieces,
                        Float frame, int count)
{
    struct debris_piece *piece;
    int i;

    pose_action(NULL, action, frame, count, &pieces[count - 1].matrix);
    /* @hack redundant: a call of pose_get_matrix that never runs, which keeps
     * the function's address in R8 through the loop (0x0c9046a0,
     * 0x0c9046ba). without it, R8 holds rand's address, the loop loads
     * pose_get_matrix's into R3, and 8 of 86 words differ */
    if (0) {
        pose_get_matrix(0, NULL);
    }
    for (i = 0; i < count; i++) {
        piece = &pieces[i];
        pose_get_matrix(i, &piece->matrix);
        piece->offset.x = piece->offset.y = piece->offset.z = 0.0F;
        piece->speed.x = piece->speed.y = piece->speed.z = 0.0F;
        piece->spin_x = piece->spin_y = piece->spin_z = 0;
        /* @bug logic: the wait truncates toward 0 (FTRC at 0x0c9046f6),
         * so about 1 piece in 60 starts at 0 and never waits. debris_fly
         * never launches such a piece: it falls straight down from its pose,
         * without spinning */
        piece->steps = (int)RANDOM(-DELAY_MAX, 0.0F);
    }
}

/* draw each node of object's tree that has a model, where its piece has
 * flown. *piece walks the pieces in the order debris_count counted the
 * nodes */
static void debris_draw_tree(NJS_OBJECT *object, struct debris_piece **piece)
{
    do {
        if (object->model != NULL) {
            njPushMatrix(NULL);
            njTranslateV(NULL, &(*piece)->offset);
            njMultiMatrix(NULL, &(*piece)->matrix);
            draw_model(object->model);
            njPopMatrix(1);
        }
        ++*piece;
        if (object->child != NULL) {
            debris_draw_tree(object->child, piece);
        }
        object = object->sibling;
    } while (object != NULL);
}

/* on a frame's first step, draw the pieces */
static void debris_draw(struct task *task)
{
    struct debris *debris = task->extra;
    struct debris_piece *piece;

    if (frame_step != 0) {
        return;
    }
    piece = debris->pieces;
    njSetTexture(erobo_textures);
    debris_draw_tree(debris->object, &piece);
}

/* run a piece a step. the step its wait ends, it takes off away from the
 * origin and upward. from then on it falls and spins. a piece that started
 * with no wait only falls. returns FALSE once it has flown FLIGHT_STEPS
 * steps, and TRUE until then */
static Bool debris_fly(struct debris *debris, struct debris_piece *piece)
{
    NJS_POINT3 pos;
    NJS_VECTOR dir;

    if (piece->steps++ < 0) {
        if (piece->steps >= 0) {
            njGetTranslation(&piece->matrix, &pos);
            VECTOR_DIFF(&dir, &pos, &debris->origin);
            vector_unit(&dir, &dir);
            VECTOR_SCALE(&piece->speed, &dir, 1.0F);
            if (piece->speed.y <= 0.0F) {
                piece->speed.y = -piece->speed.y;
            }
            piece->speed.x += RANDOM(-0.5F, 0.5F);
            piece->speed.z += RANDOM(-0.5F, 0.5F);
            piece->speed.y += RANDOM(0.0F, 1.0F);
            piece->spin_x = (Angle)(RANDOM(-10.0F, 10.0F) * ANGLE_PER_DEGREE);
            piece->spin_y = (Angle)(RANDOM(-10.0F, 10.0F) * ANGLE_PER_DEGREE);
            piece->spin_z = (Angle)(RANDOM(-10.0F, 10.0F) * ANGLE_PER_DEGREE);
        }
    } else {
        piece->speed.y += GRAVITY;
        VECTOR_SUM(&piece->offset, &piece->offset, &piece->speed);
        njRotateZXY(&piece->matrix, piece->spin_x, piece->spin_y,
                    piece->spin_z);
        if (piece->steps >= FLIGHT_STEPS) {
            return FALSE;
        }
    }
    return TRUE;
}

/* run each piece a step, and draw them. the debris goes once no piece
 * flies */
static void debris_step(struct task *task)
{
    struct debris *debris = task->extra;
    struct debris_piece *piece;
    int count;
    Bool flying;

    ++debris->steps;
    count = debris->count;
    piece = debris->pieces;
    flying = FALSE;
    while (count > 0) {
        flying |= debris_fly(debris, piece);
        --count;
        ++piece;
    }
    if (!flying) {
        task_kill(task);
    } else {
        debris_draw(task);
    }
}
