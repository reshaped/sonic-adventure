/* CYL_2, a cylinder with a grip for players to hold. a cannon's shell hits it
 * through its target, its entry in sky_deck_targets. the first hit sets it
 * puffing dust, and the second breaks it into CYLINDER_PIECES pieces. once
 * broken, it hits the platforms near it, then its pieces fly apart and fall
 * away */
#include <mathf.h>
#include <ninja.h>
#include <sg_xpt.h>
#include <stdlib.h>

#include "collision/world.h"
#include "debug/editor.h"
#include "draw/model.h"
#include "draw/tree.h"
#include "effect/dust.h"
#include "object/placement.h"
#include "player/player.h"
#include "stg06/cyl_2.h"
#include "stg06/cylinder.h"
#include "stg06/models.h"
#include "stg06/platform.h"
#include "stg06/shards.h"
#include "stg06/sky_deck.h"
#include "stg06/target.h"
#include "system/task.h"
#include "system/util.h"

extern NJS_OBJECT cyl_2_object;

extern NJS_OBJECT dat_0ca8c858;
extern NJS_OBJECT dat_0ca8cbfc;
extern NJS_OBJECT dat_0ca8ce2c;
extern NJS_OBJECT dat_0ca8d0f0;
extern NJS_OBJECT dat_0ca8d464;
extern NJS_OBJECT dat_0ca8d668;
extern NJS_OBJECT dat_0ca8d958;
extern NJS_OBJECT dat_0ca8dbcc;
extern NJS_OBJECT dat_0ca8ddfc;
extern NJS_OBJECT dat_0ca8e0ec;
extern NJS_OBJECT dat_0ca8e3dc;
extern NJS_OBJECT dat_0ca8e6a0;
extern NJS_OBJECT dat_0ca8e8a4;
extern NJS_OBJECT dat_0ca8eb58;
extern NJS_OBJECT dat_0ca8ede0;
extern NJS_OBJECT dat_0ca8efa8;
extern NJS_OBJECT dat_0ca8f26c;
extern NJS_OBJECT dat_0ca8f55c;
extern NJS_OBJECT dat_0ca8f78c;

/* the r of cyl_2_object's model. init picks the world size from it by tests
 * that SHC folds. so the image shows only that it lies in (100, 200] */
#define CYL_2_REACH 141.77777F
/* the world test's range from the cylinder's middle, which the step passes
 * player_in_range. 40016 is 200 squared plus 16, the form of a squared range,
 * but player_in_range takes a plain distance. so the test holds whenever
 * player 0 keeps the cylinder in range */
#define CYL_2_NEAR 40016.0F
/* the pieces' objects. each one's pos places it where it sits in the whole
 * cylinder */
static NJS_OBJECT *cyl_2_pieces[CYLINDER_PIECES] = {
    &dat_0ca8c858, &dat_0ca8cbfc, &dat_0ca8ce2c, &dat_0ca8d0f0, &dat_0ca8d464,
    &dat_0ca8d668, &dat_0ca8d958, &dat_0ca8dbcc, &dat_0ca8ddfc, &dat_0ca8e0ec,
    &dat_0ca8e3dc, &dat_0ca8e6a0, &dat_0ca8e8a4, &dat_0ca8eb58, &dat_0ca8ede0,
    &dat_0ca8efa8, &dat_0ca8f26c, &dat_0ca8f55c, &dat_0ca8f78c};

/* the speed of a broken cylinder's hits on the platforms near it: none */
static const NJS_VECTOR cyl_2_hit_speed = {0.0F, 0.0F, 0.0F};

static void cyl_2_init(struct task *task);
static void cyl_2_destroy(struct task *task);
static void cyl_2_step(struct task *task);
static void cyl_2_draw(struct task *task);

/* its target's hit at pos, by something moving at speed. TRUE for the first two
 * hits, and FALSE after. each of the two sets CYLINDER_YAW from speed. the
 * first sets CYLINDER_HIT, in which the step puffs dust. the second breaks the
 * cylinder into its pieces and sets TARGET_BROKEN. it then throws BREAK_SHARDS
 * shards from the cylinder's position.
 *
 * each piece flies from pos's offset from the cylinder through its place, at
 * its own random speed. a piece nearer that offset than that speed moves by its
 * offset from it instead. its spins then scale by its distance over that speed.
 * the heap must have room for the pieces: with none, they are written through
 * null */
