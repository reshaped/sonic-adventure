/* motions: a model playing a table of actions, blending each into the next */
#include <ninja.h>
#include <sg_xpt.h>

#include "draw/motion.h"
#include "draw/pose.h"
#include "player/player.h"
#include "system/task.h"
#include "system/util.h"

/* the room, in nodes, for a tree as tree_list_nodes lists it, and in the
 * blend's motion data */
#define TREE_NODES 128
#define BLEND_NODES 118

/* the factors in each node's motion data: positions and angles, and scales
 * too in a motion with NJD_MTYPE_SCL_2 */
#define FACTORS 2
#define SCALED_FACTORS 3

/* the blend's motion: two frames, from the old action's pose at the frame
 * to the new action's first key. it has scales when either action does.
 * each factor has a key at frame 0 and one at frame 1 */
#define BLEND_FRAMES 2
#define BLEND_KEYS 2
#define BLEND_TYPE (NJD_MTYPE_POS_0 | NJD_MTYPE_ANG_1)
#define BLEND_SCALED_TYPE (BLEND_TYPE | NJD_MTYPE_SCL_2)

/* narrow an angle to 16 bits, a half turn either way, in place */
/* @hack spelling: a bare scope with a local of its own. SHC then loads each
 * angle's address and value into R4 and R5 by turns (0x8c0198a2 to
 * 0x8c0198dc). a plain assignment keeps the address in R4 and puts the
 * value in R3 or R2, and 47 of 827 words differ */
#define SHORT_ANGLE(angle)                                                     \
    {                                                                          \
        Angle wide = (angle);                                                  \
        (angle) = (Sint16)wide;                                                \
    }

static void motion_follow(struct motion *motion, int index);
static void motion_enter(struct motion *motion);
static void motion_blend_start(struct task_work *work, struct motion *motion);
static void motion_blend_make(struct task_work *work, struct motion *motion);
static void motion_blend_free(struct task_work *work, struct motion *motion);

Sint8 motion_flags;
struct task_work *boss_work;
Sint8 boss_last_mode;
Sint8 boss_next_mode;
Sint8 boss_return_mode;
Sint32 boss_attack_checks;
Sint8 boss_repeats;
Sint8 boss_waiting;
Sint8 chaos_handed_over;
/* a byte no source reads; its type is unknown */
Uint8 dat_8c6c042b;
Sint8 chaos_current_form;
struct chaos *chaos_record;
struct chaos_form *chaos_params;
Float chaos_travel;
struct boss_point boss_points[BOSS_POINTS];
/* 4 bytes this file never reads; their type is unknown */
static Uint32 dat_8c6c08bc;
/* motion_weld's matrices: those of a weld's nodes node_a and node_b in the
 * pose, and their inverses. pose_action copies a matrix out to
 * motion_pose_matrix, and nothing reads it there */
static NJS_MATRIX motion_weld_matrix_a;
static NJS_MATRIX motion_weld_matrix_b;
static NJS_MATRIX motion_weld_inverse_a;
static NJS_MATRIX motion_weld_inverse_b;
static NJS_MATRIX motion_pose_matrix;

struct motion *motion_create(struct task_work *work, int size)
{
    struct motion *motion = heap_calloc(1, size);

    if (motion != NULL) {
        motion->state = MOTION_START;
        motion->weld_mode = PLAYER_WELD_MAKE;
    }
    work->flags |= 1;
    motion_flags = 0;
    return motion;
}

/* a change of entry blends from the playing action's pose into the new
 * action's first key, by the new entry's blend each step. a new entry whose
 * blend is 1, or whose action's object isn't the playing action's, is cut
 * to at once. once the blend ends, the new entry's mode plays its action */
