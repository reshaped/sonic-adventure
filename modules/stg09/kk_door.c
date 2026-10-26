/* O KKDOOR paired doors, whose collision follows their motion */
#include <ninja.h>
#include <sg_xpt.h>

#include "collision/collision.h"
#include "debug/editor.h"
#include "draw/draw.h"
#include "draw/pose.h"
#include "draw/tree.h"
#include "object/placement.h"
#include "player/player.h"
#include "sound/sound.h"
#include "stg09/casino.h"
#include "stg09/kk_door.h"
#include "system/task.h"
#include "system/util.h"

extern NJS_OBJECT kk_door_object;
extern NJS_MOTION kk_door_motion;
extern NJS_ACTION kk_door_action;

/* destroyed beyond about 600 units of the camera and player 0 */
#define RANGE ((600.0F * 600.0F) + 10.0F)

enum kk_door_mode { DOOR_SHUT, DOOR_OPENING, DOOR_OPEN, DOOR_CLOSING };

/* the action frames the doors move a step */
static Float kk_door_frame_rate = 0.15F;
/* the shut doors open once the nearest player comes within
 * kk_door_open_distance. the open doors shut once the nearest player is
 * beyond kk_door_close_distance */
static Float kk_door_open_distance = 70.0F;
static Float kk_door_close_distance = 80.0F;

/* the doors' boxes. while the doors move, kk_door_run puts each at its door's
 * posed point plus the center here */
static struct collision_shape kk_door_shapes[2] = {
    {SHAPE_RECTANGLE | SHAPE_HURTABLE,
     0,
     PUSH_ALL | PUSH_FIRM(PUSH_ALL),
     {0.0F, 34.0F, 15.5F},
     1.5F,
     34.0F,
     16.0F,
     {0},
     0,
     NULL},
    {SHAPE_RECTANGLE | SHAPE_HURTABLE,
     0,
     PUSH_ALL | PUSH_FIRM(PUSH_ALL),
     {0.0F, 34.0F, -15.5F},
     1.5F,
     34.0F,
     16.0F,
     {0},
     0,
     NULL},
};

static Bool kk_door_pose(Float frame, NJS_POINT3 *first, NJS_POINT3 *second);
static void kk_door_init(struct task *task);
static void kk_door_destroy(struct task *task);
static void kk_door_run(struct task *task);
static void kk_door_draw(struct task *task);

/* pose the action at frame. set first to kk_door_object.pos as node 0's
 * matrix moves it. set second to its child's pos as node 1's matrix moves it.
 * both are in the model's space. returns TRUE, as pose_action always does */
static Bool kk_door_pose(Float frame, NJS_POINT3 *first, NJS_POINT3 *second)
{
    /* Ninja needs a matrix on an 8-byte boundary, and the stack is aligned
     * only to 4. matrix points at the boundary at or below shifted.matrix's
     * start. storage keeps that boundary, and a whole matrix after it, inside
     * the struct */
    struct {
        NJS_MATRIX *matrix;
        union {
            NJS_MATRIX matrix;
            struct {
                Float alignment;
                NJS_MATRIX matrix;
            } shifted;
        } storage;
    } aligned;

    aligned.matrix =
        (NJS_MATRIX *)((Uint32)&aligned.storage.shifted.matrix & ~7);
    njPushMatrix(&_unit_matrix_);
    if (pose_action(NULL, &kk_door_action, frame, 0, aligned.matrix)) {
        njCalcPoint(aligned.matrix, (NJS_POINT3 *)kk_door_object.pos, first);
        njUnitMatrix(NULL);
        if (pose_action(NULL, &kk_door_action, frame, 1, aligned.matrix)) {
            njCalcPoint(aligned.matrix, (NJS_POINT3 *)kk_door_object.child->pos,
                        second);
            njPopMatrix(1);
            return TRUE;
        }
        njPopMatrix(1);
        return FALSE;
    }
    njPopMatrix(1);
    return FALSE;
}

void kk_door_step(struct task *task)
{
    /* @hack spelling: a copy of task, which keeps task in R14 across the
     * range check (MOV R4,R14 at 0x0c90eee6). without it, SHC keeps task on the
     * stack, the step runs 2 bytes longer, and 12 of its 16 words differ */
    struct task *current = task;

    if (placement_try_leave_by(current, RANGE)) {
        return;
    }
    kk_door_init(current);
}

