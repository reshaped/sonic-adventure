/* TALAP_1, a platform that breaks into nine pieces when its target is hit. the
 * pieces fly apart and fall away */
#include <ninja.h>
#include <sg_xpt.h>
#include <stdlib.h>

#include "collision/world.h"
#include "debug/editor.h"
#include "draw/model.h"
#include "draw/tree.h"
#include "object/placement.h"
#include "player/player.h"
#include "stg06/models.h"
#include "stg06/platform.h"
#include "stg06/shards.h"
#include "stg06/sky_deck.h"
#include "stg06/talap_1.h"
#include "stg06/target.h"
#include "system/task.h"
#include "system/util.h"

extern NJS_OBJECT talap_1_object;
extern NJS_OBJECT talap_1_world_object;

extern NJS_OBJECT dat_0ca8a598;
extern NJS_OBJECT dat_0ca8a7dc;
extern NJS_OBJECT dat_0ca8ab2c;
extern NJS_OBJECT dat_0ca8ac94;
extern NJS_OBJECT dat_0ca8adfc;
extern NJS_OBJECT dat_0ca8afe4;
extern NJS_OBJECT dat_0ca8b220;
extern NJS_OBJECT dat_0ca8b4a4;
extern NJS_OBJECT dat_0ca8b7a8;

#define PIECES 9
/* the radius of talap_1_world_object's model, which sizes the platform in
 * the world */
#define TALAP_1_REACH 73.092415F
/* the world test's range from the platform's middle, which the step passes
 * player_in_range. 10016 is 100 squared plus 16, the form of a squared range,
 * but player_in_range takes a plain distance. so the test holds whenever
 * player 0 keeps the platform in range */
#define TALAP_1_NEAR 10016.0F

/* the pieces, each placed where it sits in the whole platform */
static NJS_OBJECT *talap_1_pieces[PIECES] = {
    &dat_0ca8a598, &dat_0ca8a7dc, &dat_0ca8ab2c, &dat_0ca8ac94, &dat_0ca8adfc,
    &dat_0ca8afe4, &dat_0ca8b220, &dat_0ca8b4a4, &dat_0ca8b7a8};

static void talap_1_init(struct task *task);
static void talap_1_destroy(struct task *task);
static void talap_1_step(struct task *task);
static void talap_1_draw(struct task *task);

/* the target's hit, which breaks the platform while it's whole. TRUE for that
 * hit, FALSE for any other. each piece flies from pos's offset from the
 * platform toward its place, at its own random speed. a piece nearer that
 * offset than that speed moves by its offset from it instead. its spins then
 * scale by its distance over that speed. each piece also takes PIECE_PUSH of
 * speed, the hitter's, turned into the platform's frame. the break then bursts
 * ten shards from the platform. the heap must have room for the pieces: with
 * none, they are written through null */