void motion_step(struct task_work *work, struct motion *motion)
{
    NJS_ACTION *action;
    int index = motion->wanted;

    motion->previous = motion->current;
    if (motion->state != MOTION_FREE) {
        if (motion->current != motion->wanted &&
            motion->state != MOTION_BLENDING) {
            motion->state = MOTION_CHANGE;
            motion->steps = 0;
        } else {
            motion->steps++;
        }
    }
    switch (motion->state) {
    case MOTION_START:
        motion->frame = 0.0F;
        motion->current = index;
        motion->state = MOTION_CHANGE;
        motion->blend = heap_alloc(sizeof(NJS_ACTION));
        motion->blend->object = motion->actions[index].action->object;
        motion->blend->motion = heap_alloc(sizeof(NJS_MOTION));
        motion->blend->motion->nbFrame = BLEND_FRAMES;
        motion->blend->motion->type = BLEND_TYPE;
        motion->blend->motion->inp_fn = FACTORS;
        motion->blend->motion->mdata =
            heap_calloc(BLEND_NODES, sizeof(NJS_MDATA3));
        motion_flags = 0;
        break;
    case MOTION_CHANGE:
        if (motion->actions[index].blend != 1.0F &&
            motion->actions[index].action->object ==
                motion->actions[motion->current].action->object) {
            motion_blend_start(work, motion);
            motion->frame = 0.0F;
        } else {
            motion->frame = 1.0F;
        }
        motion->current = index;
        motion->state = MOTION_BLENDING;
        /* fall through */
    case MOTION_BLENDING:
        motion->frame += motion->actions[index].blend;
        if (motion->frame < 1.0F) {
            break;
        }
        /* NOLINTNEXTLINE(bugprone-signed-char-misuse) */
        motion->state = motion->actions[index].mode;
        motion_blend_free(work, motion);
        motion_enter(motion);
        break;
    case MOTION_LOOP:
        action = motion->actions[index].action;
        motion->frame += motion->actions[index].speed;
        if (motion->frame >= (Float)action->motion->nbFrame) {
            motion->frame -= (Float)action->motion->nbFrame;
            motion_flags |= MOTION_ENDED;
        }
        break;
    case MOTION_PACED:
        action = motion->actions[index].action;
        motion->frame += motion->speed * motion->actions[index].speed;
        /* @hack spelling: the second test's 0 on the left, which loads the
         * frame into FR3 and the 0 into FR2 (0x8c0191da, 0x8c0191dc); written
         * motion->frame < 0.0F, the two trade and 6 of 445 words differ */
        if (motion->frame >= (Float)action->motion->nbFrame) {
            motion->frame -= (Float)action->motion->nbFrame;
        } else if (0.0F > motion->frame) {
            motion->frame += (Float)action->motion->nbFrame;
        }
        break;
    case MOTION_AT:
        action = motion->actions[index].action;
        motion->frame = motion->pace * (Float)action->motion->nbFrame;
        break;
    case MOTION_STEP:
        action = motion->actions[index].action;
        motion->frame += motion->pace;
        if (motion->frame >= (Float)action->motion->nbFrame - 1.0F) {
            motion_follow(motion, index);
        }
        break;
    case MOTION_ONCE:
        action = motion->actions[index].action;
        motion->frame += motion->actions[index].speed;
        if (motion->frame >= (Float)action->motion->nbFrame - 1.0F) {
            motion_follow(motion, index);
            motion_flags |= MOTION_ENDED;
        }
        break;
    case MOTION_HOLD:
        action = motion->actions[index].action;
        motion->frame += motion->actions[index].speed;
        if (motion->frame >= (Float)action->motion->nbFrame - 1.0F) {
            motion->frame = (Float)action->motion->nbFrame - 1.0F;
            if (motion_flags & MOTION_HOLDING) {
                motion_flags &= ~MOTION_HOLDING;
            } else {
                motion_flags |= MOTION_HOLDING;
            }
        }
        break;
    case MOTION_STILL:
        break;
    case MOTION_BACKWARD:
        motion->frame -= motion->actions[index].speed;
        if (motion->frame < 0.0F) {
            motion_follow(motion, index);
            motion_flags |= MOTION_ENDED;
        }
        break;
    case MOTION_FREE:
        motion_blend_free(work, motion);
        heap_free(motion->blend->motion->mdata);
        heap_free(motion->blend->motion);
        heap_free(motion->blend);
        motion->blend = NULL;
        break;
    }
}

