/* poses: a model tree at one frame of a motion, and the matrix of each node */
#include <ninja.h>
#include <sg_xpt.h>

#include "collision/collision.h"
#include "draw/pose.h"
#include "player/player.h"
#include "system/task.h"
#include "system/util.h"

#define POSE_NODES 128

/* the factors of a node's motion data, in order, and the most it can have,
 * as in NJS_MDATA5 */
#define FACTOR_POS 0
#define FACTOR_ANG 1
#define FACTOR_SCL 2
#define FACTORS_MAX 5

/* a walk of a model tree through a motion */
struct pose_walk {
    void *mdata; /* the next node's motion data */
    NJS_MATRIX *matrix; /* where the next node's matrix goes */
    Uint32 type;
    Uint32 frames; /* the motion's nbFrame */
    Float frame;
};

/* a node's three axes, a line each: x in red, y in green and z in blue */
static NJS_POINT3 axis_points[6] = {
#include "draw/pose/axis_points.inc"
};
static NJS_BGRA axis_colors[6] = {
#include "draw/pose/axis_colors.inc"
};
/* @hack type: axes, an array of one, whose address pose_draw_player's loop
 * keeps in R9 (MOV.L at 0x8c05b1f8, MOV R9,R4 at 0x8c05b214); as a struct
 * passed as &axes, the loop keeps njDrawLine3D in R9 instead and 29 of 143
 * words differ */
static NJS_POINT3COL axes[1] = {
#include "draw/pose/axes.inc"
};

/* the last pose: each node's matrix, in the order the walk met them */
static NJS_MATRIX pose_matrices[POSE_NODES];
static Uint32 pose_count; /* the nodes it kept */
static Uint32 pose_limit; /* the most nodes it may keep */

/* pose object at the walk's frame, keep its matrix, and pose the nodes
 * under it, then return its sibling. when the walk has kept pose_limit
 * nodes already, it poses nothing and returns null. a node's motion data
 * holds a factor for each bit of the type up to its highest. a factor
 * without keys takes the node's own value */
