/* a Chaos's model melted down and out, and put back */
#include <mathf.h>
#include <ninja.h>
#include <sg_xpt.h>
#include <string.h>

#include "chaos/deform.h"
#include "chaos/steer.h"
#include "draw/motion.h"
#include "draw/pose.h"
#include "system/task.h"
#include "system/util.h"

/* the nodes a deformer has room for, and the deepest it walks */
#define DEFORM_NODES 120
#define DEFORM_DEPTH 12

/* a point of a node's model where the pose put it as the deformer was made.
 * ang_y and ang_z are its angles about the pose's y and z axes. DEFORM_MELT
 * reads ang_y, and nothing reads ang_z */
struct deform_point {
    Angle ang_y;
    Angle ang_z;
    NJS_POINT3 pos;
};

static NJS_MATRIX deform_matrix;

void deform_step(struct deform *deform)
{
    switch (deform->mode) {
    case DEFORM_MAKE: {
        struct deform_node *node;
        struct deform_point *point;
        NJS_OBJECT *object;
        NJS_POINT3 *points;
        int size;
        int count;
        /* @hack redundant: register on i and j, which keeps i in R14 and j
         * in R13 (MOV #0,R14 at 0x8c0fca08, MOV #0,R13 at 0x8c0fca7c); as
         * plain ints, they take R8 and R10, the frame shrinks by 4 bytes,
         * and 224 of 382 words differ */
        register int i;
        register int j;

        node = deform->nodes;
        for (i = 0; i < deform->count; i++, node++) {
            node->points = NULL;
            object = node->object;
            if (object->model == NULL) {
                continue;
            }
            count = object->model->nbPoint;
            size = count * sizeof(NJS_POINT3);
            points = heap_alloc(size);
            if (points == NULL) {
                for (;;) {
                }
            }
            node->points = points;
            memcpy(points, object->model->points, size);
            /* @hack type: (Uint16)i, which narrows the node's number by
             * EXTU.W for the call (0x8c0fca54); as i, a plain MOV, and 1 of
             * 382 words differ */
            pose_get_matrix((Uint16)i, &deform_matrix);
            njPushMatrix(&deform_matrix);
            points = node->points;
            point = heap_alloc(count * sizeof(struct deform_point));
            if (point == NULL) {
                for (;;) {
                }
            }
            node->posed = point;
            for (j = 0; j < count; j++) {
                njCalcPoint(NULL, points, &point->pos);
                point->ang_y = (Angle)(atan2f(point->pos.z, point->pos.x) *
                                       ANGLE_PER_RADIAN);
                point->ang_z = (Angle)(atan2f(point->pos.y, point->pos.x) *
                                       ANGLE_PER_RADIAN);
                points++;
                point++;
            }
            njPopMatrix(1);
        }
    }
        /* fall through */
    case DEFORM_RESTORE: {
        struct deform_node *node;
        NJS_OBJECT *object;
        NJS_POINT3 *from;
        NJS_POINT3 *to;
        int count;
        /* @hack redundant: register on i and j, which keeps i in R4
         * (MOV #0,R4 at 0x8c0fcad0); as plain ints, i takes R13, the frame
         * shrinks by 4 bytes, and 133 of 382 words differ */
        register int i;
        register int j;

        for (i = 0, node = deform->nodes; i < deform->count; i++, node++) {
            object = node->object;
            if (object->model == NULL) {
                continue;
            }
            from = node->points;
            to = object->model->points;
            count = object->model->nbPoint;
            for (j = 0; j < count; j++) {
                *to++ = *from++;
            }
        }
        deform->mode = DEFORM_IDLE;
        break;
    }
    case DEFORM_IDLE:
        break;
    case DEFORM_MELT: {
        struct deform_node *node;
        struct deform_point *point;
        NJS_OBJECT *object;
        NJS_MODEL *model;
        NJS_POINT3 *from;
        NJS_POINT3 *to;
        Float share;
        Float reach;
        Float limit;
        Angle ang;
        int count;
        /* @hack redundant: register on i and j, which keeps i in R14
         * (MOV #0,R14 at 0x8c0fcba2); as plain ints, i takes a frame slot,
         * the frame shrinks by 8 bytes, deform.c's code runs 32 bytes short
         * and 190 of 382 words differ */
        register int i;
        register int j;

        njPushMatrix(&_unit_matrix_);
        share = 1.0F / chaos_params->melt_grain * chaos_params->height;
        reach = deform->scale * deform->scale * chaos_params->melt_spread;
        njTranslate(NULL, 0.0F, chaos_params->model_y, 0.0F);
        node = deform->nodes;
        for (i = 0; i < deform->count; i++, node++) {
            NJS_VECTOR offset = {0.0F, 0.0F, 0.0F};
            NJS_POINT3 pos;

            object = node->object;
            if (object->model == NULL) {
                continue;
            }
            /* @hack type: (Uint16)i, which narrows the node's number by
             * EXTU.W for the call (0x8c0fcbd8); as i, a plain MOV, and 1 of
             * 382 words differ */
            pose_get_matrix((Uint16)i, &deform_matrix);
            from = node->points;
            model = object->model;
            to = model->points;
            count = model->nbPoint;
            point = node->posed;
            njPushMatrix(NULL);
            njMultiMatrix(NULL, &deform_matrix);
            njInvertMatrix(&deform_matrix);
            for (j = 0; j < count; j++) {
                ang = point->ang_y;
                njCalcPoint(NULL, from, &pos);
                limit = deform->scale * (chaos_params->melt_grain + (j & 7)) *
                        share;
                offset.x = reach * njSin(ang);
                offset.y = -(pos.y > limit ? limit : pos.y);
                offset.z = reach * njCos(ang);
                njPushMatrix(&deform_matrix);
                njCalcVector(NULL, &offset, to);
                njPopMatrix(1);
                njAddVector(to++, from++);
                point++;
            }
            njPopMatrix(1);
        }
        njPopMatrix(1);
        break;
    }
    case DEFORM_FREE: {
        struct deform_node *node;
        NJS_OBJECT *object;
        int i;

        for (i = 0, node = deform->nodes; i < deform->count; i++, node++) {
            object = node->object;
            if (object->model == NULL || node->points == NULL) {
                continue;
            }
            heap_free(node->points);
            heap_free(node->posed);
        }
        heap_free(deform->nodes);
        heap_free(deform);
        break;
    }
    }
}