/* go on to entry index's next entry, and start its action in its mode */
static void motion_follow(struct motion *motion, int index)
{
    motion->wanted = motion->actions[index].next;
    motion->current = motion->wanted;
    /* NOLINTNEXTLINE(bugprone-signed-char-misuse) */
    motion->state = motion->actions[motion->current].mode;
    motion_enter(motion);
}

/* an action starts at its first frame, or its last when it plays
 * backward */
static void motion_enter(struct motion *motion)
{
    int index = motion->wanted;
    NJS_ACTION *action;

    switch (motion->state) {
    case MOTION_BACKWARD:
        action = motion->actions[index].action;
        motion->frame = (Float)action->motion->nbFrame - 1.0F;
        break;
    default:
        motion->frame = 0.0F;
        break;
    }
}

/* make the blend from the playing entry into the entry asked for */
static void motion_blend_start(struct task_work *work, struct motion *motion)
{
    NJS_MOTION *next;
    NJS_MOTION *prev;
    Uint16 next_factors;
    Uint16 prev_factors;
    Uint16 index = motion->wanted;
    Uint16 current = motion->current;

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
    /* @hack redundant: both branches of the test for scales call
     * motion_blend_make alike, but the image makes the test (CMP/EQ #3 at
     * 0x8c019470 and 0x8c019478); as one call, 41 of 41 words differ */
    if (next_factors == SCALED_FACTORS || prev_factors == SCALED_FACTORS) {
        motion_blend_make(work, motion);
    } else {
        motion_blend_make(work, motion);
    }
}

/* allocate two keys for each factor of each node in the blend. the first
 * key is the old action's value at the frame, between the keys either side
 * of it. past the old action's last key, it lies between the last and the
 * first. the second key is the new action's first. where one action has no
 * keys for a factor, its key takes the node's own value. the new action's
 * angles are the exception: with none, the node's angles overwrite the
 * first key instead. where neither action has keys for a factor, the blend
 * has none either */