static Bool talap_1_break(NJS_POINT3 *pos, NJS_VECTOR *speed,
                          struct task_work *work)
{
    /* @hack type: pieces a pointer to all nine as one array; through a
     * plain pointer, 913 of 1002 words differ and the function ends 34
     * bytes early */
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
     * heap_alloc on (0x0c917264) and speed at sp+12 (0x0c91723a); without
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
             * (FSUB at 0x0c9172b4, 0x0c9172c4 and 0x0c9172d4), but the pieces'
             * places are in the platform's frame, which the push above is
             * turned into by -ang.y (NEG at 0x0c91727a). SET0600S.BIN turns 14
             * of the 17 it places */
            impact.vector.x = pos->x - work->pos.x;
            impact.vector.y = pos->y - work->pos.y;
            impact.vector.z = pos->z - work->pos.z;
            for (i = 0; i < PIECES; i++) {
                /* @hack redundant: if (1) with an empty else, whose jump SHC
                 * deletes once it has picked a scratch register, which puts
                 * each piece's offset in R2 (0x0c9172e6); without it, R3 takes
                 * the offset and 38 of 1002 words differ */
                if (1) {
                } else {
                }
                piece = &(*pieces)[i];
                reach =
                    (rand() * (PIECE_SPEED_SPREAD * RAND_SCALE)) + PIECE_SPEED;
                /* @hack type: the piece's pos, three Floats, read as one
                 * record, which copies it in three MOV.L words (0x0c917306
                 * to 0x0c91731a); copied by element, 933 of 1002 words
                 * differ and the function ends 192 bytes early */
                piece->pos = *(union point_floats *)talap_1_pieces[i]->pos;
                piece->ang_x = 0;
                piece->ang_z = 0;
                piece->gone = FALSE;
                {
                    /* @hack type: the cursors converted from the unions'
                     * addresses, which sets them up in R4 to R6 as the
                     * image does (0x0c917300 to 0x0c91732e); from elements,
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
                     * 0x0c917972; without it, the load fills the branch's
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
                    /* @hack spelling: the division written out, which
                     * loads the spins' -1024 into R13 and 2048 into R14
                     * (0x0c9172de, 0x0c9172e0); as reach /= distance, 89 of
                     * 1002 words differ */
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
             * gives the pieces' table R8 and njScalor R9 (0x0c9172fe,
             * 0x0c91734a); without it, the two trade and 10 of 1002 words
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
 * check (0x0c917aac); without it, task is kept at sp+0, 12 of 16 words
 * differ and talap_1_init starts 2 bytes late */
void talap_1_start(register struct task *task)
{
    if (!placement_try_leave_by(task, PLATFORM_RANGE)) {
        talap_1_init(task);
    }
}

/* start the platform whole. it goes in the world unless the editor holds
 * it. its target joins TARGET_TALAP_1 with the id scl.x */
static void talap_1_init(struct task *task)
{
    struct task_work *work = task->work;
    NJS_OBJECT *world;
    NJS_OBJECT *shape;
    Uint32 flags;

    if (!placement_editing(task)) {
        PLATFORM_VARS(work)->world = world = world_object_alloc();
        shape = &talap_1_world_object;
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
        /* @hack redundant: the world size chosen by tests of TALAP_1_REACH, a
         * constant. SHC still makes the first test, a TST of R13, which
         * holds 0, at 0x0c917b46. with the flags set plainly, 88 of 94 words
         * differ and the function ends 32 bytes early */
        flags = WORLD_SOLID;
        if (TALAP_1_REACH <= WORLD_SMALL_REACH) {
            flags |= WORLD_SMALL;
        } else if (TALAP_1_REACH <= WORLD_MEDIUM_REACH) {
            flags |= WORLD_MEDIUM;
        } else if (TALAP_1_REACH <= WORLD_LARGE_REACH) {
            flags |= WORLD_LARGE;
        }
        world_add(flags, task, world);
    }
    task->draw = talap_1_draw;
    task->step = talap_1_step;
    task->destructor = talap_1_destroy;
    work->timer = 0;
    work->mode = PLATFORM_WHOLE;
    PLATFORM_VARS(work)->pieces = NULL;
    PLATFORM_VARS(work)->target = sky_deck_target_add(work);
    PLATFORM_VARS(work)->target->id = (int)work->scl.x;
    PLATFORM_VARS(work)->target->group = TARGET_TALAP_1;
    PLATFORM_VARS(work)->target->hit = talap_1_break;
}

static void talap_1_destroy(struct task *task)
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

/* draw and move the platform while the land's first block is drawn. its
 * target has TARGET_STOOD_ON while player 0 stands on it. the world tests
 * the whole platform whenever player 0 keeps it in range. once broken, the
 * platform leaves the world, and its pieces fly until every one is gone. its
 * target then gains TARGET_GONE */
static void talap_1_step(struct task *task)
{
    struct task_work *work = task->work;
    struct platform_piece *piece;
    NJS_OBJECT *world;
    /* @hack redundant: 12 bytes of frame that no code reads, of a type unknown,
     * declared as a Uint8[12]. the frame takes 28 bytes (ADD #-28,R15 at
     * 0x0c917c76): ang, then middle, then these; without them, 2 of 618 words
     * differ */
    Uint8 unk_10[12];
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
    talap_1_draw(task);
    if (placement_editing(task)) {
        return;
    }
    if (PLATFORM_VARS(work)->pieces) {
        flying = 0;
        /* @hack spelling: i's step written out, which gives i R13 and the
         * cursor R12 (0x0c917cc6); as i++, the two trade and 144 of 618
         * words differ */
        for (i = 0; i < PIECES; i = i + 1) {
            /* @hack redundant: an empty never-run test, whose deleted jump
             * picks R3 for the piece's offset in each unrolled copy but the
             * first (0x0c917d62); without it, R2 takes the offset there and
             * 28 of 618 words differ */
            if (0) {
            }
            /* @hack type: the piece reached from a cursor at the first,
             * loaded in a statement of its own, and an offset cut to 16
             * bits, as the image adds it (0x0c917cc8 to 0x0c917cd2). the
             * cursor takes R12, and the piece's size, 44, takes R3. in one
             * statement, the two trade and 25 of 618 words differ. the cut
             * offset is the index's while i * 44 fits in 16 bits, as it does
             * for each of the PIECES pieces: indexed, the function ends 32
             * bytes early and 590 of 618 words differ */
            piece = PLATFORM_VARS(work)->pieces;
            piece = PLATFORM_PIECE(piece, i);
            if (!piece->gone) {
                flying++;
                {
                    /* @hack spelling: pos and speed stepped a Float at a
                     * time through cursors (0x0c917d7e); by member, 541 of
                     * 618 words differ and the function ends 32 bytes
                     * late */
                    Float *at = piece->pos.elements;
                    Float *by = piece->speed.elements;

                    *at++ += *by++;
                    *at++ += *by++;
                    *at += *by;
                }
                piece->speed.vector.y += PIECE_GRAVITY;
                {
                    /* @hack spelling: speed scaled a Float at a time through
                     * a cursor; by member, 412 of 618 words differ */
                    Float *at = piece->speed.elements;

                    *at++ *= PIECE_DRAG;
                    *at++ *= PIECE_DRAG;
                    *at *= PIECE_DRAG;
                }
                piece->ang_x += piece->spin_x.whole;
                piece->ang_z += piece->spin_z.whole;
                /* @hack spelling: each spin's low half scaled through a
                 * pointer and a Float; scaled in place, 403 of 618 words
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
            /* @bug logic: TALAP_1_NEAR read as a plain distance (MOVA at
             * 0x0c918148, from the pool at 0x0c9181a4, and FMOV.S @R0,FR4 at
             * 0x0c918158, for the call at 0x0c91815a). 10016 is 100 squared and
             * 16, but player_in_range takes a plain distance, not its square.
             * the platform steps only while the camera or player 0 is within
             * about 2100 of it, PLATFORM_RANGE. so the test holds for a player
             * anywhere within 10016 of the middle, as player 0 is whenever it
             * keeps the platform in range, and WORLD_TESTED stays set */
            /* @hack spelling: the flag writes spelled out, which give the
             * loop's third unrolled copy its pos cursor in R4 and its speed
             * cursor in R5 (0x0c917e12); with |= and &=, the two trade
             * and 19 of 618 words differ */
            if (player_in_range(&middle, TALAP_1_NEAR)) {
                work->flags = work->flags | WORLD_TESTED;
            } else {
                work->flags = work->flags & ~WORLD_TESTED;
            }
        }
    } else {
        work->flags = work->flags & ~WORLD_TESTED;
    }
}

/* draw the platform whole, or its pieces while they fly */
static void talap_1_draw(struct task *task)
{
    struct task_work *work = task->work;
    NJS_OBJECT *object = &talap_1_object;
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
        /* @hack spelling: i's step written out, which keeps work in R10
         * and njPushMatrix's address in R11 (0x0c918236); as i++, the two
         * trade and 17 of 296 words differ */
        for (i = 0; i < PIECES; i = i + 1) {
            /* @hack type: the piece reached from a cursor at the first,
             * loaded in a statement of its own, and an offset cut to 16
             * bits, as the image adds it (0x0c91828c to 0x0c918296). the
             * cursor takes R2, and the piece's size, 44, takes R3.
             * njPushMatrix's address takes R11 and njPopMatrix's R12
             * (0x0c918252, 0x0c918274). in one statement, each pair trades
             * and 59 of 296 words differ. the cut offset is the index's while
             * i * 44 fits in 16 bits, as it does for each of the PIECES
             * pieces: indexed, 251 of 296 words differ */
            piece = PLATFORM_VARS(work)->pieces;
            piece = PLATFORM_PIECE(piece, i);
            if (!piece->gone) {
                njPushMatrix(NULL);
                njTranslateV(NULL, &piece->pos.vector);
                TURN_VIA(njRotateZ, piece->ang_z, ang);
                TURN_VIA(njRotateX, piece->ang_x, ang);
                draw_model(talap_1_pieces[i]->model);
                njPopMatrix(1);
            }
        }
    } else {
        njSetTexture(&obj_skydeck_texlist);
        draw_tree(object);
    }
    njPopMatrix(1);
}