static void kk_door_init(struct task *task)
{
    struct task_work *work = task->work;

    if (!placement_editing(task)) {
        work->ang.x = 0;
        work->ang.z = 0;
        collision_init(task, kk_door_shapes, COLLISION_OBJECT, 1, 2);
        work->collision->shapes->levels =
            (work->collision->shapes->levels & ~SHAPE_GUARD) | GUARD(1);
    }
    task->draw = kk_door_draw;
    task->step = kk_door_run;
    task->destructor = kk_door_destroy;
    work->timer = 0;
    work->mode = DOOR_SHUT;
}

static void kk_door_destroy(struct task *task)
{
}

static void kk_door_run(struct task *task)
{
    struct task_work *work = task->work;
    struct collision_work *collision;
    NJS_POINT3 first;
    NJS_POINT3 second;
    struct task_work *player;
    int nearest;
    Float frame;
    /* @hack spelling: byte cursors over the three points of each center's sum,
     * stepped by sizeof(Float), which SHC loads with FMOV.S @Rm+ (as at
     * 0x0c90f136). as member sums, the function runs 64 bytes longer, and 155
     * of its 271 words differ */
    unsigned char *destination;
    unsigned char *source;
    unsigned char *offset;

    if (placement_try_leave_by(task, RANGE)) {
        return;
    }
    kk_door_draw(task);
    if (placement_editing(task)) {
        return;
    }
    switch (work->mode) {
    case DOOR_SHUT:
        /* @hack spelling: the nearest player's number through a local. SHC
         * then saves only R12 to R14 (0x0c90efc0 to 0x0c90efc4). indexed by
         * the call, as DOOR_OPEN indexes it, SHC saves R11 too, and 154 of
         * kk_door_run's 271 words differ */
        nearest = player_nearest(&work->pos);
        player = player_work[nearest];
        if (njDistanceP2P(&player->pos, &work->pos) < kk_door_open_distance) {
            sound_play(SE_DOOR, 0, 0, 0);
            work->mode = DOOR_OPENING;
            work->timer = 0;
        }
        break;
    case DOOR_OPEN:
        player = player_work[player_nearest(&work->pos)];
        if (njDistanceP2P(&player->pos, &work->pos) > kk_door_close_distance) {
            sound_play(SE_DOOR, 0, 0, 0);
            work->mode = DOOR_CLOSING;
            work->timer = 0;
        }
        break;
    case DOOR_OPENING:
        frame = (Uint16)work->timer * kk_door_frame_rate;
        if (frame >= kk_door_motion.nbFrame - 1.0F) {
            work->mode = DOOR_OPEN;
            work->timer = 0;
        } else {
            collision = work->collision;
            if (kk_door_pose(frame, &first, &second)) {
                *(Float *)(destination =
                               (unsigned char *)&collision->shapes[0].center) =
                    *(Float *)(source = (unsigned char *)&first) +
                    *(Float *)(offset =
                                   (unsigned char *)&kk_door_shapes[0].center);
                *(Float *)(destination += sizeof(Float)) =
                    *(Float *)(source += sizeof(Float)) +
                    *(Float *)(offset += sizeof(Float));
                destination += sizeof(Float);
                source += sizeof(Float);
                offset += sizeof(Float);
                *(Float *)destination = *(Float *)source + *(Float *)offset;

                *(Float *)(destination =
                               (unsigned char *)&collision->shapes[1].center) =
                    *(Float *)(source = (unsigned char *)&second) +
                    *(Float *)(offset =
                                   (unsigned char *)&kk_door_shapes[1].center);
                /* @hack spelling: destination's and source's advances to y as
                 * statements of their own, which gives offset R6 (MOV R12,R6
                 * at 0x0c90f158). advanced in the sum, as the first door's
                 * are, offset takes R5, and 31 of kk_door_run's 271 words
                 * differ */
                destination += sizeof(Float);
                source += sizeof(Float);
                *(Float *)destination =
                    *(Float *)source + *(Float *)(offset += sizeof(Float));
                destination += sizeof(Float);
                source += sizeof(Float);
                offset += sizeof(Float);
                *(Float *)destination = *(Float *)source + *(Float *)offset;
            }
        }
        break;
    case DOOR_CLOSING:
        frame = (Uint16)work->timer * kk_door_frame_rate;
        if (frame >= kk_door_motion.nbFrame - 1.0F) {
            work->mode = DOOR_SHUT;
            work->timer = 0;
        } else {
            collision = work->collision;
            frame = kk_door_motion.nbFrame - frame - 1.0F;
            if (kk_door_pose(frame, &first, &second)) {
                /* @hack spelling: destination and source set as statements of
                 * their own. the second door's sums below then take offset in
                 * R6 and destination in R5 (0x0c90f1f8, 0x0c90f208). set in
                 * the sum, as the opening's are, the two trade, and 13 of
                 * kk_door_run's 271 words differ */
                destination = (unsigned char *)&collision->shapes[0].center;
                source = (unsigned char *)&first;
                *(Float *)destination =
                    *(Float *)source +
                    *(Float *)(offset =
                                   (unsigned char *)&kk_door_shapes[0].center);
                *(Float *)(destination += sizeof(Float)) =
                    *(Float *)(source += sizeof(Float)) +
                    *(Float *)(offset += sizeof(Float));
                destination += sizeof(Float);
                source += sizeof(Float);
                offset += sizeof(Float);
                *(Float *)destination = *(Float *)source + *(Float *)offset;

                *(Float *)(destination =
                               (unsigned char *)&collision->shapes[1].center) =
                    *(Float *)(source = (unsigned char *)&second) +
                    *(Float *)(offset =
                                   (unsigned char *)&kk_door_shapes[1].center);
                /* @hack spelling: source's advance to y as a statement of its
                 * own. the opening's second door then takes source in R4 and
                 * destination in R5 (0x0c90f150, 0x0c90f160). advanced in the
                 * sum, as the first door's is, the two trade, and 23 of
                 * kk_door_run's 271 words differ */
                source += sizeof(Float);
                *(Float *)(destination += sizeof(Float)) =
                    *(Float *)source + *(Float *)(offset += sizeof(Float));
                destination += sizeof(Float);
                source += sizeof(Float);
                offset += sizeof(Float);
                *(Float *)destination = *(Float *)source + *(Float *)offset;
            }
        }
        break;
    /* @hack redundant: default: break;, which gives collision_register's
     * address R3 (0x0c90f224). without it, the address takes R2, and 2 of
     * kk_door_run's 271 words differ */
    default:
        break;
    }
    collision_register(work);
    /* @hack spelling: the clear written out, which keeps the opening's second
     * source in R4 and destination in R5 (0x0c90f150, 0x0c90f160). with &=,
     * the two trade, and 41 of kk_door_run's 271 words differ */
    work->flags = work->flags & ~COLLISION_DAMAGED;
    /* @hack spelling: the test for the wrap to 0, written <= 0 for the image's
     * CMP/PL (0x0c90f23a). the cast makes the value tested a Uint16, so
     * <= 0 holds only at 0. the decrement undoes the wrap, which holds the
     * timer at 65535. as == 0, the test is TST, and 2 of kk_door_run's 271
     * words differ */
    if ((Uint16)++work->timer <= 0) {
        /* @hack spelling: the decrement written out, which keeps the opening's
         * second source in R4 and destination in R5 (0x0c90f150, 0x0c90f160).
         * as --work->timer, the two trade, and 41 of kk_door_run's 271 words
         * differ */
        work->timer = work->timer - 1;
    }
}

