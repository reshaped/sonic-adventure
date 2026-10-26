/* EDGE_1, a platform with a ledge for players to hold. a cannon's shell hits it
 * through its target, its entry in sky_deck_targets. the first hit breaks it
 * into EDGE_1_PIECES pieces, which fly apart and fall away */
#include <ninja.h>
#include <sg_xpt.h>
#include <stdlib.h>

#include "collision/world.h"
#include "debug/editor.h"
#include "draw/model.h"
#include "draw/tree.h"
#include "object/placement.h"
#include "path/path.h"
#include "path/steps.h"
#include "player/player.h"
#include "stage/stage.h"
#include "stg06/edge_1.h"
#include "stg06/models.h"
#include "stg06/platform.h"
#include "stg06/shards.h"
#include "stg06/sky_deck.h"
#include "stg06/target.h"
#include "system/task.h"
#include "system/util.h"

extern NJS_OBJECT edge_1_object;
extern NJS_OBJECT edge_1_world_object;

extern NJS_OBJECT dat_0ca885c4;
extern NJS_OBJECT dat_0ca88a20;
extern NJS_OBJECT dat_0ca88ec4;
extern NJS_OBJECT dat_0ca8902c;
extern NJS_OBJECT dat_0ca895ac;
extern NJS_OBJECT dat_0ca898e0;
extern NJS_OBJECT dat_0ca89cdc;
extern NJS_OBJECT dat_0ca89f44;
extern NJS_OBJECT dat_0ca8a180;
extern NJS_OBJECT dat_0ca8a3e8;

#define EDGE_1_PIECES 10
/* the r of edge_1_world_object's model. init picks the world size from it by
 * tests that SHC folds. so the image shows only that it lies in (30, 100] */
#define EDGE_1_REACH 74.838615F
/* the world test's range from the platform's middle, which the step passes
 * player_in_range. 10016 is 100 squared plus 16, the form of a squared range,
 * but player_in_range takes a plain distance. so the test holds whenever
 * player 0 keeps the platform in range */
#define EDGE_1_NEAR 10016.0F
/* the ledge: a column of LEDGE_POINTS points LEDGE_STEP apart, the first
 * LEDGE_BASE + LEDGE_STEP above the platform. the column stands LEDGE_OUT out
 * from the platform, at the platform's yaw less LEDGE_TURN. the ledge's task
 * faces the platform's yaw less LEDGE_TASK_TURN */
#define LEDGE_POINTS 41
#define LEDGE_STEP 3.0F
#define LEDGE_BASE 6.0F
#define LEDGE_OUT 110.0F
#define LEDGE_TURN 0x2000
#define LEDGE_TASK_TURN 0x6000

/* the pieces' objects. each one's pos places it where it sits in the whole
 * platform */
static NJS_OBJECT *edge_1_pieces[EDGE_1_PIECES] = {
    &dat_0ca885c4, &dat_0ca88a20, &dat_0ca88ec4, &dat_0ca8902c, &dat_0ca895ac,
    &dat_0ca898e0, &dat_0ca89cdc, &dat_0ca89f44, &dat_0ca8a180, &dat_0ca8a3e8};

static void edge_1_init(struct task *task);
static void edge_1_destroy(struct task *task);
static void edge_1_step(struct task *task);
static void edge_1_draw(struct task *task);

/* its target's hit at pos, by something moving at speed. TRUE for the hit that
 * breaks the platform, and FALSE once it's broken. the hit breaks the platform
 * into its pieces and sets TARGET_BROKEN. it then throws BREAK_SHARDS shards
 * from the platform's position.
 *
 * each piece flies from pos's offset from the platform through its place, at
 * its own random speed. a piece nearer that offset than that speed moves by its
 * offset from it instead. its spins then scale by its distance over that speed.
 * the heap must have room for the pieces: with none, they are written through
 * null */
static Bool edge_1_break(NJS_POINT3 *pos, NJS_VECTOR *speed,
                         struct task_work *work)
{
    /* @hack type: pieces typed as a pointer to the ten as one array; as a plain
     * pointer, 913 of 1002 words differ and the function ends 34 bytes early */
    struct platform_piece(*pieces)[EDGE_1_PIECES];
    struct platform_piece *piece;
    union point_floats impact;
    union point_floats away;
    NJS_VECTOR push;
    Float reach;
    Float distance;
    int i;
    Angle ang;

