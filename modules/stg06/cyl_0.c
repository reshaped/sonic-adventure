/* CYL_0, a cylinder with a grip for players to hold. its target's first hit
 * sets it puffing dust, and the second breaks it into nineteen pieces. once
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
#include "stg06/cyl_0.h"
#include "stg06/cylinder.h"
#include "stg06/models.h"
#include "stg06/platform.h"
#include "stg06/shards.h"
#include "stg06/sky_deck.h"
#include "stg06/target.h"
#include "system/task.h"
#include "system/util.h"

extern NJS_OBJECT cyl_0_object;

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

/* the radius of cyl_0_object's model, from which init picks the world
 * size. the image's code shows only that it is over 100 and at most 200 */
#define CYL_0_REACH 135.2365F
/* the world test's range from the cylinder's middle, which the step passes
 * player_in_range. 40016 is 200 squared plus 16, the form of a squared range,
 * but player_in_range takes a plain distance. so the test holds whenever
 * player 0 keeps the cylinder in range */
#define CYL_0_NEAR 40016.0F

/* the pieces' objects. each one's pos places it where it sits in the whole
 * cylinder */
static NJS_OBJECT *cyl_0_pieces[CYLINDER_PIECES] = {
    &dat_0ca8c858, &dat_0ca8cbfc, &dat_0ca8ce2c, &dat_0ca8d0f0, &dat_0ca8d464,
    &dat_0ca8d668, &dat_0ca8d958, &dat_0ca8dbcc, &dat_0ca8ddfc, &dat_0ca8e0ec,
    &dat_0ca8e3dc, &dat_0ca8e6a0, &dat_0ca8e8a4, &dat_0ca8eb58, &dat_0ca8ede0,
    &dat_0ca8efa8, &dat_0ca8f26c, &dat_0ca8f55c, &dat_0ca8f78c};

/* the speed of a broken cylinder's hits on the platforms near it: none */
static const NJS_VECTOR cyl_0_hit_speed = {0.0F, 0.0F, 0.0F};

static void cyl_0_init(struct task *task);
static void cyl_0_destroy(struct task *task);
static void cyl_0_step(struct task *task);
static void cyl_0_draw(struct task *task);

/* its target's hit, by something moving at speed, at pos. TRUE for the first
 * two hits, and FALSE after. each of the two sets CYLINDER_YAW from speed, or
 * to 0 for no speed. the first sets CYLINDER_HIT, in which the step puffs dust.
 * the second breaks the cylinder into its pieces and sets TARGET_BROKEN. ten
 * shards then burst from the cylinder's position. each piece starts where it
 * sits in the cylinder, turned by CYLINDER_YAW. it flies straight away from
 * pos's offset from the cylinder at a random speed of its own. a piece nearer
 * that offset than that speed flies at its distance instead. its spins then
 * scale by that distance over the speed. each piece adds PIECE_PUSH of the
 * hit's speed, turned into the cylinder's frame */