static NJS_OBJECT *pose_node(NJS_OBJECT *object, struct pose_walk *walk)
{
    Uint32 type;
    Uint32 bit;
    int factors;
    Uint32 frame;
    Float rate;
    NJS_MKEY_F *key;
    NJS_MKEY_F *next;
    Float *kpos;
    Float *npos;
    Angle *kang;
    Angle *nang;
    Float px;
    Float py;
    Float pz;
    Angle x;
    Angle y;
    Angle z;
    Angle dx;
    Angle dy;
    Angle dz;
    Angle ax;
    Angle ay;
    Angle az;
    NJS_OBJECT *child;

    if (++pose_count > pose_limit) {
        pose_count = pose_limit;
        return NULL;
    }
    type = walk->type;
    factors = 0;
    bit = 1;
    while (factors < FACTORS_MAX && type != 0) {
        factors++;
        type &= ~bit;
        bit <<= 1;
    }
    njPushMatrix(NULL);
    /* @hack spelling: a counter declared in each case, which SHC keeps in R9
     * with njRotateXYZ in R11 (MOV #0,R9 at 0x8c05ac54, MOV.L at 0x8c05ac3c);
     * declared once for the function, 389 of 582 words differ */
    switch (factors) {
    case 2: {
        NJS_MDATA2 *mdata;
        int i;

        mdata = walk->mdata;
        for (i = 0; i < 2; i++) {
            if (mdata->p[i] == NULL) {
                switch (i) {
                case FACTOR_POS:
                    njTranslateV(NULL, (NJS_VECTOR *)object->pos);
                    break;
                case FACTOR_ANG:
                    if (!(object->evalflags & NJD_EVAL_UNIT_ANG)) {
                        njRotateXYZ(NULL, object->ang[0], object->ang[1],
                                    object->ang[2]);
                    }
                    break;
                }
                continue;
            }
            /* find the keys either side of the frame. a key on the motion's
             * last frame, nbFrame - 1, runs on to its first key over one
             * frame */
            frame = walk->frame;
            key = mdata->p[i];
            next = key + 1;
            while (frame > key->keyframe && frame >= next->keyframe) {
                key++;
                next++;
            }
            if (key->keyframe == walk->frames - 1) {
                next = mdata->p[i];
                rate = walk->frame - key->keyframe;
            } else {
                rate = (walk->frame - key->keyframe) /
                       (Float)(next->keyframe - key->keyframe);
            }
            switch (i) {
            case FACTOR_POS:
                kpos = key->key;
                npos = next->key;
                px = *kpos + ((*npos - *kpos) * rate);
                kpos++;
                npos++;
                py = *kpos + ((*npos - *kpos) * rate);
                kpos++;
                npos++;
                pz = *kpos + ((*npos - *kpos) * rate);
                njTranslate(NULL, px, py, pz);
                break;
            case FACTOR_ANG:
                kang = ((NJS_MKEY_A *)key)->key;
                nang = ((NJS_MKEY_A *)next)->key;
                x = *kang++;
                y = *kang++;
                z = *kang;
                dx = angle_difference(x, *nang++);
                dy = angle_difference(y, *nang++);
                dz = angle_difference(z, *nang);
                /* @hack redundant: a read through y as a pointer, which SHC
                 * drops, but which keeps x, y and z at sp+8, sp+4 and sp+12
                 * (0x8c05adb6 to 0x8c05adbe); without it, 6 of 582 words
                 * differ */
                (void)((NJS_MKEY_A *)y)->keyframe;
                ax = x + (Angle)(dx * rate);
                ay = y + (Angle)(dy * rate);
                az = z + (Angle)(dz * rate);
                njRotateXYZ(NULL, ax, ay, az);
                break;
            }
        }
        if (!(object->evalflags & NJD_EVAL_UNIT_SCL)) {
            njScaleV(NULL, (NJS_VECTOR *)object->scl);
        }
        walk->mdata = mdata + 1;
        break;
    }
    case 3: {
        NJS_MDATA3 *mdata;
        int i;

        mdata = walk->mdata;
        for (i = 0; i < 3; i++) {
            if (mdata->p[i] == NULL) {
                switch (i) {
                case FACTOR_POS:
                    if (!(object->evalflags & NJD_EVAL_UNIT_POS)) {
                        njTranslateV(NULL, (NJS_VECTOR *)object->pos);
                    }
                    break;
                case FACTOR_ANG:
                    if (!(object->evalflags & NJD_EVAL_UNIT_ANG)) {
                        njRotateXYZ(NULL, object->ang[0], object->ang[1],
                                    object->ang[2]);
                    }
                    break;
                case FACTOR_SCL:
                    if (!(object->evalflags & NJD_EVAL_UNIT_SCL)) {
                        njScaleV(NULL, (NJS_VECTOR *)object->scl);
                    }
                    break;
                }
                continue;
            }
            frame = walk->frame;
            key = mdata->p[i];
            next = key + 1;
            while (frame > key->keyframe && frame >= next->keyframe) {
                key++;
                next++;
            }
            if (key->keyframe == walk->frames - 1) {
                next = mdata->p[i];
                rate = walk->frame - key->keyframe;
            } else {
                rate = (walk->frame - key->keyframe) /
                       (Float)(next->keyframe - key->keyframe);
            }
            switch (i) {
            case FACTOR_POS:
                kpos = key->key;
                npos = next->key;
                px = *kpos + ((*npos - *kpos) * rate);
                kpos++;
                npos++;
                py = *kpos + ((*npos - *kpos) * rate);
                kpos++;
                npos++;
                pz = *kpos + ((*npos - *kpos) * rate);
                njTranslate(NULL, px, py, pz);
                break;
            case FACTOR_ANG:
                kang = ((NJS_MKEY_A *)key)->key;
                nang = ((NJS_MKEY_A *)next)->key;
                x = *kang++;
                y = *kang++;
                z = *kang;
                dx = angle_difference(x, *nang++);
                dy = angle_difference(y, *nang++);
                dz = angle_difference(z, *nang);
                /* @hack redundant: the same read, which keeps x, y and z at
                 * sp+8, sp+4 and sp+12 in this case too (0x8c05b00a to
                 * 0x8c05b012); without it, 15 of 582 words differ */
                (void)((NJS_MKEY_A *)y)->keyframe;
                ax = x + (Angle)(dx * rate);
                ay = y + (Angle)(dy * rate);
                az = z + (Angle)(dz * rate);
                njRotateXYZ(NULL, ax, ay, az);
                break;
            case FACTOR_SCL:
                kpos = key->key;
                npos = next->key;
                px = *kpos + ((*npos - *kpos) * rate);
                kpos++;
                npos++;
                py = *kpos + ((*npos - *kpos) * rate);
                kpos++;
                npos++;
                pz = *kpos + ((*npos - *kpos) * rate);
                njScale(NULL, px, py, pz);
                break;
            }
        }
        mdata++;
        walk->mdata = mdata;
        break;
    }
    default:
        /* a motion with other than two or three factors hangs the game */
        for (;;) {
        }
    }
    njGetMatrix(walk->matrix);
    walk->matrix++;
    child = object->child;
    while (child != NULL) {
        child = pose_node(child, walk);
    }
    njPopMatrix(1);
    return object->sibling;
}

