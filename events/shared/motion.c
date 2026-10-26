/* event motions: a model playing a table of actions, blending from each
 * into the next. welds close the model's seams. event_actor_draw draws the
 * motions an event record queues, welded the same way */
#include <mathf.h>
#include <ninja.h>
#include <sg_xpt.h>

#include "draw/draw.h"
#include "draw/model.h"
#include "draw/motion.h"
#include "draw/pose.h"
#include "event/event.h"
#include "player/player.h"
#include "shared/motion.h"
#include "system/task.h"
#include "system/util.h"

/* the most nodes a blended model can have. the blend's motion data has
 * room for this many, and so has event_motion_blend_make's list of nodes */
#define BLEND_NODES 111

/* the factors in a node's motion data, as inp_fn counts them. they are
 * position and angle, and scale too in a motion with scales */
#define FACTORS 2
#define SCALED_FACTORS 3

/* a blend's motion: two frames, from the old action's pose to the new
 * action's first keys. it has scales when either action has them */
#define BLEND_FRAMES 2
#define BLEND_TYPE (NJD_MTYPE_POS_0 | NJD_MTYPE_ANG_1)
#define BLEND_SCALED_TYPE (BLEND_TYPE | NJD_MTYPE_SCL_2)

static void event_motion_enter(struct event_motion_control *motion);
static void event_motion_follow(struct event_motion_control *motion,
                                Uint16 index);
static void event_motion_blend_make(struct event_motion_control *motion);
static void event_motion_blend_start(struct event_motion_control *motion);
static void event_motion_blend_free(struct event_motion_control *motion);
static void event_motion_weld_pose(struct task_work *work, NJS_OBJECT *object,
                                   NJS_MOTION *motion, Float frame);

/* the matrices of a weld's two nodes, a and b, in the pose, and their
 * inverses */
static NJS_MATRIX event_motion_weld_matrix_a;
static NJS_MATRIX event_motion_weld_matrix_b;
static NJS_MATRIX event_motion_weld_inverse_a;
static NJS_MATRIX event_motion_weld_inverse_b;
/* the matrix pose_action copies out, which no code reads */
static NJS_MATRIX event_motion_pose_matrix;

/* start the action at frame 0, or a looping one at start. start then goes
 * back to 0 */
static void event_motion_enter(struct event_motion_control *motion)
{
    if (motion->state == EVENT_MOTION_LOOP) {
        /* @bug uninitialized: the first looping action starts at a frame the
         * heap left (FMOV.S @(R0,R4),FR3 at 0x0cb85bea in EV0007, R0 20). no
         * code writes start but the line below, and event_motion_alloc stores
         * only state and weld_mode (0x0cb86c2a, 0x0cb86c2c) */
        motion->frame = motion->start;
        motion->start = 0.0F;
    } else {
        motion->frame = 0.0F;
    }
}

/* cut to entry index's next, with no blend, and set EVENT_MOTION_ENDED */
static void event_motion_follow(struct event_motion_control *motion,
                                Uint16 index)
{
    motion->action = motion->actions[index].next;
    motion->current = motion->action;
    /* NOLINTNEXTLINE(bugprone-signed-char-misuse) */
    motion->state = motion->actions[motion->current].mode;
    motion->next = motion->actions[motion->action].next;
    motion->flags |= EVENT_MOTION_ENDED;
    event_motion_enter(motion);
}

/* fill the blend's motion data: two keys for each factor of each node. the
 * first is the old action's value at frame, found between the keys either
 * side of it. the second is the new action's first key. a factor either
 * action lacks takes the node's own value */