    /* @hack redundant: taking pieces' address, which keeps it at sp+4 from
     * heap_alloc on (0x0c918b84) and speed at sp+12 (0x0c918b5a); without
     * it, the two trade and 191 of 1002 words differ */
    (void)&pieces;
    if (work->mode == PLATFORM_WHOLE) {
        if (!PLATFORM_VARS(work)->pieces) {
            pieces = heap_alloc(sizeof *pieces);
            njPushMatrix(&_unit_matrix_);
            njScale(NULL, PIECE_PUSH, PIECE_PUSH, PIECE_PUSH);
            TURN_VIA(njRotateY, -work->ang.y, ang);
            njCalcVector(NULL, speed, &push);
            njPopMatrix(1);
            PLATFORM_VARS(work)->pieces = *pieces;
            /* @bug logic: a turned platform's pieces fly from another point
             * than pos. pos's offset from the platform is taken in world axes
             * (FSUB at 0x0c918bd4, 0x0c918be4 and 0x0c918bf4), but the pieces'
             * places are in the platform's frame, which the push above is
             * turned into by -ang.y (NEG at 0x0c918b9a). SET0600S.BIN turns 6
             * of the 9 it places */
            impact.vector.x = pos->x - work->pos.x;
            impact.vector.y = pos->y - work->pos.y;
            impact.vector.z = pos->z - work->pos.z;
            for (i = 0; i < EDGE_1_PIECES; i++) {
                /* @hack redundant: an empty else, whose jump SHC deletes once
                 * it has picked a scratch register, which puts the piece's
                 * offset in R2 in the four unrolled copies (0x0c918c06) and
                 * in R3 in the last (0x0c919226); without it, the two trade
                 * and 38 of 1002 words differ */
                if (1) {
                } else {
                }
                piece = &(*pieces)[i];
                reach =
                    (rand() * (PIECE_SPEED_SPREAD * RAND_SCALE)) + PIECE_SPEED;
                /* @hack type: the piece's pos, three Floats, read as one
                 * record, which copies it in three MOV.L words (0x0c918c26
                 * to 0x0c918c3a); copied by element, 933 of 1002 words
                 * differ and the function ends 192 bytes early */
                piece->pos = *(union point_floats *)edge_1_pieces[i]->pos;
                piece->ang_x = 0;
                piece->ang_z = 0;
                piece->gone = FALSE;
                {
                    /* @hack type: the cursors converted from the unions'
                     * addresses, which sets them up in R4 to R6 as the
                     * image does (0x0c918c20 to 0x0c918c4e); from elements,
                     * 925 of 1002 words differ and the function ends 118
                     * bytes early */
                    Float *to = (Float *)&away;
                    Float *from = (Float *)&piece->pos;
                    Float *at = (Float *)&impact;

                    *to = *from - *at;
                    to++;
                    from++;
                    at++;
                    *to = *from - *at;
                    to++;
                    from++;
                    at++;
                    *to = *from - *at;
                }
                distance = njScalor(&away.vector);
                if (distance < reach) {
                    /* @hack redundant: a never-run store to push.x, which
                     * keeps the last piece's load of it after the BF at
                     * 0x0c919292; without it, the load fills the branch's
                     * delay slot, 98 of 1002 words differ and the function
                     * ends 4 bytes early */
                    if (0) {
                        push.x = push.y;
                    }
                    away.vector.x += push.x;
                    away.vector.y += push.y;
                    away.vector.z += push.z;
                    reach = distance / reach;
                    piece->spin_x.whole =
                        ((rand() % (2 * PIECE_SPIN)) - PIECE_SPIN) * reach;
                    piece->spin_z.whole =
                        ((rand() % (2 * PIECE_SPIN)) - PIECE_SPIN) * reach;
                } else {
                    /* @hack spelling: reach written out as reach / distance; as
                     * reach /= distance, 89 of 1002 words differ */
                    reach = reach / distance;
                    away.vector.x = (away.vector.x * reach) + push.x;
                    away.vector.y = (away.vector.y * reach) + push.y;
                    away.vector.z = (away.vector.z * reach) + push.z;
                    piece->spin_x.whole =
                        (rand() % (2 * PIECE_SPIN)) - PIECE_SPIN;
                    piece->spin_z.whole =
                        (rand() % (2 * PIECE_SPIN)) - PIECE_SPIN;
                }
                piece->speed = away;
            }
            /* @hack redundant: a never-run test of njScalor's address, which
             * gives the pieces' table R8 and njScalor R9 (0x0c918c1e,
             * 0x0c918c6a); without it, the two trade and 10 of 1002 words
             * differ */
            if (0) {
                if (&njScalor) {
                }
            }
            sky_deck_shards_create(&work->pos, 3.0F, -0.1F, BREAK_SHARDS, 1);
            PLATFORM_VARS(work)->target->state |= TARGET_BROKEN;
            return TRUE;
        }
    }
    return FALSE;
}

