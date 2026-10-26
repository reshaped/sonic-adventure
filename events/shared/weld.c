/* welds for the events: the seams of a model that an event poses itself,
 * closed as the model is posed */
#include <ninja.h>
#include <sg_xpt.h>

#include "draw/pose.h"
#include "player/player.h"
#include "shared/weld.h"
#include "system/task.h"
#include "system/util.h"

/* a weld's two nodes' matrices in the pose, and their inverses */
static NJS_MATRIX event_weld_matrix_a;
static NJS_MATRIX event_weld_matrix_b;
static NJS_MATRIX event_weld_inverse_a;
static NJS_MATRIX event_weld_inverse_b;
/* the matrix pose_action copies out, which nothing reads */
static NJS_MATRIX event_weld_pose_matrix;

void event_weld(struct weld *welds, NJS_ACTION *action, Float frame, int node,
                int mode)
{
    struct weld *weld = welds;
    NJS_MODEL *model_a;
    NJS_MODEL *model_b;
    NJS_POINT3 *vertex;
    NJS_POINT3 *points_a;
    NJS_POINT3 *points_b;
    NJS_VECTOR *normals_a;
    NJS_VECTOR *normals_b;
    int i;
    int index_a;
    int index_b;
    /* @hack redundant: 24 bytes of frame that no code reads, of a type unknown,
     * declared as two points. the image's ADD #-92,R15 reserves them in its
     * 124-byte frame (0x0cb83da0 in EV0008); without them, 2 of 416 words
     * differ */
    NJS_POINT3 point;
    NJS_POINT3 other;

    switch (mode) {
    case PLAYER_WELD_MAKE:
        if (weld == NULL) {
            return;
        }
        for (; weld->base != NULL; weld++) {
            weld->vertices = heap_alloc(weld->count * (4 * sizeof(NJS_POINT3)));
            vertex = weld->vertices;
            model_a = weld->object_a->model;
            model_b = weld->object_b->model;
            for (i = 0; i < weld->count; i++) {
                *vertex++ = model_a->points[weld->indices[i * 2]];
                *vertex++ = model_a->normals[weld->indices[i * 2]];
                *vertex++ = model_b->points[weld->indices[(i * 2) + 1]];
                *vertex++ = model_b->normals[weld->indices[(i * 2) + 1]];
            }
            weld->node_a = tree_node_index(weld->base, weld->object_a);
            weld->node_b = tree_node_index(weld->base, weld->object_b);
        }
        break;
    case PLAYER_WELDING:
        pose_action(&_unit_matrix_, action, frame, node,
                    &event_weld_pose_matrix);
        if (weld == NULL) {
            return;
        }
        for (; weld->base != NULL; weld++) {
            if (weld->base == action->object) {
                pose_get_matrix(weld->node_a, &event_weld_matrix_a);
                pose_get_matrix(weld->node_b, &event_weld_matrix_b);
                points_a = weld->object_a->model->points;
                normals_a = weld->object_a->model->normals;
                points_b = weld->object_b->model->points;
                normals_b = weld->object_b->model->normals;
                njSetMatrix(&event_weld_inverse_a, &event_weld_matrix_a);
                njSetMatrix(&event_weld_inverse_b, &event_weld_matrix_b);
                njInvertMatrix(&event_weld_inverse_a);
                njInvertMatrix(&event_weld_inverse_b);
                /* WELD_BOTH moves b's vertices onto a's, as WELD_TO_A
                 * does */
                switch (weld->kind) {
                case WELD_BOTH:
                case WELD_TO_A:
                    njPushMatrix(&event_weld_inverse_b);
                    njMultiMatrix(NULL, &event_weld_matrix_a);
                    for (i = 0; i < weld->count; i++) {
                        index_a = weld->indices[i * 2];
                        index_b = weld->indices[(i * 2) + 1];
                        points_a[index_a] = weld->vertices[i * 4];
                        njCalcPoint(NULL, &points_a[index_a],
                                    &points_b[index_b]);
                        normals_a[index_a] = weld->vertices[(i * 4) + 1];
                        njCalcVector(NULL, &normals_a[index_a],
                                     &normals_b[index_b]);
                    }
                    njPopMatrix(1);
                    break;
                case WELD_TO_B:
                    njPushMatrix(&event_weld_inverse_a);
                    njMultiMatrix(NULL, &event_weld_matrix_b);
                    for (i = 0; i < weld->count; i++) {
                        index_a = weld->indices[i * 2];
                        index_b = weld->indices[(i * 2) + 1];
                        points_b[index_b] = weld->vertices[(i * 4) + 2];
                        njCalcPoint(NULL, &points_b[index_b],
                                    &points_a[index_a]);
                        normals_b[index_b] = weld->vertices[(i * 4) + 3];
                        njCalcVector(NULL, &normals_b[index_b],
                                     &normals_a[index_a]);
                    }
                    njPopMatrix(1);
                    break;
                }
            }
        }
        break;
    case PLAYER_WELD_FREE:
    default:
        for (; weld->base != NULL; weld++) {
            heap_free(weld->vertices);
        }
        break;
    }
}
