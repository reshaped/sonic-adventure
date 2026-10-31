/* a player's model posed by a motion the caller gives, with the player's
 * welds closing its seams. events pose players so, in place of their own
 * actions */
#include <ninja.h>
#include <sg_xpt.h>

#include "draw/pose.h"
#include "player/player.h"
#include "player/weld.h"

static NJS_MATRIX weld_matrix_a;
static NJS_MATRIX weld_matrix_b;
static NJS_MATRIX weld_inverse_a;
static NJS_MATRIX weld_inverse_b;

void player_weld(int player, NJS_OBJECT *object, NJS_MOTION *motion,
                 Float frame)
{
    struct player *record;
    struct player_anim *anim;
    struct weld *weld;
    NJS_POINT3 *vertex_a;
    NJS_POINT3 *vertex_b;
    NJS_ACTION action;
    NJS_POINT3 *points_a;
    NJS_POINT3 *points_b;
    NJS_VECTOR *normals_a;
    NJS_VECTOR *normals_b;
    NJS_POINT3 point;
    NJS_POINT3 other;
    /* @hack type: i as a Uint16, which SHC zero-extends (EXTU.W) for each
     * index and each loop test (0x8c04f1ae, 0x8c04f2ca); as an int, 125 of
     * 682 words differ */
    Uint16 i;
    int index_a;
    int index_b;

    record = player_get(player);
    anim = &record->anim;
    weld = anim->welds;
    action.object = object;
    action.motion = motion;
#ifdef GCC
    /* GCC can't see that a weld whose kind reads the points and normals has
     * the object_b that sets them, as in player_update_welds */
    points_a = points_b = normals_a = normals_b = NULL;
#endif
    switch (anim->weld_mode) {
    case PLAYER_WELDING:
        pose_action(&_unit_matrix_, &action, frame, anim->actions->nodes, NULL);
        for (; weld->base != NULL; weld++) {
            if (weld->base == object) {
                pose_get_matrix(weld->node_a, &weld_matrix_a);
                if (weld->object_b != NULL) {
                    points_a = weld->object_a->model->points;
                    normals_a = weld->object_a->model->normals;
                    njSetMatrix(&weld_inverse_a, &weld_matrix_a);
                    njInvertMatrix(&weld_inverse_a);
                    pose_get_matrix(weld->node_b, &weld_matrix_b);
                    points_b = weld->object_b->model->points;
                    normals_b = weld->object_b->model->normals;
                    njSetMatrix(&weld_inverse_b, &weld_matrix_b);
                    njInvertMatrix(&weld_inverse_b);
                } else {
                    /* @hack spelling: point's and other's zeros in two chains,
                     * which put 0.0F in FR13 (FLDI0 FR13 at 0x8c04f026) and
                     * lay the frame out as the image does (MOV #92,R0 at
                     * 0x8c04efbe); as six single stores, 0.0F takes FR15,
                     * the frame's slots move, and 44 of 682 words differ */
                    point.x = point.y = point.z = 0.0F;
                    other.x = other.y = other.z = 0.0F;
                    if (weld->count == 0) {
                        other.x = 1.0F;
                    } else if (weld->count == 1) {
                        other.y = 1.0F;
                    } else {
                        other.z = 1.0F;
                    }
                    njPushMatrix(&weld_matrix_a);
                }
                switch (weld->kind) {
                case WELD_BOTH:
                    for (i = 0; i < weld->count; i++) {
                        index_a = weld->indices[i * 2];
                        index_b = weld->indices[(i * 2) + 1];
                        /* @hack spelling: a's and b's points through vertex_a
                         * and vertex_b, which has SHC form vertex_b before the
                         * first call and keep it in the frame (MOV.L
                         * R2,@(R0,R15) at 0x8c04f1e4, R0 68). passed directly,
                         * the frame is 192 bytes, not 196, and 196 of 682
                         * words differ */
                        vertex_a = &weld->vertices[i * 4];
                        vertex_b = &weld->vertices[(i * 4) + 2];
                        njCalcPoint(&weld_matrix_a, vertex_a, &point);
                        njCalcPoint(&weld_matrix_b, vertex_b, &other);
                        point.x = (point.x + other.x) * 0.5F;
                        point.y = (point.y + other.y) * 0.5F;
                        point.z = (point.z + other.z) * 0.5F;
                        njCalcPoint(&weld_inverse_a, &point,
                                    &points_a[index_a]);
                        njCalcPoint(&weld_inverse_b, &point,
                                    &points_b[index_b]);
                        njCalcVector(&weld_matrix_a,
                                     &weld->vertices[(i * 4) + 1], &point);
                        njCalcVector(&weld_matrix_b,
                                     &weld->vertices[(i * 4) + 3], &other);
                        point.x += other.x;
                        point.y += other.y;
                        point.z += other.z;
                        njUnitVector(&point);
                        njCalcVector(&weld_inverse_a, &point,
                                     &normals_a[index_a]);
                        njCalcVector(&weld_inverse_b, &point,
                                     &normals_b[index_b]);
                    }
                    break;
                case WELD_TO_A:
                    njPushMatrix(&weld_inverse_b);
                    njMultiMatrix(NULL, &weld_matrix_a);
                    for (i = 0; i < weld->count; i++) {
                        index_a = weld->indices[i * 2];
                        index_b = weld->indices[(i * 2) + 1];
                        points_a[index_a] = weld->vertices[i * 4];
                        njCalcPoint(NULL, &weld->vertices[i * 4],
                                    &points_b[index_b]);
                        normals_a[index_a] = weld->vertices[(i * 4) + 1];
                        njCalcVector(NULL, &weld->vertices[(i * 4) + 1],
                                     &normals_b[index_b]);
                    }
                    njPopMatrix(1);
                    break;
                case WELD_TO_B:
                    njPushMatrix(&weld_inverse_a);
                    njMultiMatrix(NULL, &weld_matrix_b);
                    for (i = 0; i < weld->count; i++) {
                        index_a = weld->indices[i * 2];
                        index_b = weld->indices[(i * 2) + 1];
                        points_b[index_b] = weld->vertices[(i * 4) + 2];
                        njCalcPoint(NULL, &weld->vertices[(i * 4) + 2],
                                    &points_a[index_a]);
                        normals_b[index_b] = weld->vertices[(i * 4) + 3];
                        njCalcVector(NULL, &weld->vertices[(i * 4) + 3],
                                     &normals_a[index_a]);
                    }
                    njPopMatrix(1);
                    break;
                case WELD_MARK:
                    njCalcPoint(NULL, &point, &record->weld_point0);
                    njCalcVector(NULL, &other, &record->weld_axis0);
                    njPopMatrix(1);
                    break;
                case WELD_MARK + 1:
                    njCalcPoint(NULL, &point, &record->weld_point1);
                    njCalcVector(NULL, &other, &record->weld_axis1);
                    njPopMatrix(1);
                    break;
                case WELD_MARK + 2:
                    njCalcPoint(NULL, &point, &record->weld_point2);
                    njCalcVector(NULL, &other, &record->weld_axis2);
                    njPopMatrix(1);
                    break;
                case WELD_MARK + 3:
                    njCalcPoint(NULL, &point, &record->weld_point3);
                    njCalcVector(NULL, &other, &record->weld_axis3);
                    njPopMatrix(1);
                    break;
                case WELD_MARK + 4:
                    njCalcPoint(NULL, &point, &record->weld_point4);
                    njCalcVector(NULL, &other, &record->weld_axis4);
                    njPopMatrix(1);
                    break;
                case WELD_MARK + 5:
                    njCalcPoint(NULL, &point, &record->weld_point5);
                    njCalcVector(NULL, &other, &record->weld_axis5);
                    njPopMatrix(1);
                    break;
                }
            }
        }
        break;
    /* @hack redundant: case labels for PLAYER_WELD_MAKE and PLAYER_WELD_FREE,
     * which the default takes too, keep the image's CMP/EQ #0 and CMP/EQ #2
     * (0x8c04ef60, 0x8c04ef6c); without them, 639 of 682 words differ */
    case PLAYER_WELD_MAKE:
    case PLAYER_WELD_FREE:
    default:
        while (1) {
        }
    }
}