static void kk_door_draw(struct task *task)
{
    struct task_work *work = task->work;
    NJS_OBJECT *object = &kk_door_object;
    NJS_MOTION *motion = &kk_door_motion;
    Angle angle;
    Float frame;

    if (placement_try_leave_by(task, RANGE)) {
        return;
    }
    casino_set_texture(CASINO_TEXTURES_ACTS_12);
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    TURN_VIA(njRotateY, work->ang.y, angle);
    switch (work->mode) {
    case DOOR_OPEN:
        draw_motion(object, motion, motion->nbFrame - 1.0F);
        break;
    case DOOR_SHUT:
        draw_tree(object);
        break;
    case DOOR_OPENING:
        frame = (Uint16)work->timer * kk_door_frame_rate;
        if (frame >= motion->nbFrame - 1.0F) {
            frame = motion->nbFrame - 1.0F;
        }
        draw_motion(object, motion, frame);
        break;
    case DOOR_CLOSING:
        frame =
            motion->nbFrame - ((Uint16)work->timer * kk_door_frame_rate) - 1.0F;
        if (frame < 0.0F) {
            frame = 0.0F;
        }
        draw_motion(object, motion, frame);
        break;
    /* @hack redundant: default: break;, which gives njPopMatrix's address R3
     * (0x0c90f396). without it, the address takes R2, and 2 of kk_door_draw's
     * 143 words differ */
    default:
        break;
    }
    njPopMatrix(1);
}