static Bool cyl_0_break(NJS_POINT3 *pos, NJS_VECTOR *speed,
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

    switch (work->mode) {
    case CYLINDER_WHOLE:
        if (speed->x == 0.0F && speed->y == 0.0F && speed->z == 0.0F) {
            /* @hack type: the yaw read and written, here and below, through
             * scl.z's address as an Angle, which keeps that address across
             * atan2f (0x0c919fb8) and loads the yaw through a copy of work
             * (0x0c91a110); through a struct for the work that declares that
             * word an Angle, the function ends 32 bytes early and 977 of 1011
             * words differ */
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
             * (FSUB at 0x0c91a0ec, 0x0c91a0fc and 0x0c91a10a), but the pieces'
             * places are in the cylinder's frame, which the push above is
             * turned into by -ang.y (NEG at 0x0c91a0c0). SET0600S.BIN turns the
             * one it places */
            impact.vector.x = pos->x - work->pos.x;
            impact.vector.y = pos->y - work->pos.y;
            impact.vector.z = pos->z - work->pos.z;
            njPushMatrix(&_unit_matrix_);
            /* @hack spelling: the yaw tested as != 0 in a ?:, which loads i's 0
             * afresh (MOV #0,R10 at 0x0c91a12c); as TURN tests it, 447 of 1011
             * words differ, and as an if, 62 */
            CYLINDER_YAW(work) != 0 ? njRotateY(NULL, CYLINDER_YAW(work))
                                    : (void)0;
            for (i = 0; i < CYLINDER_PIECES; i++) {
                piece = &pieces->pieces[i];
                reach =
                    (rand() * (PIECE_SPEED_SPREAD * RAND_SCALE)) + PIECE_SPEED;
                /* @hack type: the piece's pos, three Floats, read as one
                 * record, which copies it in three MOV.L words (0x0c91a156
                 * to 0x0c91a168); copied by element, 812 of 1011 words
                 * differ and the function ends 96 bytes late */
                piece->pos = *(union point_floats *)cyl_0_pieces[i]->pos;
                njCalcPoint(NULL, &piece->pos.point, &piece->pos.point);
                piece->ang_x = 0;
                piece->ang_z = 0;
                piece->gone = FALSE;
                {
                    /* @hack type: the cursors converted from the unions'
                     * addresses, which sets them up in R4 to R6 as the
                     * image does (0x0c91a16e to 0x0c91a17a); from elements,
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
            /* @hack redundant: a never-run store of mode to timer, which picks
             * the cursors' registers in four of the five copies SHC writes of
             * the loop's body.
             * in the first, to takes R5 and from R6 (0x0c91a170 to 0x0c91a17a);
             * without it, the two trade there and 61 of 1011 words differ,
             * and with it before the loop, 62 */
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
 * check (0x0c91a870); without it, task is kept at sp+0 and 11 of 16 words
 * differ */
void cyl_0_start(register struct task *task)
{
    if (!placement_try_leave_by(task, PLATFORM_RANGE)) {
        cyl_0_init(task);
    }
}

/* start the cylinder whole. it goes in the world unless the set editor
 * holds it. its target joins TARGET_CYLINDER with the id scl.x, and a grip
 * starts at its position as a child */
static void cyl_0_init(struct task *task)
{
    struct task_work *work = task->work;
    NJS_OBJECT *world;
    NJS_OBJECT *shape;
    Uint32 flags;
    struct task *child;

    if (!placement_editing(task)) {
        CYLINDER_VARS(work)->world = world = world_object_alloc();
        shape = &cyl_0_object;
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
        /* @hack redundant: the world size chosen by tests of CYL_0_REACH, a
         * constant. SHC still makes the first, a TST of R13, which holds 0,
         * at 0x0c91a920; set plainly to WORLD_LARGE, 108 of 112 words differ */
        flags = WORLD_SOLID;
        if (CYL_0_REACH <= WORLD_SMALL_REACH) {
            flags |= WORLD_SMALL;
        } else if (CYL_0_REACH <= WORLD_MEDIUM_REACH) {
            flags |= WORLD_MEDIUM;
        } else if (CYL_0_REACH <= WORLD_LARGE_REACH) {
            flags |= WORLD_LARGE;
        }
        world_add(flags, task, world);
    }
    task->draw = cyl_0_draw;
    task->step = cyl_0_step;
    task->destructor = cyl_0_destroy;
    work->timer = 0;
    work->mode = CYLINDER_WHOLE;
    CYLINDER_VARS(work)->pieces = NULL;
    CYLINDER_VARS(work)->target = sky_deck_target_add(work);
    CYLINDER_VARS(work)->target->id = (int)work->scl.x;
    CYLINDER_VARS(work)->target->group = TARGET_CYLINDER;
    CYLINDER_VARS(work)->target->hit = cyl_0_break;
    child =
        task_create_child(TASK_WORK | TASK_MOTION, sky_deck_grip_start, task);
    child->work->pos.x = work->pos.x;
    child->work->pos.y = work->pos.y;
    child->work->pos.z = work->pos.z;
}

static void cyl_0_destroy(struct task *task)
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

/* the step does its work only while the land draws FIRST_BLOCK. it
 * draws the cylinder and steps its grip, and while the set editor holds it,
 * does no more. its target has TARGET_STOOD_ON while player 0 stands on it.
 * the world tests the whole cylinder whenever player 0 keeps it in range.
 *
 * once hit, the cylinder puffs dust each step on the side the hit came from.
 * once broken, it hits one TARGET_TALAP_1 platform each step, at
 * cyl_0_hit_speed. it picks the first in its hit box whose state is 0. the
 * hitting stops once none is found or a hit returns FALSE. the cylinder then
 * leaves the world, and its pieces fly until all are gone */
static void cyl_0_step(struct task *task)
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
    cyl_0_draw(task);
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
                speed = cyl_0_hit_speed;
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
                         * cursors (FMOV.S @R5+,FR3 at 0x0c91ab6e); by member,
                         * 787 of 824 words differ and the function ends 32
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
                         * (0x0c91ab98 to 0x0c91abac); by member, 685 of 824
                         * words differ and the function ends 32 bytes early */
                        Float *at = piece->speed.elements;

                        *at++ *= CYLINDER_DRAG;
                        *at++ *= CYLINDER_DRAG;
                        *at *= CYLINDER_DRAG;
                    }
                    piece->ang_x += piece->spin_x.whole;
                    piece->ang_z += piece->spin_z.whole;
                    /* @hack spelling: each spin's low half scaled through a
                     * pointer and a Float local (0x0c91abc2 to 0x0c91abce);
                     * scaled in place, 739 of 824 words differ and the
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
             * sp+12 (0x0c91af64); declared at the function's top, 18 of 824
             * words differ */
            NJS_POINT3 puff;

            random = rand();
            side = random % 3;
            nudged = random % 2;
            njPushMatrix(&_unit_matrix_);
            njTranslateV(NULL, &work->pos);
            /* @hack spelling: ang and turn assigned in the test, as TURN_VIA
             * assigns its local, which keeps both in the frame for the call
             * (0x0c91af52, 0x0c91af54); assigned ahead of it, 275 of 824
             * words differ and the function ends 32 bytes early */
            /* @hack type: the yaw read through scl.z's address as an Angle,
             * which loads it through a copy of work, twice (0x0c91af20,
             * 0x0c91af50); through a struct for the work that declares that
             * word an Angle, the function ends 32 bytes early and 306 of 824
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
        if (work->mode != CYLINDER_GONE && CYLINDER_VARS(work)->world) {
            world = CYLINDER_VARS(work)->world;
            /* @hack redundant: world tested again, though the test above
             * passes only with it set. the image tests it twice (TST at
             * 0x0c91b088 and 0x0c91b092); tested once, 123 of 824 words
             * differ and the object ends 32 bytes early */
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
                /* @bug logic: CYL_0_NEAR read as a plain distance (FMOV.S
                 * @R0,FR4 at 0x0c91b136, for the call at 0x0c91b138). the
                 * cylinder steps only while the camera or player 0 is within
                 * about 1200 of it, its kind's range. so the test holds
                 * whenever player 0 keeps it in range, and WORLD_TESTED stays
                 * set */
                if (player_in_range(&middle, CYL_0_NEAR)) {
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

/* draw the whole cylinder, or once it's broken, each piece not yet gone,
 * turned by CYLINDER_YAW. nothing is drawn out of PLATFORM_RANGE or once the
 * cylinder is gone */
static void cyl_0_draw(struct task *task)
{
    struct task_work *work = task->work;
    NJS_OBJECT *object = &cyl_0_object;
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
                /* @hack spelling: the yaw tested as != 0 in a ?:, which keeps
                 * the pieces' table in R9 (0x0c91b266) and loads njRotateY's
                 * address at the call (0x0c91b298); as TURN tests it, 278 of
                 * 348 words differ, and as an if, 131 */
                /* @hack type: the yaw read through scl.z's address as an Angle,
                 * which loads it through a copy of work for the test and again
                 * for the call (0x0c91b290, 0x0c91b29a); through a struct for
                 * the work that declares that word an Angle, 279 of 348 words
                 * differ */
                CYLINDER_YAW(work) != 0 ? njRotateY(NULL, CYLINDER_YAW(work))
                                        : (void)0;
                TURN_VIA(njRotateZ, piece->ang_z, ang);
                TURN_VIA(njRotateX, piece->ang_x, ang);
                draw_model(cyl_0_pieces[i]->model);
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
     * njTranslateV's R10 and njPopMatrix's R12 (0x0c91b232, 0x0c91b23a,
     * 0x0c91b254); without them, the three rotate and 21 of 348 words
     * differ */
    if (0) {
        njPopMatrix(1);
        njPopMatrix(1);
        njPushMatrix(NULL);
    }
}