/* @hack redundant: register on task, which keeps it in R14 across the range
 * check (0x0c9193cc); without it, task is kept at sp+0, 12 of 16 words
 * differ and edge_1_init starts 2 bytes late */
void edge_1_start(register struct task *task)
{
    if (!placement_try_leave_by(task, PLATFORM_RANGE)) {
        edge_1_init(task);
    }
}

/* put the platform in the world, unless the set editor holds it. add its ledge,
 * as a child, and its target, whose id is scl.x */
static void edge_1_init(struct task *task)
{
    struct task_work *work = task->work;
    NJS_OBJECT *world;
    struct task *child;
    struct path_point *points;
    struct path_point *point;
    struct path *path;
    Uint32 flags;
    int i;
    NJS_POINT3 out;
    NJS_POINT3 offset;
    int count;
    Angle ang;

    if (!placement_editing(task)) {
        PLATFORM_VARS(work)->world = world = world_object_alloc();
        world->evalflags = 0;
        world->scl[0] = 1.0F;
        world->scl[1] = 1.0F;
        world->scl[2] = 1.0F;
        world->ang[0] = 0;
        world->ang[1] = work->ang.y;
        world->ang[2] = 0;
        world->pos[0] = work->pos.x;
        world->pos[1] = work->pos.y;
        world->pos[2] = work->pos.z;
        world->model = edge_1_world_object.model;
        world->child = edge_1_world_object.child;
        world->sibling = NULL;
        work->flags |= WORLD_TESTED;
        /* @hack redundant: the world size picked by tests of the constant
         * reach, as for a variable one. SHC keeps the first, a TST of R12,
         * which holds 0 (0x0c919470); set plainly to WORLD_MEDIUM, 179 of 230
         * words differ and the function ends 68 bytes early */
        flags = WORLD_SOLID;
        if (EDGE_1_REACH <= WORLD_SMALL_REACH) {
            flags |= WORLD_SMALL;
        } else if (EDGE_1_REACH <= WORLD_MEDIUM_REACH) {
            flags |= WORLD_MEDIUM;
        } else if (EDGE_1_REACH <= WORLD_LARGE_REACH) {
            flags |= WORLD_LARGE;
        }
        world_add(flags, task, world);
    }
    child = task_create_child(TASK_WORK, path_ledge_step, task);
    PATH_VARS(child->work)->stage_act = stage_get_stage_act();
    child->work->ang.x = 0;
    child->work->ang.y = work->ang.y - LEDGE_TASK_TURN;
    child->work->ang.z = 0;
    /* @hack spelling: the loop's bound read from a local, count; as
     * LEDGE_POINTS, 141 of 230 words differ */
    count = LEDGE_POINTS;
    offset.x = LEDGE_OUT;
    offset.y = LEDGE_BASE;
    offset.z = 0.0F;
    njPushMatrix(NULL);
    njUnitMatrix(NULL);
    TURN_VIA(njRotateY, work->ang.y - LEDGE_TURN, ang);
    points = heap_alloc(LEDGE_POINTS * sizeof(struct path_point));
    PATH_OWNER(work)->points = points;
    /* @hack spelling: the points walked by a cursor; indexed, 102 of 230 words
     * differ */
    for (i = 0, point = points; i < count; i++) {
        offset.y += LEDGE_STEP;
        point->ang_x = work->ang.x;
        point->ang_z = work->ang.z;
        point->length = LEDGE_STEP;
        njCalcPoint(NULL, &offset, &out);
        point->pos.x = work->pos.x + out.x;
        point->pos.y = work->pos.y + out.y;
        point->pos.z = work->pos.z + out.z;
        point++;
    }
    njPopMatrix(1);
    PATH_OWNER(work)->path = path = heap_alloc(sizeof(struct path));
    path->ends = PATH_STOP;
    path->count = LEDGE_POINTS;
    path->length = (LEDGE_POINTS - 1) * LEDGE_STEP;
    path->points = points;
    path->step = path_ledge_step;
    PATH_VARS(child->work)->path = path;
    task->draw = edge_1_draw;
    task->step = edge_1_step;
    task->destructor = edge_1_destroy;
    work->timer = 0;
    work->mode = PLATFORM_WHOLE;
    /* @bug memory: each platform leaks its ledge's points and path. the
     * pieces and the target take the vars' words that held PATH_OWNER's path
     * and points: the stores at 0x0c9195da and 0x0c9195e2 overwrite them.
     * nothing frees either after that, where the other path owners'
     * destructors free both */
    PLATFORM_VARS(work)->pieces = NULL;
    PLATFORM_VARS(work)->target = sky_deck_target_add(work);
    PLATFORM_VARS(work)->target->id = (int)work->scl.x;
    PLATFORM_VARS(work)->target->group = TARGET_TALAP_1;
    PLATFORM_VARS(work)->target->hit = edge_1_break;
}