static void event_motion_blend_make(struct event_motion_control *motion)
{
    NJS_OBJECT *nodes[BLEND_NODES];
    NJS_OBJECT *object;
    NJS_OBJECT *node;
    NJS_MOTION *next;
    NJS_MOTION *prev;
    Uint16 next_factors;
    Uint16 prev_factors;
    NJS_MDATA2 *next_data;
    NJS_MDATA2 *prev_data;
    NJS_MDATA2 *blend_data;
    Uint32 i;
    Uint16 index = motion->action;
    Uint16 current = motion->current;

    object = motion->actions[index].action->object;
    motion->blend->object = object;
    tree_list_nodes(object, nodes);
    next = motion->actions[index].action->motion;
    prev = motion->actions[current].action->motion;
    if (next->type & NJD_MTYPE_SCL_2) {
        next_factors = SCALED_FACTORS;
    } else {
        next_factors = FACTORS;
    }
    if (prev->type & NJD_MTYPE_SCL_2) {
        prev_factors = SCALED_FACTORS;
    } else {
        prev_factors = FACTORS;
    }
    next_data = next->mdata;
    prev_data = prev->mdata;
    blend_data = motion->blend->motion->mdata;
    for (i = 0; i < motion->actions[index].nodes; i++) {
        node = nodes[i];
        if (next_data->p[0] != NULL || prev_data->p[0] != NULL) {
            NJS_MKEY_F *key = heap_alloc(2 * sizeof(NJS_MKEY_F));

            blend_data->p[0] = key;
            key->keyframe = 0;
            if (prev_data->p[0] != NULL) {
                NJS_MKEY_F *keys = prev_data->p[0];
                Uint32 nb = prev_data->nb[0];
                Uint32 start = 0;
                Uint32 end = 0;
                Uint32 span;
                Float along;

                while (motion->frame > (Float)keys[end].keyframe) {
                    if (end >= nb - 1) {
                        end = 0;
                        start = nb - 1;
                        break;
                    }
                    start = end;
                    end++;
                }
                key->key[0] = keys[start].key[0];
                key->key[1] = keys[start].key[1];
                key->key[2] = keys[start].key[2];
                if (end != start) {
                    span = keys[end].keyframe - keys[start].keyframe;
                    if (end < start) {
                        span += prev->nbFrame;
                    }
                    along = (motion->frame - (Float)keys[start].keyframe) /
                            (Float)span;
                    key->key[0] +=
                        (keys[end].key[0] - keys[start].key[0]) * along;
                    key->key[1] +=
                        (keys[end].key[1] - keys[start].key[1]) * along;
                    key->key[2] +=
                        (keys[end].key[2] - keys[start].key[2]) * along;
                }
            } else {
                key->key[0] = node->pos[0];
                key->key[1] = node->pos[1];
                key->key[2] = node->pos[2];
            }
            key[1].keyframe = 1;
            if (next_data->p[0] != NULL) {
                NJS_MKEY_F *keys = next_data->p[0];

                key[1].key[0] = keys->key[0];
                key[1].key[1] = keys->key[1];
                key[1].key[2] = keys->key[2];
            } else {
                key[1].key[0] = node->pos[0];
                key[1].key[1] = node->pos[1];
                key[1].key[2] = node->pos[2];
            }
            blend_data->nb[0] = 2;
        } else {
            blend_data->p[0] = NULL;
            blend_data->nb[0] = 0;
        }
        if (next_data->p[1] != NULL || prev_data->p[1] != NULL) {
            NJS_MKEY_A *key = heap_alloc(2 * sizeof(NJS_MKEY_A));

            blend_data->p[1] = key;
            key->keyframe = 0;
            if (prev_data->p[1] != NULL) {
                NJS_MKEY_A *keys = prev_data->p[1];
                Uint32 nb = prev_data->nb[1];
                Uint32 start = 0;
                Uint32 end = 0;
                Uint32 span;
                Float along;

                while (motion->frame > (Float)keys[end].keyframe) {
                    if (end >= nb - 1) {
                        end = 0;
                        start = nb - 1;
                        break;
                    }
                    start = end;
                    end++;
                }
                key->key[0] = keys[start].key[0];
                key->key[1] = keys[start].key[1];
                key->key[2] = keys[start].key[2];
                if (end != start) {
                    span = keys[end].keyframe - keys[start].keyframe;
                    if (end < start) {
                        span += prev->nbFrame;
                    }
                    along = (motion->frame - (Float)keys[start].keyframe) /
                            (Float)span;
                    key->key[0] +=
                        (Angle)((Float)angle_difference(keys[start].key[0],
                                                        keys[end].key[0]) *
                                along);
                    key->key[1] +=
                        (Angle)((Float)angle_difference(keys[start].key[1],
                                                        keys[end].key[1]) *
                                along);
                    key->key[2] +=
                        (Angle)((Float)angle_difference(keys[start].key[2],
                                                        keys[end].key[2]) *
                                along);
                }
            } else {
                key->key[0] = node->ang[0];
                key->key[1] = node->ang[1];
                key->key[2] = node->ang[2];
            }
            key[1].keyframe = 1;
            if (next_data->p[1] != NULL) {
                NJS_MKEY_A *keys = next_data->p[1];

                key[1].key[0] = keys->key[0];
                key[1].key[1] = keys->key[1];
                key[1].key[2] = keys->key[2];
            } else {
                key[1].key[0] = node->ang[0];
                key[1].key[1] = node->ang[1];
                key[1].key[2] = node->ang[2];
            }
            /* the second key's angles, moved by whole turns so the blend
             * turns the short way */
            key[1].key[0] = angle_unwrap(key->key[0], key[1].key[0]);
            key[1].key[1] = angle_unwrap(key->key[1], key[1].key[1]);
            key[1].key[2] = angle_unwrap(key->key[2], key[1].key[2]);
            blend_data->nb[1] = 2;
        } else {
            blend_data->p[1] = NULL;
            blend_data->nb[1] = 0;
        }
        if (prev_factors == SCALED_FACTORS || next_factors == SCALED_FACTORS) {
            NJS_MDATA3 *blend3;
            NJS_MDATA3 *next3;
            NJS_MDATA3 *prev3;

            next3 = (NJS_MDATA3 *)next_data;
            blend3 = (NJS_MDATA3 *)blend_data;
            prev3 = (NJS_MDATA3 *)prev_data;
            blend3->nb[1] = blend_data->nb[1];
            blend3->nb[0] = blend_data->nb[0];
            if ((next_factors == SCALED_FACTORS && next3->p[2] != NULL) ||
                (prev_factors == SCALED_FACTORS && prev3->p[2] != NULL)) {
                NJS_MKEY_F *key = heap_alloc(2 * sizeof(NJS_MKEY_F));

                blend3->p[2] = key;
                key->keyframe = 0;
                if (prev_factors == SCALED_FACTORS && prev3->p[2] != NULL) {
                    /* @hack spelling: start declared before nb, so end copies
                     * its 0 from start in R6 (MOV R6,R5 at 0x0cb860ac in
                     * EV0007). with nb declared first, 1 of 802 words
                     * differ */
                    NJS_MKEY_F *keys = prev3->p[2];
                    Uint32 start = 0;
                    Uint32 nb = prev3->nb[2];
                    Uint32 end = 0;
                    Uint32 span;
                    Float along;

                    while (motion->frame > (Float)keys[end].keyframe) {
                        if (end >= nb - 1) {
                            end = 0;
                            start = nb - 1;
                            break;
                        }
                        start = end;
                        end++;
                    }
                    key->key[0] = keys[start].key[0];
                    key->key[1] = keys[start].key[1];
                    key->key[2] = keys[start].key[2];
                    if (end != start) {
                        span = keys[end].keyframe - keys[start].keyframe;
                        if (end < start) {
                            span += prev->nbFrame;
                        }
                        along = (motion->frame - (Float)keys[start].keyframe) /
                                (Float)span;
                        key->key[0] +=
                            (keys[end].key[0] - keys[start].key[0]) * along;
                        key->key[1] +=
                            (keys[end].key[1] - keys[start].key[1]) * along;
                        key->key[2] +=
                            (keys[end].key[2] - keys[start].key[2]) * along;
                    }
                } else {
                    key->key[0] = node->scl[0];
                    key->key[1] = node->scl[1];
                    key->key[2] = node->scl[2];
                }
                key[1].keyframe = 1;
                if (next_factors == SCALED_FACTORS && next3->p[2] != NULL) {
                    NJS_MKEY_F *keys = next3->p[2];

                    key[1].key[0] = keys->key[0];
                    key[1].key[1] = keys->key[1];
                    key[1].key[2] = keys->key[2];
                } else {
                    key[1].key[0] = node->scl[0];
                    key[1].key[1] = node->scl[1];
                    key[1].key[2] = node->scl[2];
                }
                blend3->nb[2] = 2;
            } else {
                blend3->p[2] = NULL;
                blend3->nb[2] = 0;
            }
        }
        if (next_factors == SCALED_FACTORS || prev_factors == SCALED_FACTORS) {
            blend_data = (NJS_MDATA2 *)((NJS_MDATA3 *)blend_data + 1);
        } else {
            blend_data++;
        }
        if (next_factors == SCALED_FACTORS) {
            next_data = (NJS_MDATA2 *)((NJS_MDATA3 *)next_data + 1);
        } else {
            next_data++;
        }
        if (prev_factors == SCALED_FACTORS) {
            prev_data = (NJS_MDATA2 *)((NJS_MDATA3 *)prev_data + 1);
        } else {
            prev_data++;
        }
    }
    if (next_factors == SCALED_FACTORS || prev_factors == SCALED_FACTORS) {
        motion->blend->motion->type = BLEND_SCALED_TYPE;
        motion->blend->motion->inp_fn = SCALED_FACTORS;
    } else {
        motion->blend->motion->type = BLEND_TYPE;
        motion->blend->motion->inp_fn = FACTORS;
    }
}