static void motion_blend_make(struct task_work *work, struct motion *motion)
{
    NJS_OBJECT *nodes[TREE_NODES];
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
    Uint16 index = motion->wanted;
    Uint16 current = motion->current;

    object = motion->blend->object = motion->actions[index].action->object;
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
            NJS_MKEY_F *key = heap_alloc(BLEND_KEYS * sizeof(NJS_MKEY_F));

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
            blend_data->nb[0] = BLEND_KEYS;
        } else {
            blend_data->p[0] = NULL;
            blend_data->nb[0] = 0;
        }
        if (next_data->p[1] != NULL || prev_data->p[1] != NULL) {
            NJS_MKEY_A *key = heap_alloc(BLEND_KEYS * sizeof(NJS_MKEY_A));

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
                /* @bug logic: with no angle keys in the new action, the
                 * node's angles are stored through key, at R13, where they
                 * belong in key[1], at R13 + 16 (0x8c019898 to 0x8c0198a0).
                 * they overwrite the old action's angles in the first key,
                 * and the second key's angles keep what heap_alloc left
                 * there, which SHORT_ANGLE and angle_unwrap then read */
                key->key[0] = node->ang[0];
                key->key[1] = node->ang[1];
                key->key[2] = node->ang[2];
            }
            /* narrow both keys' angles to a half turn either way. angle_unwrap
             * then moves each of the second key's by whole turns, so the
             * blend turns the short way round from the first */
            SHORT_ANGLE(key->key[0]);
            SHORT_ANGLE(key->key[1]);
            SHORT_ANGLE(key->key[2]);
            SHORT_ANGLE(key[1].key[0]);
            SHORT_ANGLE(key[1].key[1]);
            SHORT_ANGLE(key[1].key[2]);
            key[1].key[0] = angle_unwrap(key->key[0], key[1].key[0]);
            key[1].key[1] = angle_unwrap(key->key[1], key[1].key[1]);
            key[1].key[2] = angle_unwrap(key->key[2], key[1].key[2]);
            blend_data->nb[1] = BLEND_KEYS;
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
                NJS_MKEY_F *key = heap_alloc(BLEND_KEYS * sizeof(NJS_MKEY_F));

                blend3->p[2] = key;
                key->keyframe = 0;
                if (prev_factors == SCALED_FACTORS && prev3->p[2] != NULL) {
                    /* @hack spelling: start declared before nb, which has end
                     * copy its 0 from start's R6 (MOV R6,R5 at 0x8c0199b0);
                     * declared after nb, end copies it from R14 and 1 of 827
                     * words differs */
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
                blend3->nb[2] = BLEND_KEYS;
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

/* free each key array in the blend, and set its pointer and its count to
 * 0. an NJS_MDATA2 or NJS_MDATA3 entry holds a key pointer for each factor,
 * p[], then a count for each, nb[]. data walks the entries a word at a time,
 * and finds a factor's count factors words past its keys */
static void motion_blend_free(struct task_work *work, struct motion *motion)
{
    NJS_MOTION *blend = motion->blend->motion;
    void **data = (void **)blend->mdata;
    Uint16 factors = blend->inp_fn;
    int i;

    for (i = motion->actions[motion->wanted].nodes; i > 0; i--) {
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

/* PLAYER_WELDING puts one side of each weld on the posed object back to
 * its copies. it then carries those through the two nodes' matrices onto
 * the other side's vertices */
void motion_weld(struct task_work *work, struct motion *motion)
{
    struct weld *weld = motion->welds;
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
     * declared as a Uint32[6]. the image's ADD #-96,R15 reserves them
     * (0x8c019cb0); without them, 2 of 543 words differ */
    Uint32 unused[6];

    switch (motion->weld_mode) {
    case PLAYER_WELD_MAKE:
        motion->weld_mode = PLAYER_WELDING;
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
        if (motion->state == MOTION_BLENDING) {
            pose_action(&_unit_matrix_, motion->blend, motion->frame,
                        motion->actions[motion->wanted].nodes,
                        &motion_pose_matrix);
        } else {
            pose_action(&_unit_matrix_, motion->actions[motion->wanted].action,
                        motion->frame, motion->actions[motion->wanted].nodes,
                        &motion_pose_matrix);
        }
        if (weld == NULL) {
            return;
        }
        for (; weld->base != NULL; weld++) {
            if (weld->base == motion->actions[motion->wanted].action->object) {
                pose_get_matrix(weld->node_a, &motion_weld_matrix_a);
                pose_get_matrix(weld->node_b, &motion_weld_matrix_b);
                points_a = weld->object_a->model->points;
                normals_a = weld->object_a->model->normals;
                points_b = weld->object_b->model->points;
                normals_b = weld->object_b->model->normals;
                njSetMatrix(&motion_weld_inverse_a, &motion_weld_matrix_a);
                njSetMatrix(&motion_weld_inverse_b, &motion_weld_matrix_b);
                njInvertMatrix(&motion_weld_inverse_a);
                njInvertMatrix(&motion_weld_inverse_b);
                /* WELD_BOTH takes WELD_TO_A's code: b's vertices follow a's */
                switch (weld->kind) {
                case WELD_BOTH:
                case WELD_TO_A:
                    njPushMatrix(&motion_weld_inverse_b);
                    njMultiMatrix(NULL, &motion_weld_matrix_a);
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
                    njPushMatrix(&motion_weld_inverse_a);
                    njMultiMatrix(NULL, &motion_weld_matrix_b);
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
            vertex = weld->vertices;
            model_a = weld->object_a->model;
            model_b = weld->object_b->model;
            for (i = 0; i < weld->count; i++) {
                model_a->points[weld->indices[i * 2]] = *vertex++;
                model_a->normals[weld->indices[i * 2]] = *vertex++;
                model_b->points[weld->indices[(i * 2) + 1]] = *vertex++;
                model_b->normals[weld->indices[(i * 2) + 1]] = *vertex++;
            }
            heap_free(weld->vertices);
        }
        break;
    }
}