static void edge_1_destroy(struct task *task)
{
    struct task_work *work = task->work;

    if (PLATFORM_VARS(work)->world) {
        world_remove(task, PLATFORM_VARS(work)->world);
        world_object_free(PLATFORM_VARS(work)->world);
    }
    if (PLATFORM_VARS(work)->pieces) {
        heap_free(PLATFORM_VARS(work)->pieces);
    }
    task_destroy_children(task);
    sky_deck_target_remove(PLATFORM_VARS(work)->target);
}

/* the step does its work only while the land's first block is drawn. it draws
 * the platform and steps its ledge, and does no more while the set editor holds
 * it. once broken, the platform leaves the world, and its pieces fly until all
 * are gone. its target then gains TARGET_GONE */
static void edge_1_step(struct task *task)
{
    struct task_work *work = task->work;
    struct platform_piece *piece;
    NJS_OBJECT *world;
    /* @hack redundant: 12 bytes of frame that no code reads, of a type unknown,
     * declared as a Uint8[12]. the frame takes 28 bytes (ADD #-28,R15 at
     * 0x0c9196f6): ang, then middle, then these; without them, 2 of 614 words
     * differ */
    Uint8 unused[12];
    NJS_POINT3 middle;
    int i;
    int flying;
    Angle ang;

    if (placement_try_leave_by(task, PLATFORM_RANGE)) {
        return;
    }
    if (!(world_land_blocks & FIRST_BLOCK)) {
        return;
    }
    edge_1_draw(task);
    task_step_children(task);
    if (placement_editing(task)) {
        return;
    }
    if (PLATFORM_VARS(work)->pieces) {
        flying = 0;
        /* @hack spelling: i stepped as i = i + 1; as i++, i and the cursor
         * trade R13 and R12 (0x0c91974c), and 114 of 614 words differ */
        for (i = 0; i < EDGE_1_PIECES; i = i + 1) {
            /* @hack redundant: an empty never-run test, whose deleted jump
             * picks R3 for the piece's offset in each copy of the loop's
             * body after the first (0x0c9197e8); without it, R2 takes the
             * offset there and 28 of 614 words differ */
            if (0) {
            }
            /* @hack type: the piece reached from a cursor at the first,
             * loaded in a statement of its own, and an offset cut to 16
             * bits, as the image adds it (0x0c91974e to 0x0c919758): the
             * cursor takes R12 and 44 R3. in one statement, the two trade
             * and 25 of 614 words differ. the cut offset is the index's
             * while i * 44 fits in 16 bits, as it does for each of the
             * EDGE_1_PIECES pieces: indexed, the function ends 32 bytes
             * early and 592 of 614 words differ */
            piece = PLATFORM_VARS(work)->pieces;
            piece = PLATFORM_PIECE(piece, i);
            if (!piece->gone) {
                flying++;
                {
                    /* @hack spelling: pos stepped a Float at a time through
                     * cursors, as the image steps it (0x0c919768); by member,
                     * 541 of 614 words differ and the function ends 32 bytes
                     * late */
                    Float *at = piece->pos.elements;
                    Float *by = piece->speed.elements;

                    *at++ += *by++;
                    *at++ += *by++;
                    *at += *by;
                }
                piece->speed.vector.y += PIECE_GRAVITY;
                {
                    /* @hack spelling: speed scaled through a cursor; by member,
                     * 496 of 614 words differ */
                    Float *at = piece->speed.elements;

                    *at++ *= PIECE_DRAG;
                    *at++ *= PIECE_DRAG;
                    *at *= PIECE_DRAG;
                }
                piece->ang_x += piece->spin_x.whole;
                piece->ang_z += piece->spin_z.whole;
                /* @hack spelling: each spin's low half scaled through a pointer
                 * and a Float local; scaled in place, 476 of 614 words differ
                 */
                {
                    Sint16 *spin = piece->spin_x.half;
                    Float drag = *spin;

                    drag *= PIECE_DRAG;
                    *spin = drag;
                }
                {
                    Sint16 *spin = piece->spin_z.half;
                    Float drag = *spin;

                    drag *= PIECE_DRAG;
                    *spin = drag;
                }
                if (njScalor(&piece->pos.vector) > PIECE_GONE) {
                    piece->gone = TRUE;
                }
            }
        }
        if (PLATFORM_VARS(work)->world) {
            world_remove(task, PLATFORM_VARS(work)->world);
            world_object_free(PLATFORM_VARS(work)->world);
            PLATFORM_VARS(work)->world = NULL;
        }
        if (!flying) {
            work->mode = PLATFORM_GONE;
            work->timer = 0;
            heap_free(PLATFORM_VARS(work)->pieces);
            PLATFORM_VARS(work)->pieces = NULL;
            PLATFORM_VARS(work)->target->state |= TARGET_GONE;
        }
    }
    /* @hack redundant: player_stands_on's result tested and dropped, which
     * keeps its TST at 0x0c919b16; without it, 93 of 614 words differ */
    if (player_stands_on(0, task)) {
    }
    if (work->mode != PLATFORM_GONE) {
        world = PLATFORM_VARS(work)->world;
        if (world) {
            middle.x = world->model->center.x;
            middle.y = world->model->center.y;
            middle.z = world->model->center.z;
            njPushMatrix(&_unit_matrix_);
            njTranslateV(NULL, &work->pos);
            TURN_VIA(njRotateZ, work->ang.z, ang);
            TURN_VIA(njRotateY, work->ang.y, ang);
            TURN_VIA(njRotateX, work->ang.x, ang);
            njCalcPoint(NULL, &middle, &middle);
            njPopMatrix(1);
            world->pos[0] = work->pos.x;
            world->pos[1] = work->pos.y;
            world->pos[2] = work->pos.z;
            world->ang[0] = work->ang.x;
            world->ang[1] = work->ang.y;
            world->ang[2] = work->ang.z;
            /* @bug logic: EDGE_1_NEAR read as a plain distance (FMOV.S
             * @R0,FR4 at 0x0c919bc8, for the call at 0x0c919bca). the platform
             * steps only while the camera or player 0 is within about 2100 of
             * it, PLATFORM_RANGE. so the test holds whenever player 0 keeps it
             * in range, and WORLD_TESTED stays set */
            if (player_in_range(&middle, EDGE_1_NEAR)) {
                work->flags |= WORLD_TESTED;
            } else {
                work->flags &= ~WORLD_TESTED;
            }
        }
    } else {
        work->flags &= ~WORLD_TESTED;
    }
}