struct deform *deform_create(NJS_OBJECT *root, struct task_work *work)
{
    NJS_OBJECT *stack[DEFORM_DEPTH];
    NJS_OBJECT *object;
    struct deform *deform;
    struct deform_node *node;
    int depth;
    Bool more;
    Bool climb;

    depth = 0;
    more = TRUE;
    deform = heap_calloc(1, sizeof(struct deform));
    if (deform == NULL) {
        for (;;) {
        }
    }
    deform->nodes = node =
        heap_calloc(DEFORM_NODES, sizeof(struct deform_node));
    if (node == NULL) {
        for (;;) {
        }
    }
    object = root;
    deform->count = 0;
    do {
        node->object = object;
        node->depth = depth;
        deform->count++;
        node++;
        if (object->child != NULL) {
            stack[depth] = object;
            object = object->child;
            depth++;
        } else {
            climb = TRUE;
            do {
                if (object->sibling != NULL) {
                    object = object->sibling;
                    climb = FALSE;
                } else if (depth > 0) {
                    depth--;
                    object = stack[depth];
                } else {
                    more = climb = FALSE;
                }
            } while (climb);
        }
    } while (more);
    deform_step(deform);
    return deform;
}

void deform_free(struct deform *deform)
{
    deform->mode = DEFORM_FREE;
    deform_step(deform);
}

void fun_8c0fce40(struct deform *deform)
{
    deform->unk_0c = 1.0F;
    deform->unk_10 = 0.98F;
    deform->unk_18 = 0;
    deform->unk_24 = 0x400;
    deform->unk_1c = 0;
    deform->unk_24 = 0x200;
    deform->mode = DEFORM_HOLD;
}

void deform_use_moved(struct deform *deform)
{
    struct deform_node *node;
    NJS_OBJECT *object;
    int i;

    for (i = 0, node = deform->nodes; i < deform->count; i++, node++) {
        object = node->object;
        if (object->model == NULL) {
            continue;
        }
        object->model->points = node->moved;
    }
}