/* make the blend from the old action's pose to the new one's first keys */
static void event_motion_blend_start(struct event_motion_control *motion)
{
    NJS_MOTION *next;
    NJS_MOTION *prev;
    Uint16 next_factors;
    Uint16 prev_factors;
    Uint16 index = motion->action;
    Uint16 current = motion->current;

    /* @hack redundant: the reads and the test below, whose two arms make the
     * blend alike. SHC keeps them (CMP/EQ #3 at 0x0cb862ea and 0x0cb862f2 in
     * EV0007). without them, SHC inlines the function into
     * event_motion_step, the object runs 88 bytes short, and 47 of 48 words
     * differ */
    next = motion->actions[index].action->motion;
    prev = motion->actions[current].action->motion;
    if (next->type & NJD_MTYPE_SCL_2) {
        next_factors = SCALED_FACTORS;
    } else {
        next_factors = FACTORS;
    }
    if (prev->type & NJD_MTYPE_SCL_2) {
        prev_factors = SCALED_FACTORS;
    } else {
        prev_factors = FACTORS;
    }
    if (next_factors == SCALED_FACTORS || prev_factors == SCALED_FACTORS) {
        event_motion_blend_make(motion);
    } else {
        event_motion_blend_make(motion);
    }
    /* @bug memory: makes the blend a second time. its new keys replace the
     * first make's, which no path frees (BSR at 0x0cb862f4 and the tail BRA
     * at 0x0cb862fc in EV0007, both to event_motion_blend_make) */
    event_motion_blend_make(motion);
}