/* draw the whole platform, or once it's broken, each piece not yet gone.
 * nothing is drawn out of PLATFORM_RANGE or once the platform is gone */
static void edge_1_draw(struct task *task)
{
    struct task_work *work = task->work;
    NJS_OBJECT *object = &edge_1_object;
    struct platform_piece *piece;
    int i;
    Angle ang;

    if (placement_try_leave_by(task, PLATFORM_RANGE)) {
        return;
    }
    if (work->mode == PLATFORM_GONE) {
        return;
    }
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    TURN_VIA(njRotateY, work->ang.y, ang);
    if (PLATFORM_VARS(work)->pieces) {
        njSetTexture(&obj_skydeck_texlist);
        /* @hack spelling: i stepped as i = i + 1; as i++, work and
         * njPushMatrix's address trade R10 and R11 (0x0c919c96), and 17 of 296
         * words differ */
        for (i = 0; i < EDGE_1_PIECES; i = i + 1) {
            /* @hack type: the piece reached from a cursor at the first,
             * loaded in a statement of its own, and an offset cut to 16
             * bits, as the image adds it (0x0c919cec to 0x0c919cf6): the
             * cursor takes R2 and 44 R3, and njPushMatrix's address R11 and
             * njPopMatrix's R12 (0x0c919cb2, 0x0c919cd4). in one statement,
             * each pair trades and 59 of 296 words differ. the cut offset is
             * the index's while i * 44 fits in 16 bits, as it does for each of
             * the EDGE_1_PIECES pieces: indexed, the function ends 96 bytes
             * early and 251 of 296 words differ */
            piece = PLATFORM_VARS(work)->pieces;
            piece = PLATFORM_PIECE(piece, i);
            if (!piece->gone) {
                njPushMatrix(NULL);
                njTranslateV(NULL, &piece->pos.vector);
                TURN_VIA(njRotateZ, piece->ang_z, ang);
                TURN_VIA(njRotateX, piece->ang_x, ang);
                draw_model(edge_1_pieces[i]->model);
                njPopMatrix(1);
            }
        }
    } else {
        njSetTexture(&obj_skydeck_texlist);
        draw_tree(object);
    }
    njPopMatrix(1);
}
