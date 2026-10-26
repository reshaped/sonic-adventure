/* TALAP_0, a platform that breaks into 34 pieces when its target is hit. the
 * pieces fly apart and fall away */
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
#include "stg06/models.h"
#include "stg06/platform.h"
#include "stg06/shards.h"
#include "stg06/sky_deck.h"
#include "stg06/talap_0.h"
#include "stg06/target.h"
#include "system/task.h"
#include "system/util.h"

extern NJS_OBJECT talap_0_object;
extern NJS_OBJECT talap_0_world_object;

extern NJS_OBJECT dat_0ca8385c;
extern NJS_OBJECT dat_0ca83b60;
extern NJS_OBJECT dat_0ca83c44;
extern NJS_OBJECT dat_0ca83d48;
extern NJS_OBJECT dat_0ca83f58;
extern NJS_OBJECT dat_0ca84168;
extern NJS_OBJECT dat_0ca843a4;
extern NJS_OBJECT dat_0ca844a8;
extern NJS_OBJECT dat_0ca847ac;
extern NJS_OBJECT dat_0ca84acc;
extern NJS_OBJECT dat_0ca84dd0;
extern NJS_OBJECT dat_0ca84fe0;
extern NJS_OBJECT dat_0ca850e4;
extern NJS_OBJECT dat_0ca851c8;
extern NJS_OBJECT dat_0ca852ac;
extern NJS_OBJECT dat_0ca853b0;
extern NJS_OBJECT dat_0ca854b4;
extern NJS_OBJECT dat_0ca8570c;
extern NJS_OBJECT dat_0ca85a8c;
extern NJS_OBJECT dat_0ca85c3c;
extern NJS_OBJECT dat_0ca85dec;
extern NJS_OBJECT dat_0ca861cc;
extern NJS_OBJECT dat_0ca86348;
extern NJS_OBJECT dat_0ca86788;
extern NJS_OBJECT dat_0ca869cc;
extern NJS_OBJECT dat_0ca86d84;
extern NJS_OBJECT dat_0ca86f20;
extern NJS_OBJECT dat_0ca87360;
extern NJS_OBJECT dat_0ca875a4;
extern NJS_OBJECT dat_0ca87740;
extern NJS_OBJECT dat_0ca878f0;
extern NJS_OBJECT dat_0ca87a8c;
extern NJS_OBJECT dat_0ca87c28;
extern NJS_OBJECT dat_0ca87dc4;

#define PIECES 34
/* how far its collision shapes reach from its position. it also sizes the
 * platform in the world */
#define TALAP_0_REACH 115.0F
/* the world test's range from the platform's middle, which the step passes
 * player_in_range. 40016 is 200 squared plus 16, the form of a squared range,
 * but player_in_range takes a plain distance. so the test holds whenever
 * player 0 keeps the platform in range */
#define TALAP_0_NEAR 40016.0F

/* clang-format off */
static struct collision_shape talap_0_shapes[2] = {
    {SHAPE_RECTANGLE | SHAPE_NO_PUSH, 0, PUSH_ALL | PUSH_FIRM(PUSH_ALL),
     {0.0F, 22.5F, 16.0F}, 100.0F, 7.5F, 0.0F, {0}, 0, NULL},
    {SHAPE_RECTANGLE | SHAPE_NO_PUSH, 0, PUSH_ALL | PUSH_FIRM(PUSH_ALL),
     {0.0F, 22.5F, -16.0F}, 100.0F, 7.5F, 0.0F, {0}, 0, NULL},
};
/* clang-format on */
/* the pieces, each placed where it sits in the whole platform */
static NJS_OBJECT *talap_0_pieces[PIECES] = {
    &dat_0ca8385c, &dat_0ca83b60, &dat_0ca83c44, &dat_0ca83d48, &dat_0ca83f58,
    &dat_0ca84168, &dat_0ca843a4, &dat_0ca844a8, &dat_0ca847ac, &dat_0ca84acc,
    &dat_0ca84dd0, &dat_0ca84fe0, &dat_0ca850e4, &dat_0ca851c8, &dat_0ca852ac,
    &dat_0ca853b0, &dat_0ca854b4, &dat_0ca8570c, &dat_0ca85a8c, &dat_0ca85c3c,
    &dat_0ca85dec, &dat_0ca861cc, &dat_0ca86348, &dat_0ca86788, &dat_0ca869cc,
    &dat_0ca86d84, &dat_0ca86f20, &dat_0ca87360, &dat_0ca875a4, &dat_0ca87740,
    &dat_0ca878f0, &dat_0ca87a8c, &dat_0ca87c28, &dat_0ca87dc4};