/* free the blend's keys, and clear their pointers and counts. the walk
 * takes the motion data a word at a time. a factor's count lies as many
 * words past its pointer as the data has factors */
static void event_motion_blend_free(struct event_motion_control *motion)
{
    NJS_MOTION *blend = motion->blend->motion;
    void **data = (void **)blend->mdata;
    Uint16 factors = blend->inp_fn;
    int i;

    for (i = motion->actions[motion->action].nodes; i > 0; i--) {
        if (*data != NULL) {
            heap_free(*data);
            *data = NULL;
            data[factors] = NULL;
        }
        data++;
        if (*data != NULL) {
            heap_free(*data);
            *data = NULL;
            data[factors] = NULL;
        }
        data++;
        if (factors == SCALED_FACTORS) {
            if (*data != NULL) {
                heap_free(*data);
                *data = NULL;
                data[factors] = NULL;
            }
        }
        if (factors == SCALED_FACTORS) {
            data += 4;
        } else {
            data += 2;
        }
    }
}

void event_motion_step(struct event_motion_control *motion)
{
    Uint16 index = motion->action;
    NJS_ACTION *action;

    motion->previous = motion->current;
    motion->flags = 0;
    if (motion->current != motion->action &&
        motion->state != EVENT_MOTION_BLENDING &&
        motion->state != EVENT_MOTION_START) {
        motion->state = EVENT_MOTION_CHANGE;
        motion->steps = 0;
    } else {
        motion->steps++;
    }
    switch (motion->state) {
    case EVENT_MOTION_START:
        motion->frame = 0.0F;
        motion->current = index;
        motion->state = EVENT_MOTION_CHANGE;
        motion->blend = heap_alloc(sizeof(NJS_ACTION));
        motion->blend->object = motion->actions[index].action->object;
        motion->blend->motion = heap_alloc(sizeof(NJS_MOTION));
        motion->blend->motion->nbFrame = BLEND_FRAMES;
        motion->blend->motion->type = BLEND_TYPE;
        motion->blend->motion->inp_fn = FACTORS;
        motion->blend->motion->mdata =
            heap_calloc(BLEND_NODES, sizeof(NJS_MDATA3));
        break;
    case EVENT_MOTION_CHANGE:
        if (motion->actions[index].blend != 1.0F &&
            motion->actions[index].action->object ==
                motion->actions[motion->current].action->object) {
            event_motion_blend_start(motion);
            motion->frame = 0.0F;
        } else {
            motion->frame = 1.0F;
        }
        motion->current = index;
        motion->state = EVENT_MOTION_BLENDING;
        motion->next = motion->actions[index].next;
        break;
    case EVENT_MOTION_BLENDING:
        motion->frame += motion->actions[index].blend;
        if (motion->frame >= 1.0F) {
            /* NOLINTNEXTLINE(bugprone-signed-char-misuse) */
            motion->state = motion->actions[index].mode;
            event_motion_blend_free(motion);
            event_motion_enter(motion);
        }
        break;
    case EVENT_MOTION_LOOP:
        action = motion->actions[index].action;
        motion->frame += motion->actions[index].speed;
        if (motion->frame >= (Float)action->motion->nbFrame) {
            motion->frame = 0.0F;
        }
        break;
    case EVENT_MOTION_ONCE:
        action = motion->actions[index].action;
        /* @bug logic: subtracts speed from frame, where EVENT_MOTION_LOOP and
         * EVENT_MOTION_HOLD add it (FSUB at 0x0cb86530 in EV0007). from frame
         * 0, an action of two frames or more then reaches its end and the
         * entry's next only at a negative speed */
        motion->frame -= motion->actions[index].speed;
        if (motion->frame >= (Float)action->motion->nbFrame - 1.0F) {
            event_motion_follow(motion, index);
        }
        break;
    case EVENT_MOTION_HOLD:
        action = motion->actions[index].action;
        motion->frame += motion->actions[index].speed;
        if (motion->frame >= (Float)action->motion->nbFrame - 1.0F) {
            motion->frame = (Float)action->motion->nbFrame - 1.0F;
            motion->flags |= EVENT_MOTION_ENDED;
        }
        break;
    case EVENT_MOTION_FREE:
    default:
        event_motion_blend_free(motion);
        heap_free(motion->blend->motion->mdata);
        heap_free(motion->blend->motion);
        heap_free(motion->blend);
        motion->blend = NULL;
        break;
    }
}