void pose_draw_player(struct task_work *work, struct player *player,
                      struct player_action *actions)
{
    struct pose_walk walk;
    Uint32 i;
    NJS_MATRIX *matrix;

    pose_count = 0;
    pose_limit = POSE_NODES;
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->collision->shapes->center);
    TURN(njRotateZ, work->ang.z);
    TURN(njRotateX, work->ang.x);
    TURN(njRotateY, NJM_DEG_ANG(180) - work->ang.y);
    walk.mdata = actions[player->anim.action].action->motion->mdata;
    walk.matrix = pose_matrices;
    walk.type = actions[player->anim.action].action->motion->type;
    walk.frames = actions[player->anim.action].action->motion->nbFrame;
    walk.frame = player->anim.frame;
    pose_node(actions[player->anim.action].action->object, &walk);
    njPopMatrix(1);
    for (i = 0, matrix = pose_matrices; i < pose_count && i < POSE_NODES;
         i++, matrix++) {
        njPushMatrix(NULL);
        njSetMatrix(NULL, matrix);
        njDrawLine3D(axes, 3, NJD_DRAW_NORMAL);
        njPopMatrix(1);
    }
}

Bool pose_draw_player_node(struct task_work *work, struct player *player,
                           struct player_action *actions, Uint32 node)
{
    struct pose_walk walk;

    pose_count = 0;
    /* @hack spelling: pose_limit assigned in the test, which loads POSE_NODES
     * into R4 and pose_limit's address into R5 (0x8c05b292, 0x8c05b294);
     * assigned first, the two trade and 14 of 134 words differ */
    if ((pose_limit = node + 1) > POSE_NODES) {
        pose_limit = POSE_NODES;
    }
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->collision->shapes->center);
    TURN(njRotateZ, work->ang.z);
    TURN(njRotateX, work->ang.x);
    TURN(njRotateY, NJM_DEG_ANG(180) - work->ang.y);
    walk.mdata = actions[player->anim.action].action->motion->mdata;
    walk.matrix = pose_matrices;
    walk.type = actions[player->anim.action].action->motion->type;
    walk.frames = actions[player->anim.action].action->motion->nbFrame;
    walk.frame = player->anim.frame;
    pose_node(actions[player->anim.action].action->object, &walk);
    njPopMatrix(1);
    njPushMatrix(NULL);
    njSetMatrix(NULL, &pose_matrices[node]);
    njDrawLine3D(axes, 3, NJD_DRAW_NORMAL);
    njPopMatrix(1);
    return TRUE;
}

Bool pose_action(NJS_MATRIX *base, NJS_ACTION *action, Float frame, Uint32 node,
                 NJS_MATRIX *out)
{
    struct pose_walk walk;

    pose_count = 0;
    /* @hack spelling: pose_limit assigned in the test, which loads POSE_NODES
     * into R4 and pose_limit's address into R5 (0x8c05b3f2, 0x8c05b3ee);
     * assigned first, the two trade and 16 of 70 words differ */
    if ((pose_limit = node + 1) > POSE_NODES) {
        pose_limit = POSE_NODES;
    }
    njPushMatrix(NULL);
    if (base != NULL) {
        njSetMatrix(NULL, base);
    }
    walk.mdata = action->motion->mdata;
    walk.matrix = pose_matrices;
    walk.type = action->motion->type;
    walk.frames = action->motion->nbFrame;
    walk.frame = frame;
    pose_node(action->object, &walk);
    if (out != NULL) {
        njSetMatrix(out, &pose_matrices[node]);
    }
    njPopMatrix(1);
    return TRUE;
}

Uint32 pose_get_count(void)
{
    return pose_count;
}

Bool pose_get_matrix(Uint32 node, NJS_MATRIX *out)
{
    if (node >= pose_count) {
        return FALSE;
    }
    njSetMatrix(out, &pose_matrices[node]);
    return TRUE;
}
