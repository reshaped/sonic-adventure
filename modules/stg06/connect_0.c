/* CONNECT_0, a platform that breaks into nine pieces when its target is hit.
 * the pieces fly apart and fall away. while whole, the platform draws its
 * shadow on the clouds */
#include <ninja.h>
#include <sg_xpt.h>
#include <stdlib.h>

#include "collision/collision.h"
#include "collision/world.h"
#include "debug/editor.h"
#include "draw/model.h"
#include "draw/tree.h"
#include "object/placement.h"
#include "player/player.h"
#include "stage/stage.h"
#include "stg06/connect_0.h"
#include "stg06/models.h"
#include "stg06/platform.h"
#include "stg06/shards.h"
#include "stg06/target.h"
#include "system/task.h"
#include "system/util.h"

extern NJS_OBJECT connect_0_object;
extern NJS_OBJECT connect_0_shadow_object;
extern NJS_OBJECT connect_0_world_object;

extern NJS_OBJECT dat_0ca81bb4;
extern NJS_OBJECT dat_0ca82094;
extern NJS_OBJECT dat_0ca82590;
extern NJS_OBJECT dat_0ca82a8c;
extern NJS_OBJECT dat_0ca82e74;
extern NJS_OBJECT dat_0ca82fdc;
extern NJS_OBJECT dat_0ca83350;
extern NJS_OBJECT dat_0ca83500;
extern NJS_OBJECT dat_0ca83758;

#define PIECES 9
/* the world test's range from the platform's middle, which the step passes
 * player_in_range. 40016 is 200 squared plus 16, the form of a squared range,
 * but player_in_range takes a plain distance. so the test holds whenever
 * player 0 keeps the platform in range */
#define CONNECT_0_NEAR 40016.0F

#define TEXT_1_LENGTH 21
#include "stg06/connect_0/text.inc"

/* clang-format off */
static struct collision_shape connect_0_shapes[2] = {
    {SHAPE_RECTANGLE | SHAPE_NO_PUSH, 0, PUSH_ALL | PUSH_FIRM(PUSH_ALL),
     {0.0F, 22.5F, 16.0F}, 20.0F, 7.5F, 0.0F, {0}, 0, NULL},
    {SHAPE_RECTANGLE | SHAPE_NO_PUSH, 0, PUSH_ALL | PUSH_FIRM(PUSH_ALL),
     {0.0F, 22.5F, -16.0F}, 20.0F, 7.5F, 0.0F, {0}, 0, NULL},
};
/* clang-format on */
/* the pieces' objects. each one's pos places it where it sits in the whole
 * platform */
static NJS_OBJECT *connect_0_pieces[PIECES] = {
    &dat_0ca81bb4, &dat_0ca82094, &dat_0ca82590, &dat_0ca82a8c, &dat_0ca82e74,
    &dat_0ca82fdc, &dat_0ca83350, &dat_0ca83500, &dat_0ca83758};

static void connect_0_init(struct task *task);
static void connect_0_destroy(struct task *task);
static void connect_0_step(struct task *task);
static void connect_0_draw(struct task *task);

/* its target's hit, by something moving at speed, at pos. TRUE for the hit that
 * breaks the platform, and FALSE once it's broken. that hit sets TARGET_BROKEN,
 * and ten shards burst from the platform's position. each piece flies straight
 * away from pos's offset from the platform at a random speed of its own. a
 * piece nearer that offset than that speed flies at its distance instead. its
 * spins then scale by that distance over the speed. each piece adds PIECE_PUSH
 * of the hit's speed, turned into the platform's frame */
static Bool connect_0_break(NJS_POINT3 *pos, NJS_VECTOR *speed,
                            struct task_work *work)
{
    /* @hack type: pieces typed as a pointer to all nine as one array; as a
     * plain pointer, 913 of 1002 words differ and the function ends 34 bytes
     * early */
    struct platform_piece(*pieces)[PIECES];
    struct platform_piece *piece;
    union point_floats impact;
    union point_floats away;
    NJS_VECTOR push;
    Float reach;
    Float distance;
    int i;
    Angle ang;