static Bool cyl_2_break(NJS_POINT3 *pos, NJS_VECTOR *speed,
                        struct task_work *work)
{
    struct cylinder_pieces *pieces;
    struct platform_piece *piece;
    union point_floats impact;
    union point_floats away;
    NJS_VECTOR push;
    Float reach;
    Float distance;
    int i;
    Angle ang;

    /* @hack type: the yaw read and written through scl.z's address as an Angle,
     * which keeps that address across atan2f (0x0c91cb58) and loads the yaw
     * through a copy of work (0x0c91ccb0); through a struct for the work that
     * declares that word an Angle, the function ends 32 bytes early and 977 of
     * 1011 words differ */
    switch (work->mode) {
    case CYLINDER_WHOLE:
        if (speed->x == 0.0F && speed->y == 0.0F && speed->z == 0.0F) {
            CYLINDER_YAW(work) = 0;
        } else {
            CYLINDER_YAW(work) =
                (Angle)(atan2f(-speed->x, -speed->z) * ANGLE_PER_RADIAN) -
                work->ang.y;
        }
        work->mode = CYLINDER_HIT;
        work->timer = 0;
        return TRUE;
    case CYLINDER_HIT:
        if (!CYLINDER_VARS(work)->pieces) {
            pieces = heap_alloc(sizeof *pieces);
            CYLINDER_VARS(work)->pieces = pieces;
            pieces->hitting = TRUE;
            if (speed->x == 0.0F && speed->y == 0.0F && speed->z == 0.0F) {
                CYLINDER_YAW(work) = 0;
            } else {
                CYLINDER_YAW(work) =
                    (Angle)(atan2f(-speed->x, -speed->z) * ANGLE_PER_RADIAN) -
                    work->ang.y;
            }
            njPushMatrix(&_unit_matrix_);
            njScale(NULL, PIECE_PUSH, PIECE_PUSH, PIECE_PUSH);
            TURN_VIA(njRotateY, -work->ang.y, ang);
            njCalcVector(NULL, speed, &push);
            njPopMatrix(1);
            /* @bug logic: a turned cylinder's pieces fly from another point
             * than pos. pos's offset from the cylinder is taken in world axes
             * (FSUB at 0x0c91cc8c, 0x0c91cc9c and 0x0c91ccaa), but the pieces'
             * places are in the cylinder's frame, which the push above is
             * turned into by -ang.y (NEG at 0x0c91cc60). SET0600S.BIN turns the
             * 3 it places */
            impact.vector.x = pos->x - work->pos.x;
            impact.vector.y = pos->y - work->pos.y;
            impact.vector.z = pos->z - work->pos.z;
            njPushMatrix(&_unit_matrix_);
            /* @hack spelling: the yaw tested as != 0, which loads i's 0
             * afresh (MOV #0,R10 at 0x0c91cccc); without it, as TURN tests
             * the yaw, 447 of 1011 words differ */
            CYLINDER_YAW(work) != 0 ? njRotateY(NULL, CYLINDER_YAW(work))
                                    : (void)0;
            for (i = 0; i < CYLINDER_PIECES; i++) {
                piece = &pieces->pieces[i];
                reach =
                    (rand() * (PIECE_SPEED_SPREAD * RAND_SCALE)) + PIECE_SPEED;
                /* @hack type: the piece's pos, three Floats, read as one
                 * record, which copies it in three MOV.L words (0x0c91ccf6
                 * to 0x0c91cd08); copied by element, 812 of 1011 words
                 * differ and the function ends 96 bytes late */
                piece->pos = *(union point_floats *)cyl_2_pieces[i]->pos;
                njCalcPoint(NULL, &piece->pos.point, &piece->pos.point);
                piece->ang_x = 0;
                piece->ang_z = 0;
                piece->gone = FALSE;
                {
                    /* @hack type: the cursors converted from the unions'
                     * addresses, which sets them up in R4 to R6 as the
                     * image does (0x0c91cd0e to 0x0c91cd1a); from elements,
                     * 850 of 1011 words differ and the function ends 148
                     * bytes late */
                    Float *to = (Float *)&away;
                    Float *from = (Float *)&piece->pos;
                    Float *at = (Float *)&impact;

                    *to++ = *from++ - *at++;
                    *to++ = *from++ - *at++;
                    *to = *from - *at;
                }
                distance = njScalor(&away.vector);
                if (distance < reach) {
                    away.vector.x += push.x;
                    away.vector.y += push.y;
                    away.vector.z += push.z;
                    reach = distance / reach;
                    piece->spin_x.whole =
                        ((rand() % (2 * PIECE_SPIN)) - PIECE_SPIN) * reach;
                    piece->spin_z.whole =
                        ((rand() % (2 * PIECE_SPIN)) - PIECE_SPIN) * reach;
                } else {
                    reach /= distance;
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
            /* @hack redundant: a never-run store of mode to timer, which
             * orders the cursors' registers in four of the loop body's five
             * copies: in the first, to in R5 and from in R6 (0x0c91cd10 to
             * 0x0c91cd1a); without it, the two trade there, and 61 of 1011
             * words differ, and before the loop, 62 */
            if (0) {
                /* NOLINTNEXTLINE(bugprone-signed-char-misuse) */
                work->timer = work->mode;
            }
            njPopMatrix(1);
            sky_deck_shards_create(&work->pos, 3.0F, -0.1F, BREAK_SHARDS, 1);
            CYLINDER_VARS(work)->target->state |= TARGET_BROKEN;
            return TRUE;
        }
        break;
    }
    return FALSE;
}

/* @hack redundant: register on task, which keeps it in R14 across the range
 * check (0x0c91d410); without it, task is kept at sp+0 and 11 of 16 words
 * differ */
void cyl_2_start(register struct task *task)
{
    if (!placement_try_leave_by(task, PLATFORM_RANGE)) {
        cyl_2_init(task);
    }
}

/* put the cylinder in the world, unless the set editor holds it. add its
 * target, whose id is scl.x, and a grip at its position as a child */
static void cyl_2_init(struct task *task)
{
    struct task_work *work = task->work;
    NJS_OBJECT *world;
    NJS_OBJECT *shape;
    Uint32 flags;
    struct task *child;

    if (!placement_editing(task)) {
        CYLINDER_VARS(work)->world = world = world_object_alloc();
        shape = &cyl_2_object;
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
        /* @hack redundant: the world size picked by tests of the constant
         * reach, as for a variable one. SHC keeps the first, a TST of R13,
         * which holds 0 (0x0c91d4c0); set plainly to WORLD_LARGE, 108 of 112
         * words differ */
        flags = WORLD_SOLID;
        if (CYL_2_REACH <= WORLD_SMALL_REACH) {
            flags |= WORLD_SMALL;
        } else if (CYL_2_REACH <= WORLD_MEDIUM_REACH) {
            flags |= WORLD_MEDIUM;
        } else if (CYL_2_REACH <= WORLD_LARGE_REACH) {
            flags |= WORLD_LARGE;
        }
        world_add(flags, task, world);
    }
    task->draw = cyl_2_draw;
    task->step = cyl_2_step;
    task->destructor = cyl_2_destroy;
    work->timer = 0;
    work->mode = CYLINDER_WHOLE;
    CYLINDER_VARS(work)->pieces = NULL;
    CYLINDER_VARS(work)->target = sky_deck_target_add(work);
    CYLINDER_VARS(work)->target->id = (int)work->scl.x;
    CYLINDER_VARS(work)->target->group = TARGET_CYLINDER;
    CYLINDER_VARS(work)->target->hit = cyl_2_break;
    child =
        task_create_child(TASK_WORK | TASK_MOTION, sky_deck_grip_start, task);
    child->work->pos.x = work->pos.x;
    child->work->pos.y = work->pos.y;
    child->work->pos.z = work->pos.z;
}

static void cyl_2_destroy(struct task *task)
{
    struct task_work *work = task->work;

    if (CYLINDER_VARS(work)->world) {
        world_remove(task, CYLINDER_VARS(work)->world);
        world_object_free(CYLINDER_VARS(work)->world);
    }
    if (CYLINDER_VARS(work)->pieces) {
        heap_free(CYLINDER_VARS(work)->pieces);
    }
    task_destroy_children(task);
    sky_deck_target_remove(CYLINDER_VARS(work)->target);
}

/* the step does its work only while the land's first block is drawn. it draws
 * the cylinder and steps its grip, and does no more while the set editor holds
 * it. its target has TARGET_STOOD_ON while player 0 stands on it.
 *
 * once hit, the cylinder puffs dust each step on the side the hit came from.
 * once broken, it hits one platform each step, at cyl_2_hit_speed. the platform
 * is a TARGET_TALAP_1 target in the box HIT_REACH and HIT_HEIGHT give. of
 * those, the step takes the first with none of its state's bits set. the
 * hitting stops once the step finds none, or once a hit returns FALSE. the
 * cylinder then leaves the world, and its pieces fly until all are gone. its
 * target then gains TARGET_GONE */
static void cyl_2_step(struct task *task)
{
    struct task_work *work = task->work;
    struct sky_deck_target *target;
    struct platform_piece *piece;
    NJS_OBJECT *world;
    NJS_VECTOR speed;
    Angle ang;
    Angle turn;
    int i;
    int flying;
    int random;
    int side;
    int nudged;

    if (sky_deck_try_leave(task)) {
        return;
    }
    if (!(world_land_blocks & FIRST_BLOCK)) {
        return;
    }
    cyl_2_draw(task);
    task_step_children(task);
    if (placement_editing(task)) {
        return;
    }
    if (player_stands_on(0, task)) {
        CYLINDER_VARS(work)->target->state |= TARGET_STOOD_ON;
    } else {
        CYLINDER_VARS(work)->target->state &= ~TARGET_STOOD_ON;
    }
    if (CYLINDER_VARS(work)->pieces) {
        if (CYLINDER_VARS(work)->pieces->hitting) {
            target = sky_deck_target_find(TARGET_TALAP_1, &work->pos, HIT_REACH,
                                          0, HIT_HEIGHT);
            if (target) {
                speed = cyl_2_hit_speed;
                if (!target->hit(&target->work->pos, &speed, target->work)) {
                    CYLINDER_VARS(work)->pieces->hitting = FALSE;
                }
            } else {
                CYLINDER_VARS(work)->pieces->hitting = FALSE;
            }
        } else {
            flying = 0;
            for (i = 0; i < CYLINDER_PIECES; i++) {
                piece = &CYLINDER_VARS(work)->pieces->pieces[i];
                if (!piece->gone) {
                    flying++;
                    {
                        /* @hack spelling: pos stepped a Float at a time through
                         * cursors (FMOV.S @R4+,FR3 at 0x0c91d70e); by member,
                         * 781 of 819 words differ and the function ends 32
                         * bytes late */
                        Float *at = piece->pos.elements;
                        Float *by = piece->speed.elements;

                        *at++ += *by++;
                        *at++ += *by++;
                        *at += *by;
                    }
                    piece->speed.vector.y += PIECE_GRAVITY;
                    {
                        /* @hack spelling: speed scaled through a cursor
                         * (0x0c91d738 to 0x0c91d74c); by member, 682 of 819
                         * words differ and the function ends 32 bytes early */
                        Float *at = piece->speed.elements;

                        *at++ *= CYLINDER_DRAG;
                        *at++ *= CYLINDER_DRAG;
                        *at *= CYLINDER_DRAG;
                    }
                    piece->ang_x += piece->spin_x.whole;
                    piece->ang_z += piece->spin_z.whole;
                    /* @hack spelling: each spin's low half scaled through a
                     * pointer and a Float local (0x0c91d762 to 0x0c91d76e);
                     * scaled in place, 754 of 819 words differ and the
                     * function ends 32 bytes early */
                    {
                        Sint16 *spin = piece->spin_x.half;
                        Float drag = *spin;

                        drag *= CYLINDER_DRAG;
                        *spin = drag;
                    }
                    {
                        Sint16 *spin = piece->spin_z.half;
                        Float drag = *spin;

                        drag *= CYLINDER_DRAG;
                        *spin = drag;
                    }
                    if (njScalor(&piece->pos.vector) > PIECE_GONE) {
                        piece->gone = TRUE;
                    }
                }
            }
            if (CYLINDER_VARS(work)->world) {
                world_remove(task, CYLINDER_VARS(work)->world);
                world_object_free(CYLINDER_VARS(work)->world);
                CYLINDER_VARS(work)->world = NULL;
            }
            if (!flying) {
                work->mode = CYLINDER_GONE;
                work->timer = 0;
                heap_free(CYLINDER_VARS(work)->pieces);
                CYLINDER_VARS(work)->pieces = NULL;
                CYLINDER_VARS(work)->target->state |= TARGET_GONE;
            }
        }
    } else {
        if (work->mode == CYLINDER_HIT) {
            /* @hack spelling: puff declared in this block, which puts it at
             * sp+12 (0x0c91db04); declared at the function's top, 18 of 819
             * words differ */
            NJS_POINT3 puff;

            random = rand();
            side = random % 3;
            nudged = random % 2;
            njPushMatrix(&_unit_matrix_);
            njTranslateV(NULL, &work->pos);
            /* @hack spelling: ang and turn assigned in the test, as TURN_VIA
             * assigns its local, which keeps both in the frame for the call
             * (0x0c91daf2, 0x0c91daf4); assigned ahead of it, 309 of 819
             * words differ and the function ends 32 bytes early */
            /* @hack type: the yaw read through scl.z's address as an Angle,
             * which loads it through a copy of work, twice (0x0c91dac0,
             * 0x0c91daf0); through a struct for the work that declares that
             * word an Angle, the function ends 32 bytes early and 320 of 819
             * words differ */
            (CYLINDER_YAW(work) + (ang = work->ang.y) +
             (turn = ((((Float)side - 1.0F) * DUST_SIDE) +
                      ((Float)nudged * DUST_NUDGE)) *
                     ANGLE_PER_DEGREE))
                ? njRotateY(NULL, CYLINDER_YAW(work) + ang + turn)
                : (void)0;
            puff.x = 0.0F;
            puff.y = 0.0F;
            puff.z = DUST_OUT;
            njCalcPoint(NULL, &puff, &puff);
            njPopMatrix(1);
            speed.x = (Float)rand() * (0.2F * RAND_SCALE);
            speed.y = -2.6F;
            speed.z = -((Float)rand() * (0.5F * RAND_SCALE)) - 2.0F;
            njCalcVector(NULL, &speed, &speed);
            puff.y += DUST_RISE;
            dust_create(&puff, &speed,
                        ((Float)rand() * (0.9F * RAND_SCALE)) +
                            (side == 1 ? 0.6F : 0.0F) + 2.0F);
        }
        if (work->mode != CYLINDER_GONE) {
            world = CYLINDER_VARS(work)->world;
            if (world) {
                NJS_POINT3 middle;

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
                /* @bug logic: CYL_2_NEAR read as a plain distance (FMOV.S
                 * @R0,FR4 at 0x0c91dccc, for the call at 0x0c91dcce). the
                 * cylinder steps only while the camera or player 0 is within
                 * about 1200 of it, its kind's range. so the test holds
                 * whenever player 0 keeps it in range, and WORLD_TESTED stays
                 * set */
                if (player_in_range(&middle, CYL_2_NEAR)) {
                    work->flags |= WORLD_TESTED;
                } else {
                    work->flags &= ~WORLD_TESTED;
                }
            }
        } else {
            work->flags &= ~WORLD_TESTED;
        }
    }
}

/* draw the whole cylinder, or once it's broken, each piece not yet gone, turned
 * by CYLINDER_YAW. nothing is drawn out of PLATFORM_RANGE or once the cylinder
 * is gone */
static void cyl_2_draw(struct task *task)
{
    struct task_work *work = task->work;
    NJS_OBJECT *object = &cyl_2_object;
    struct platform_piece *piece;
    Angle ang;
    int i;

    if (placement_try_leave_by(task, PLATFORM_RANGE)) {
        return;
    }
    if (work->mode == CYLINDER_GONE) {
        return;
    }
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    TURN_VIA(njRotateY, work->ang.y, ang);
    if (CYLINDER_VARS(work)->pieces) {
        njSetTexture(&obj_skydeck_texlist);
        for (i = 0; i < CYLINDER_PIECES; i++) {
            piece = &CYLINDER_VARS(work)->pieces->pieces[i];
            if (!piece->gone) {
                njPushMatrix(NULL);
                njTranslateV(NULL, &piece->pos.vector);
                /* @hack spelling: the yaw tested as != 0, which keeps the
                 * pieces' table in R9 (0x0c91dde6) and loads njRotateY's
                 * address at the call (0x0c91de18); without it, as TURN tests
                 * the yaw, 278 of 348 words differ */
                /* @hack type: the yaw read through scl.z's address as an Angle,
                 * which loads it through a copy of work for the test and again
                 * for the call (0x0c91de10, 0x0c91de1a); through a struct for
                 * the work that declares that word an Angle, 279 of 348 words
                 * differ */
                CYLINDER_YAW(work) != 0 ? njRotateY(NULL, CYLINDER_YAW(work))
                                        : (void)0;
                TURN_VIA(njRotateZ, piece->ang_z, ang);
                TURN_VIA(njRotateX, piece->ang_x, ang);
                draw_model(cyl_2_pieces[i]->model);
                njPopMatrix(1);
            }
        }
    } else {
        njSetTexture(&obj_skydeck_texlist);
        draw_tree(object);
    }
    njPopMatrix(1);
    /* @hack redundant: never-run calls of njPopMatrix and njPushMatrix,
     * which weigh their addresses so that njPushMatrix's takes R11,
     * njTranslateV's R10 and njPopMatrix's R12 (0x0c91ddb2, 0x0c91ddba,
     * 0x0c91ddd4); without them, the three rotate and 21 of 348 words
     * differ */
    if (0) {
        njPopMatrix(1);
        njPopMatrix(1);
        njPushMatrix(NULL);
    }
}