void event_motion_weld(struct task_work *work,
                       struct event_motion_control *motion)
{
    /* @hack spelling: control, a second pointer to the motion, which SHC
     * keeps in R10 (MOV R5,R10 at 0x0cb8660e in EV0007); without it, the
     * function runs 8 bytes short and 415 of 739 words differ */
    struct event_motion_control *control = motion;
    struct weld *weld = control->welds;
    NJS_MODEL *model_a;
    NJS_MODEL *model_b;
    NJS_POINT3 *vertex;
    NJS_POINT3 *points_a;
    NJS_POINT3 *points_b;
    NJS_VECTOR *normals_a;
    NJS_VECTOR *normals_b;
    Uint16 i;
    int index_a;
    int index_b;
    NJS_POINT3 point;
    NJS_POINT3 other;
    NJS_POINT3 *vertex_b;

    switch (control->weld_mode) {
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
        control->weld_mode = PLAYER_WELDING;
        break;
    case PLAYER_WELDING:
        if (control->state == EVENT_MOTION_BLENDING) {
            pose_action(&_unit_matrix_, control->blend, control->frame,
                        control->actions[control->action].nodes,
                        &event_motion_pose_matrix);
        } else {
            pose_action(&_unit_matrix_,
                        control->actions[control->action].action,
                        control->frame, control->actions[control->action].nodes,
                        &event_motion_pose_matrix);
        }
        for (; weld->base != NULL; weld++) {
            if (weld->base ==
                control->actions[control->action].action->object) {
                pose_get_matrix(weld->node_a, &event_motion_weld_matrix_a);
                pose_get_matrix(weld->node_b, &event_motion_weld_matrix_b);
                points_a = weld->object_a->model->points;
                normals_a = weld->object_a->model->normals;
                points_b = weld->object_b->model->points;
                normals_b = weld->object_b->model->normals;
                njSetMatrix(&event_motion_weld_inverse_a,
                            &event_motion_weld_matrix_a);
                njSetMatrix(&event_motion_weld_inverse_b,
                            &event_motion_weld_matrix_b);
                njInvertMatrix(&event_motion_weld_inverse_a);
                njInvertMatrix(&event_motion_weld_inverse_b);
                switch (weld->kind) {
                case WELD_BOTH:
                    /* @bug logic: leaves b's vertices at their copied
                     * places. a's vertices go halfway to b's, and both take
                     * the normal halfway between theirs, but points_b takes
                     * other, not the halfway point: the njCalcPoint at
                     * 0x0cb86946 in EV0007 takes sp+108, where the one
                     * before it takes the halfway point at sp+120 */
                    for (i = 0; i < weld->count; i++) {
                        index_a = weld->indices[i * 2];
                        index_b = weld->indices[(i * 2) + 1];
                        vertex = &weld->vertices[i * 4];
                        vertex_b = &weld->vertices[(i * 4) + 2];
                        njCalcPoint(&event_motion_weld_matrix_a, vertex,
                                    &point);
                        njCalcPoint(&event_motion_weld_matrix_b, vertex_b,
                                    &other);
                        point.x = (point.x + other.x) * 0.5F;
                        point.y = (point.y + other.y) * 0.5F;
                        point.z = (point.z + other.z) * 0.5F;
                        njCalcPoint(&event_motion_weld_inverse_a, &point,
                                    &points_a[index_a]);
                        njCalcPoint(&event_motion_weld_inverse_b, &other,
                                    &points_b[index_b]);
                        njCalcVector(&event_motion_weld_matrix_a,
                                     &weld->vertices[(i * 4) + 1], &point);
                        njCalcVector(&event_motion_weld_matrix_b,
                                     &weld->vertices[(i * 4) + 3], &other);
                        point.x = point.x + other.x;
                        point.y = point.y + other.y;
                        point.z = point.z + other.z;
                        njUnitVector(&point);
                        njCalcVector(&event_motion_weld_inverse_a, &point,
                                     &normals_a[index_a]);
                        njCalcVector(&event_motion_weld_inverse_b, &point,
                                     &normals_b[index_b]);
                    }
                    break;
                case WELD_TO_A:
                    /* @bug logic: maps a's copied vertices and normals into
                     * b's by the product that maps b's space into a's, not
                     * a's into b's. that product, a's inverse times b's
                     * matrix, is the one WELD_TO_B needs (njPushMatrix and
                     * njMultiMatrix at 0x0cb869d4 and 0x0cb869de in EV0007) */
                    njPushMatrix(&event_motion_weld_inverse_a);
                    njMultiMatrix(NULL, &event_motion_weld_matrix_b);
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
                    /* @bug logic: maps b's copied normals into a's space as
                     * points, so they take the translation too (njCalcPoint
                     * on the current matrix at 0x0cb86b12 in EV0007). a's
                     * vertices take b's copied ones, mapped the same way */
                    njPushMatrix(&event_motion_weld_inverse_a);
                    njMultiMatrix(NULL, &event_motion_weld_matrix_b);
                    for (i = 0; i < weld->count; i++) {
                        index_a = weld->indices[i * 2];
                        index_b = weld->indices[(i * 2) + 1];
                        points_b[index_b] = weld->vertices[(i * 4) + 2];
                        njCalcPoint(NULL, &weld->vertices[(i * 4) + 2],
                                    &points_a[index_a]);
                        normals_b[index_b] = weld->vertices[(i * 4) + 3];
                        njCalcPoint(NULL, &weld->vertices[(i * 4) + 3],
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
            if (weld->vertices != NULL) {
                model_a = weld->object_a->model;
                model_b = weld->object_b->model;
                for (i = 0; i < weld->count; i++) {
                    index_a = weld->indices[i * 2];
                    index_b = weld->indices[(i * 2) + 1];
                    model_a->points[index_a] = weld->vertices[i * 4];
                    model_a->normals[index_a] = weld->vertices[(i * 4) + 1];
                    model_b->points[index_b] = weld->vertices[(i * 4) + 2];
                    model_b->normals[index_b] = weld->vertices[(i * 4) + 3];
                }
                heap_free(weld->vertices);
            }
        }
        break;
    }
}

void event_motion_alloc(struct task_work *work)
{
    struct event_motion_control *motion =
        heap_alloc(sizeof(struct event_motion_control));

    if (motion != NULL) {
        motion->state = EVENT_MOTION_START;
        motion->weld_mode = PLAYER_WELD_MAKE;
        EVENT_MOTION(work) = motion;
    }
}

void event_motion_free(struct task_work *work)
{
    struct event_motion_control *motion = EVENT_MOTION(work);
    struct event_motion_control *control;

    /* @hack spelling: control, a second pointer to the motion, which the
     * never-run store below keeps apart from it: SHC keeps the motion in R13
     * and control in R14 (MOV R13,R14 at 0x0cb86c48 in EV0007); without it,
     * the function runs 4 bytes short and 29 of 32 words differ */
    control = motion;
    /* @hack redundant: a never-run store of NULL to motion, which gives the
     * motion its second register: the image keeps the work in R12, the motion
     * in R13 and control in R14 (0x0cb86c40 to 0x0cb86c48 in EV0007); without
     * it, the work takes R13 and the motion R14 alone, the function saves one
     * register fewer, runs 4 bytes short and 29 of 32 words differ */
    if (0) {
        motion = NULL;
    }
    control->state = EVENT_MOTION_FREE;
    /* @bug memory: leaks the blend when action changed after the last step.
     * the step called here then stores EVENT_MOTION_CHANGE over
     * EVENT_MOTION_FREE (MOV.W R4,@R14 at 0x0cb863d4 in EV0007) and starts
     * the change, and nothing after it frees the blend (BSR at 0x0cb86c52) */
    if (control->blend != NULL) {
        event_motion_step(control);
    }
    control->weld_mode = PLAYER_WELD_FREE;
    if (control->welds != NULL) {
        event_motion_weld(work, motion);
    }
    heap_free(motion);
    EVENT_MOTION(work) = NULL;
}

/* pose object as motion has it at frame. the welds of the work's event
 * motion then close its seams. it does nothing while EVENT_MOTION(work) is
 * null, and at any weld_mode but PLAYER_WELDING it never returns */
static void event_motion_weld_pose(struct task_work *work, NJS_OBJECT *object,
                                   NJS_MOTION *motion, Float frame)
{
    struct event_motion_control *control = EVENT_MOTION(work);
    struct event_motion_control *checked;
    struct weld *weld;
    NJS_POINT3 *vertex_a;
    NJS_POINT3 *vertex_b;
    NJS_POINT3 *points_a;
    NJS_POINT3 *points_b;
    NJS_VECTOR *normals_a;
    NJS_VECTOR *normals_b;
    Uint16 i;
    int index_a;
    int index_b;
    NJS_ACTION action;
    NJS_POINT3 point;
    NJS_POINT3 other;

    if (control == NULL) {
        return;
    }
    /* @hack spelling: checked, a second pointer to the motion, which the
     * never-run allocation below keeps apart from control: SHC keeps it at
     * sp+24 (MOV.L R4,@(24,R15) at 0x0cb86ca8 in EV0007); without it, 202
     * of 507 words differ */
    checked = control;
    weld = checked->welds;
    action.object = object;
    action.motion = motion;
    switch (checked->weld_mode) {
    case PLAYER_WELDING:
        pose_action(&_unit_matrix_, &action, frame, checked->actions[0].nodes,
                    NULL);
        /* @hack redundant: a never-run allocation into checked, which gives the
         * motion its second place, sp+24, beside R14 (0x0cb86ca8, 0x0cb86ca6
         * in EV0007); without it, checked moves to sp+12, the weld matrices'
         * addresses take other slots and 166 of 507 words differ */
        if (0) {
            checked = heap_alloc(sizeof(*checked));
        }
        /* @hack redundant: a goto to the next statement, and its label, which
         * end the block before the weld walk. SHC then stores the weld
         * matrices' addresses to the frame as the image does, from
         * 0x0cb86ce8 in EV0007; without them, 35 of 507 words differ */
        goto weld;
    weld:
        while (weld->base != NULL) {
            if (weld->base ==
                checked->actions[checked->action].action->object) {
                pose_get_matrix(weld->node_a, &event_motion_weld_matrix_a);
                pose_get_matrix(weld->node_b, &event_motion_weld_matrix_b);
                points_a = weld->object_a->model->points;
                normals_a = weld->object_a->model->normals;
                points_b = weld->object_b->model->points;
                normals_b = weld->object_b->model->normals;
                njSetMatrix(&event_motion_weld_inverse_a,
                            &event_motion_weld_matrix_a);
                njSetMatrix(&event_motion_weld_inverse_b,
                            &event_motion_weld_matrix_b);
                njInvertMatrix(&event_motion_weld_inverse_a);
                njInvertMatrix(&event_motion_weld_inverse_b);
                switch (weld->kind) {
                case WELD_BOTH:
                    /* @bug logic: leaves b's vertices at their copied
                     * places, as event_motion_weld's WELD_BOTH does: the
                     * njCalcPoint at 0x0cb86eb6 in EV0007 takes sp+104, where
                     * the one before it takes the halfway point at sp+116 */
                    for (i = 0; i < weld->count; i++) {
                        Uint16 *pair = &weld->indices[i * 2];

                        index_a = pair[0];
                        index_b = pair[1];
                        vertex_a = &weld->vertices[i * 4];
                        vertex_b = &weld->vertices[(i * 4) + 2];
                        njCalcPoint(&event_motion_weld_matrix_a, vertex_a,
                                    &point);
                        njCalcPoint(&event_motion_weld_matrix_b, vertex_b,
                                    &other);
                        point.x = (point.x + other.x) * 0.5F;
                        point.y = (point.y + other.y) * 0.5F;
                        point.z = (point.z + other.z) * 0.5F;
                        njCalcPoint(&event_motion_weld_inverse_a, &point,
                                    &points_a[index_a]);
                        njCalcPoint(&event_motion_weld_inverse_b, &other,
                                    &points_b[index_b]);
                        njCalcVector(&event_motion_weld_matrix_a,
                                     &weld->vertices[(i * 4) + 1], &point);
                        njCalcVector(&event_motion_weld_matrix_b,
                                     &weld->vertices[(i * 4) + 3], &other);
                        point.x += other.x;
                        point.y += other.y;
                        point.z += other.z;
                        njUnitVector(&point);
                        njCalcVector(&event_motion_weld_inverse_a, &point,
                                     &normals_a[index_a]);
                        njCalcVector(&event_motion_weld_inverse_b, &point,
                                     &normals_b[index_b]);
                    }
                    break;
                case WELD_TO_A:
                    /* @bug logic: maps a's copied vertices and normals into
                     * b's by the product that maps b's space into a's, as
                     * event_motion_weld's WELD_TO_A does (njPushMatrix and
                     * njMultiMatrix at 0x0cb86f3e and 0x0cb86f48 in EV0007) */
                    njPushMatrix(&event_motion_weld_inverse_a);
                    njMultiMatrix(NULL, &event_motion_weld_matrix_b);
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
                    /* @bug logic: maps b's copied normals into a's space as
                     * points, so they take the translation too, as
                     * event_motion_weld's WELD_TO_B does (njCalcPoint on the
                     * current matrix at 0x0cb87072 in EV0007) */
                    njPushMatrix(&event_motion_weld_inverse_a);
                    njMultiMatrix(NULL, &event_motion_weld_matrix_b);
                    for (i = 0; i < weld->count; i++) {
                        index_a = weld->indices[i * 2];
                        index_b = weld->indices[(i * 2) + 1];
                        points_b[index_b] = weld->vertices[(i * 4) + 2];
                        njCalcPoint(NULL, &weld->vertices[(i * 4) + 2],
                                    &points_a[index_a]);
                        normals_b[index_b] = weld->vertices[(i * 4) + 3];
                        njCalcPoint(NULL, &weld->vertices[(i * 4) + 3],
                                    &normals_a[index_a]);
                    }
                    njPopMatrix(1);
                    break;
                }
            }
            weld++;
        }
        /* @hack redundant: a never-run pop of the matrix, which keeps
         * event_motion_weld_matrix_a's address at sp+56 and
         * event_motion_weld_matrix_b's at sp+48 (0x0cb86cee, 0x0cb86cf2 in
         * EV0007), as the image has them; without it, the two trade and 4 of
         * 507 words differ */
        if (0) {
            njPopMatrix(0);
        }
        break;
    case PLAYER_WELD_MAKE:
    case PLAYER_WELD_FREE:
    default:
        for (;;) {
        }
        break;
    }
}

void event_actor_draw(struct task_work *work)
{
    struct event_work *event = work->event;
    struct event_motion *entry;
    struct event_motion *previous;
    NJS_MOTION_LINK link;
    NJS_ACTION_LINK action;
    Float frames;
    Float frame;

    if (event == NULL) {
        return;
    }
    entry = event->motions;
    previous = event->previous;
    draw_set_palette(DRAW_PLAYER_PALETTE);
    if (entry != NULL) {
        frames = entry->action.motion->nbFrame;
        frame = event->motion_frame;
        if (entry->texlist != NULL) {
            njSetTexture(entry->texlist);
        }
        if (previous != NULL && entry->blend) {
            action.motionlink = &link;
            action.object = entry->action.object;
            link.motion[0] = previous->action.motion;
            link.motion[1] = entry->action.motion;
            if (previous->flags & EVENT_REPEAT) {
                link.frame[0] = 0.0F;
            } else {
                link.frame[0] = previous->action.motion->nbFrame - 1;
            }
            link.frame[1] = 0.0F;
            if (floorf(frame) * 2.0F < entry->blend) {
                event_motion_weld_pose(work, entry->action.object,
                                       link.motion[0], link.frame[0]);
            } else {
                event_motion_weld_pose(work, entry->action.object,
                                       link.motion[1], link.frame[1]);
            }
            draw_action_link(&action, (frame + 1.0F) / (entry->blend + 1));
            frame += 1.0F;
            if (floorf(frame) >= entry->blend) {
                heap_free(previous);
                event->previous = NULL;
                frame = 0.0F;
            }
        } else {
            event_motion_weld_pose(work, entry->action.object,
                                   entry->action.motion, event->motion_frame);
            draw_action(&entry->action, event->motion_frame);
            frame += entry->speed;
            if (entry->flags & EVENT_REPEAT) {
                if (frame >= frames) {
                    if (entry->next != NULL) {
                        goto ended;
                    }
                    frame -= frames;
                }
            } else if (frame >= frames - 1.0F) {
            ended:
                event->previous = entry;
                frame = 0.0F;
                event->motions = entry->next;
            }
        }
        event->motion_frame = frame;
    }
    draw_set_palette(0);
}