    /* @hack redundant: taking pieces' address, which keeps it at sp+4 from
     * heap_alloc on (0x0c913964) and speed at sp+12 (0x0c91393a); without
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
             * (FSUB at 0x0c9139b4, 0x0c9139c4 and 0x0c9139d4), but the pieces'
             * places are in the platform's frame, which the push above is
             * turned into by -ang.y (NEG at 0x0c91397a). SET0600S.BIN turns 27
             * of the 40 it places */
            impact.vector.x = pos->x - work->pos.x;
            impact.vector.y = pos->y - work->pos.y;
            impact.vector.z = pos->z - work->pos.z;
            for (i = 0; i < PIECES; i++) {
                /* @hack redundant: an empty else, whose jump SHC deletes once
                 * it has picked a scratch register, which puts each piece's
                 * offset in R2 (0x0c9139e6); without it, R3 takes the offset
                 * and 38 of 1002 words differ */
                if (1) {
                } else {
                }
                piece = &(*pieces)[i];
                reach =
                    (rand() * (PIECE_SPEED_SPREAD * RAND_SCALE)) + PIECE_SPEED;
                /* @hack type: the piece's pos, three Floats, read as one
                 * record, which copies it in three MOV.L words (0x0c913a06
                 * to 0x0c913a1a); copied by element, 933 of 1002 words
                 * differ and the function ends 192 bytes early */
                piece->pos = *(union point_floats *)connect_0_pieces[i]->pos;
                piece->ang_x = 0;
                piece->ang_z = 0;
                piece->gone = FALSE;
                {
                    /* @hack type: the cursors converted from the unions'
                     * addresses, which sets them up in R4 to R6 as the
                     * image does (0x0c913a00 to 0x0c913a2e); from elements,
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
                     * 0x0c914072; without it, the load fills the branch's
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
                    /* @hack spelling: the division written out, which loads
                     * the spins' -1024 into R13 and 2048 into R14 (0x0c9139de,
                     * 0x0c9139e0); as reach /= distance, 89 of 1002 words
                     * differ */
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
             * gives the pieces' table R8 and njScalor R9 (0x0c9139fe,
             * 0x0c913a4a); without it, the two trade and 10 of 1002 words
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
 * check (0x0c9141ac); without it, task is kept at sp+0, 12 of 16 words
 * differ, connect_0_init starts 2 bytes late and the object ends 32 bytes
 * late */
void connect_0_start(register struct task *task)
{
    if (!placement_try_leave_by(task, PLATFORM_RANGE)) {
        connect_0_init(task);
    }
}

/* start the platform whole. it goes in the world unless the set editor holds
 * it. its target joins TARGET_PLATFORM with the id scl.x */
static void connect_0_init(struct task *task)
{
    struct task_work *work = task->work;
    NJS_OBJECT *world;
    NJS_OBJECT *shape;
    Uint32 flags;

    if (!placement_editing(task)) {
        PLATFORM_VARS(work)->world = world = world_object_alloc();
        shape = &connect_0_world_object;
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
        world->model = shape->model;
        world->child = shape->child;
        world->sibling = NULL;
        work->flags |= WORLD_TESTED;
        /* @hack redundant: the world size chosen by tests of constants. SHC
         * still makes the first, a TST of R12, which holds 0, at 0x0c914246;
         * set plainly to WORLD_LARGE, 86 of 118 words differ and the function
         * ends 2 bytes early */
        flags = WORLD_SOLID;
        /* @hack redundant: the first test's never-run arm. the image keeps its
         * load of WORLD_SMALL | WORLD_SOLID, 0x60000001, at 0x0c914252;
         * without it, 29 of 118 words differ */
        if (0) {
            flags |= WORLD_SMALL;
        } else if (1) {
            flags |= WORLD_LARGE;
        }
        world_add(flags, task, world);
    }
    /* @hack redundant: the same call in both arms of a test of the character.
     * SHC keeps the test, a CMP/EQ #2 at 0x0c914268, and both calls; as one
     * call, 78 of 118 words differ and the object ends 64 bytes early */
    if (get_character() == CHARACTER_TAILS) {
        collision_init(task, connect_0_shapes, COLLISION_OBJECT, 1, 2);
    } else {
        collision_init(task, connect_0_shapes, COLLISION_OBJECT, 1, 2);
    }
    task->draw = connect_0_draw;
    task->step = connect_0_step;
    task->destructor = connect_0_destroy;
    work->timer = 0;
    work->mode = PLATFORM_WHOLE;
    PLATFORM_VARS(work)->pieces = NULL;
    PLATFORM_VARS(work)->target = sky_deck_target_add(work);
    PLATFORM_VARS(work)->target->id = (int)work->scl.x;
    PLATFORM_VARS(work)->target->group = TARGET_PLATFORM;
    PLATFORM_VARS(work)->target->hit = connect_0_break;
}

static void connect_0_destroy(struct task *task)
{
    struct task_work *work = task->work;

    if (PLATFORM_VARS(work)->world) {
        world_remove(task, PLATFORM_VARS(work)->world);
        world_object_free(PLATFORM_VARS(work)->world);
    }
    if (PLATFORM_VARS(work)->pieces) {
        heap_free(PLATFORM_VARS(work)->pieces);
    }
    sky_deck_target_remove(PLATFORM_VARS(work)->target);
}

/* draw and move the platform. while the set editor holds it, the step
 * labels scl.x as its target's id and does no more. its target has
 * TARGET_STOOD_ON while player 0 stands on it. the world tests the whole
 * platform whenever player 0 keeps it in range. once broken, the platform
 * leaves the world, and its pieces fly until every one is gone. its target
 * then gains TARGET_GONE */
static void connect_0_step(struct task *task)
{
    struct task_work *work = task->work;
    struct platform_piece *piece;
    NJS_OBJECT *world;
    /* @hack redundant: 12 bytes of frame that no code reads, of a type unknown,
     * declared as a Uint8[12]. the image's ADD #-28,R15 reserves them
     * (0x0c914356); without them, 2 of 615 words differ */
    Uint8 unused[12];
    NJS_POINT3 middle;
    int i;
    int flying;
    Angle ang;

    if (placement_try_leave_by(task, PLATFORM_RANGE)) {
        return;
    }
    connect_0_draw(task);
    if (placement_editing(task)) {
        njPrintC(EDITOR_LABEL_RIGHT(EDITOR_ROW_SCL_X), TEXT_1);
        return;
    }
    if (PLATFORM_VARS(work)->pieces) {
        flying = 0;
        /* @hack spelling: i stepped as i = i + 1, which gives i R13 and the
         * cursor R12 (0x0c91441a); as i++, the two trade and 114 of 615 words
         * differ */
        for (i = 0; i < PIECES; i = i + 1) {
            /* @hack redundant: an empty never-run test, whose deleted jump
             * picks R3 for the piece's offset in each unrolled copy but the
             * first (0x0c9144b8); without it, R2 takes the offset there and
             * 28 of 615 words differ */
            if (0) {
            }
            /* @hack type: the piece reached from a cursor at the first,
             * loaded in a statement of its own, and an offset cut to 16
             * bits, as the image adds it (0x0c91441e to 0x0c914428): the
             * cursor takes R12 and 44 R3. in one statement, the two trade
             * and 25 of 615 words differ. the cut offset is the index's
             * while i * 44 fits in 16 bits, as it does for each of the
             * PIECES pieces: indexed, 462 of 615 words differ */
            piece = PLATFORM_VARS(work)->pieces;
            piece = PLATFORM_PIECE(piece, i);
            if (!piece->gone) {
                flying++;
                {
                    /* @hack spelling: pos stepped a Float at a time through
                     * cursors (0x0c914438); by member, the function ends 64
                     * bytes late and 551 of 615 words differ */
                    Float *at = piece->pos.elements;
                    Float *by = piece->speed.elements;

                    *at++ += *by++;
                    *at++ += *by++;
                    *at += *by;
                }
                piece->speed.vector.y += PIECE_GRAVITY;
                {
                    /* @hack spelling: speed scaled a Float at a time through a
                     * cursor; by member, 412 of 615 words differ */
                    Float *at = piece->speed.elements;

                    *at++ *= PIECE_DRAG;
                    *at++ *= PIECE_DRAG;
                    *at *= PIECE_DRAG;
                }
                piece->ang_x += piece->spin_x.whole;
                piece->ang_z += piece->spin_z.whole;
                /* @hack spelling: each spin's low half scaled through a pointer
                 * and a Float local; scaled in place, 407 of 615 words
                 * differ */
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
    if (player_stands_on(0, task)) {
        PLATFORM_VARS(work)->target->state |= TARGET_STOOD_ON;
    } else {
        PLATFORM_VARS(work)->target->state &= ~TARGET_STOOD_ON;
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
            /* @bug logic: CONNECT_0_NEAR read as a plain distance (FMOV.S
             * @R0,FR4 at 0x0c914898, for the call at 0x0c91489a). the
             * platform steps only while the camera or player 0 is within about
             * 2100 of it, PLATFORM_RANGE. so the test holds whenever player 0
             * keeps it in range, and WORLD_TESTED stays set */
            /* @hack spelling: the flag writes spelled out, which give the
             * loop's third unrolled copy its pos cursor in R4 and its speed
             * cursor in R5 (0x0c914568); with |= and &=, the two trade and 19
             * of 615 words differ */
            if (player_in_range(&middle, CONNECT_0_NEAR)) {
                work->flags = work->flags | WORLD_TESTED;
            } else {
                work->flags = work->flags & ~WORLD_TESTED;
            }
        }
    } else {
        work->flags = work->flags & ~WORLD_TESTED;
    }
}

/* draw the platform whole, with its shadow, or its pieces while they fly.
 * nothing is drawn out of PLATFORM_RANGE or once the platform is gone */
static void connect_0_draw(struct task *task)
{
    struct task_work *work = task->work;
    NJS_OBJECT *object = &connect_0_object;
    NJS_OBJECT *shadow = &connect_0_shadow_object;
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
        for (i = 0; i < PIECES; i++) {
            /* @hack type: the piece reached from a cursor at the first,
             * loaded in a statement of its own, and an offset cut to 16
             * bits, as the image adds it (0x0c9149d6 to 0x0c9149e0): the
             * cursor takes R2 and 44 R3. in one statement, the two trade and
             * 45 of 317 words differ. the cut offset is the index's while
             * i * 44 fits in 16 bits, as it does for each of the PIECES
             * pieces: indexed, 276 of 317 words differ */
            piece = PLATFORM_VARS(work)->pieces;
            piece = PLATFORM_PIECE(piece, i);
            if (!piece->gone) {
                njPushMatrix(NULL);
                njTranslateV(NULL, &piece->pos.vector);
                TURN_VIA(njRotateZ, piece->ang_z, ang);
                TURN_VIA(njRotateX, piece->ang_x, ang);
                draw_model(connect_0_pieces[i]->model);
                njPopMatrix(1);
            }
        }
    } else {
        njSetTexture(&obj_skydeck_texlist);
        draw_tree(object);
    }
    njPopMatrix(1);
    if (!PLATFORM_VARS(work)->pieces) {
        sky_deck_shadow_draw(task, shadow);
    }
}