static void talap_0_init(struct task *task);
static void talap_0_destroy(struct task *task);
static void talap_0_step(struct task *task);
static void talap_0_draw(struct task *task);

/* the target's hit, which breaks the platform while it's whole. TRUE for that
 * hit, FALSE for any other. each piece flies from pos's offset from the
 * platform toward its place, at its own random speed. a piece nearer that
 * offset than that speed moves by its offset from it instead. its spins then
 * scale by its distance over that speed. each piece also takes PIECE_PUSH of
 * speed, the hitter's, turned into the platform's frame. the break then bursts
 * ten shards from the platform. the heap must have room for the pieces: with
 * none, they are written through null */
static Bool talap_0_break(NJS_POINT3 *pos, NJS_VECTOR *speed,
                          struct task_work *work)
{
    /* @hack type: pieces a pointer to all 34 as one array; through a
     * plain pointer, 914 of 1002 words differ and the function ends 34
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
     * heap_alloc on (0x0c915f44) and speed at sp+12 (0x0c915f1a); without
     * it, the two trade and 203 of 1002 words differ */
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
             * (FSUB at 0x0c915f94, 0x0c915fa4 and 0x0c915fb4), but the pieces'
             * places are in the platform's frame, which the push above is
             * turned into by -ang.y (NEG at 0x0c915f5a). SET0600S.BIN turns 25
             * of the 37 it places */
            impact.vector.x = pos->x - work->pos.x;
            impact.vector.y = pos->y - work->pos.y;
            impact.vector.z = pos->z - work->pos.z;
            for (i = 0; i < PIECES; i++) {
                /* @hack redundant: if (1) with an empty else, whose jump SHC
                 * deletes once it has picked a scratch register, which puts
                 * each piece's offset in R2 (0x0c915fc6); without it, R3 takes
                 * the offset and 38 of 1002 words differ */
                if (1) {
                } else {
                }
                piece = &(*pieces)[i];
                reach =
                    (rand() * (PIECE_SPEED_SPREAD * RAND_SCALE)) + PIECE_SPEED;
                /* @hack type: the piece's pos, three Floats, read as one
                 * record, which copies it in three MOV.L words (0x0c915fe6
                 * to 0x0c915ffa); copied by element, 933 of 1002 words
                 * differ and the function ends 192 bytes early */
                piece->pos = *(union point_floats *)talap_0_pieces[i]->pos;
                piece->ang_x = 0;
                piece->ang_z = 0;
                piece->gone = FALSE;
                {
                    /* @hack type: the cursors converted from the unions'
                     * addresses, which sets them up in R4 to R6 as the
                     * image does (0x0c915fe0 to 0x0c91600e); from elements,
                     * 924 of 1002 words differ and the function ends 118
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
                     * keeps the last two pieces' load of it after the BF at
                     * 0x0c916652; without it, the load fills the branch's
                     * delay slot, 117 of 1002 words differ and the function
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
                     * (0x0c915fbe, 0x0c915fc0); as reach /= distance, 77 of
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
             * gives the pieces' table R8 and njScalor R9 (0x0c915fde,
             * 0x0c91602a); without it, the two trade and 10 of 1002 words
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
 * check (0x0c91678c); without it, task is kept at sp+0, 12 of 16 words
 * differ, talap_0_init starts 2 bytes late and the object ends 32 bytes
 * late */
void talap_0_start(register struct task *task)
{
    if (!placement_try_leave_by(task, PLATFORM_RANGE)) {
        talap_0_init(task);
    }
}

/* start the platform whole. it goes in the world unless the editor holds
 * it. its target joins TARGET_PLATFORM with the id scl.x */
static void talap_0_init(struct task *task)
{
    struct task_work *work = task->work;
    NJS_OBJECT *world;
    NJS_OBJECT *shape;
    Uint32 flags;

    if (!placement_editing(task)) {
        PLATFORM_VARS(work)->world = world = world_object_alloc();
        shape = &talap_0_world_object;
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
        /* @hack redundant: the world size chosen by tests of TALAP_0_REACH, a
         * constant. SHC still makes the first test, a TST of R12, which
         * holds 0, at 0x0c916826. with the flags set plainly, 88 of 123 words
         * differ */
        flags = WORLD_SOLID;
        if (TALAP_0_REACH <= WORLD_SMALL_REACH) {
            flags |= WORLD_SMALL;
        } else if (TALAP_0_REACH <= WORLD_MEDIUM_REACH) {
            flags |= WORLD_MEDIUM;
        } else if (TALAP_0_REACH <= WORLD_LARGE_REACH) {
            flags |= WORLD_LARGE;
        }
        world_add(flags, task, world);
    }
    /* @hack redundant: the same call in both arms of a test of the
     * character, which SHC keeps with both calls (the test at 0x0c916848);
     * with one call, 83 of 123 words differ and the object ends 64 bytes
     * early */
    if (get_character() == CHARACTER_TAILS) {
        collision_init(task, talap_0_shapes, COLLISION_OBJECT, 1, 2);
    } else {
        collision_init(task, talap_0_shapes, COLLISION_OBJECT, 1, 2);
    }
    work->collision->radius = TALAP_0_REACH;
    task->draw = talap_0_draw;
    task->step = talap_0_step;
    task->destructor = talap_0_destroy;
    work->timer = 0;
    work->mode = PLATFORM_WHOLE;
    PLATFORM_VARS(work)->pieces = NULL;
    PLATFORM_VARS(work)->target = sky_deck_target_add(work);
    PLATFORM_VARS(work)->target->id = (int)work->scl.x;
    PLATFORM_VARS(work)->target->group = TARGET_PLATFORM;
    PLATFORM_VARS(work)->target->hit = talap_0_break;
}

static void talap_0_destroy(struct task *task)
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
static void talap_0_step(struct task *task)
{
    struct task_work *work = task->work;
    struct platform_piece *piece;
    NJS_OBJECT *world;
    /* @hack redundant: 12 bytes of frame that no code reads, of a type unknown,
     * declared as a Uint8[12]. the frame takes 28 bytes (ADD #-28,R15 at
     * 0x0c916996): ang, then middle, then these; without them, 2 of 618 words
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
    talap_0_draw(task);
    if (placement_editing(task)) {
        return;
    }
    if (PLATFORM_VARS(work)->pieces) {
        flying = 0;
        /* @hack spelling: i's step written out, which gives i R13 and the
         * cursor R12 (0x0c9169e6); as i++, the two trade and 144 of 618
         * words differ */
        for (i = 0; i < PIECES; i = i + 1) {
            /* @hack redundant: an empty never-run test, whose deleted jump
             * picks R3 for the piece's offset in each unrolled copy but the
             * first (0x0c916a82); without it, R2 takes the offset there and
             * 28 of 618 words differ */
            if (0) {
            }
            /* @hack type: the piece reached from a cursor at the first,
             * loaded in a statement of its own, and an offset cut to 16
             * bits, as the image adds it (0x0c9169e8 to 0x0c9169f2). the
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
                     * time through cursors (0x0c916a9e); by member, 541 of
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
            /* @bug logic: TALAP_0_NEAR read as a plain distance (MOVA at
             * 0x0c916e68, from the pool at 0x0c916ec4, and FMOV.S @R0,FR4 at
             * 0x0c916e78, for the call at 0x0c916e7a). 40016 is 200 squared and
             * 16, but player_in_range takes a plain distance, not its square.
             * the platform steps only while the camera or player 0 is within
             * about 2100 of it, PLATFORM_RANGE. so the test holds for a player
             * anywhere within 40016 of the middle, as player 0 is whenever it
             * keeps the platform in range, and WORLD_TESTED stays set */
            /* @hack spelling: the flag writes spelled out, which give the
             * loop's third unrolled copy its pos cursor in R4 and its speed
             * cursor in R5 (0x0c916b32); with |= and &=, the two trade
             * and 19 of 618 words differ */
            if (player_in_range(&middle, TALAP_0_NEAR)) {
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
static void talap_0_draw(struct task *task)
{
    struct task_work *work = task->work;
    NJS_OBJECT *object = &talap_0_object;
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
         * and njPushMatrix's address in R11 (0x0c916f56); as i++, the two
         * trade and 17 of 296 words differ */
        for (i = 0; i < PIECES; i = i + 1) {
            /* @hack type: the piece reached from a cursor at the first,
             * loaded in a statement of its own, and an offset cut to 16
             * bits, as the image adds it (0x0c916fac to 0x0c916fb6). the
             * cursor takes R2, and the piece's size, 44, takes R3.
             * njPushMatrix's address takes R11 and njPopMatrix's R12
             * (0x0c916f72, 0x0c916f94). in one statement, each pair trades
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
                draw_model(talap_0_pieces[i]->model);
                njPopMatrix(1);
            }
        }
    } else {
        njSetTexture(&obj_skydeck_texlist);
        draw_tree(object);
    }
    njPopMatrix(1);
}
