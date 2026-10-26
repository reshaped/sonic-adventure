/* O TANKA: a tank in three sizes that Knuckles bursts with three hits */
#include <mathf.h>
#include <ninja.h>
#include <sg_xpt.h>
#include <stdlib.h>

#include "collision/collision.h"
#include "collision/world.h"
#include "debug/editor.h"
#include "draw/draw.h"
#include "draw/model.h"
#include "draw/tree.h"
#include "effect/explosion.h"
#include "effect/hit.h"
#include "object/common.h"
#include "object/placement.h"
#include "player/knuckles.h"
#include "player/player.h"
#include "sound/sound.h"
#include "stg04/tank.h"
#include "system/task.h"
#include "system/texture.h"
#include "system/util.h"

extern NJS_OBJECT tank_a_body_object;
extern NJS_OBJECT tank_a_top_object;
extern NJS_OBJECT tank_a_piece_1_object;
extern NJS_OBJECT tank_a_piece_2_object;
extern NJS_OBJECT tank_a_piece_3_object;
extern NJS_OBJECT tank_a_piece_4_object;
extern NJS_OBJECT tank_a_side_object;
extern NJS_OBJECT tank_b_body_object;
extern NJS_OBJECT tank_b_top_object;
extern NJS_OBJECT tank_b_piece_1_object;
extern NJS_OBJECT tank_b_piece_2_object;
extern NJS_OBJECT tank_b_piece_3_object;
extern NJS_OBJECT tank_b_piece_4_object;
extern NJS_OBJECT tank_b_side_object;
extern NJS_OBJECT tank_c_body_object;
extern NJS_OBJECT tank_c_top_object;
extern NJS_OBJECT tank_c_piece_1_object;
extern NJS_OBJECT tank_c_piece_2_object;
extern NJS_OBJECT tank_c_piece_3_object;
extern NJS_OBJECT tank_c_piece_4_object;
extern NJS_OBJECT tank_c_pipe_object;
extern NJS_OBJECT tank_c_side_object;

/* scale x chooses the tank: tank a below 10, tank b below 20 and tank c
 * otherwise */
#define TANK_B_SCALE 10.0F
#define TANK_C_SCALE 20.0F

#define SE_PUNCH 111

/* bits of the tank's work->tick. one of TANK_A, TANK_B and TANK_C names the
 * tank. TANK_SHAKEN is set while a hit shakes it. the end of the first hit's
 * shake sets TANK_CRACKED, the second's TANK_HIT_TWICE, and the third's
 * TANK_BURST */
#define TANK_A 0x1
#define TANK_B 0x2
#define TANK_C 0x4
#define TANK_SHAKEN 0x8
#define TANK_CRACKED 0x10
#define TANK_HIT_TWICE 0x20
#define TANK_BURST 0x40

#define STATE(work) ((work)->tick)

/* the tank's work->scl, once its scale has chosen the tank. while a hit
 * shakes the tank, its parts draw moved by SHAKE's x and z. each TANK_SHAKE
 * step sets them before the parts draw: a fraction of the tank's offset from
 * player 0. in the step a hit lands, they are as the last step left them.
 * after an earlier shake, they are 0. on the first hit, x is the 1 TANK_INIT
 * stores, and z the placement's scale z */
#define SHAKE(work) ((work)->scl)
/* a flying part's work->scl: its speed, in units a step */
#define SPEED(work) ((work)->scl)

/* the tank's work->ang.z: the id of the Master Emerald piece it hides. until it
 * bursts, each step reports the piece at the tank to knuckles_find_piece. as it
 * bursts, knuckles_take_piece gives the piece to Knuckles */
#define PIECE(work) ((work)->ang.z)

/* what a flying part's upward speed loses each step */
#define FLY_FALL 0.06F
/* how far a flying part turns about each axis each step */
#define FLY_SPIN 0x500

/* the tank's work->mode. TANK_LEFT is its mode once it bursts */
enum tank_mode {
    TANK_INIT = 0,
    TANK_STAND = 1,
    TANK_SHAKE = 2,
    TANK_CRACK = 3,
    TANK_LEFT = 4,
    TANK_GONE = 5
};

/* a part's work->mode, numbered as tank_mode is, with no 3. the body shakes in
 * PART_SHAKE, and stays in PART_LEFT once the tank bursts */
enum part_mode {
    PART_INIT = 0,
    PART_STAND = 1,
    PART_SHAKE = 2,
    PART_LEFT = 4,
    PART_GONE = 5
};

enum fly_mode { FLY_START = 0, FLY_FALLING = 1 };

static void tank_part_draw(struct task *task);
static void tank_part_draw_shaken(struct task *task);
static void tank_part_fly(struct task *task);
static void tank_part_destroy(struct task *task);
static void tank_destroy(struct task *task);
static void tank_body_step(struct task *task);
static void tank_top_step(struct task *task);
static void tank_piece_step(struct task *task);
static void tank_add_pieces(struct task *task);
static void tank_side_step(struct task *task);
static void tank_add_side(struct task *task);
static void tank_pipe_step(struct task *task);
static void tank_add_pipe(struct task *task);

/* tank a's shapes are 0 to 2, tank b's 3 to 5, and tank c's 6 to 9. the last
 * of each is the tank's base, the one shape an attack can't hurt. a standing
 * tank's collision record holds the others. the record its burst makes holds
 * the base alone */
static struct collision_shape tank_shapes[10] = {
    {SHAPE_CYLINDER | SHAPE_HURTABLE | SHAPE_NO_PUSH,
     0,
     PUSH_ALL | PUSH_FIRM(PUSH_ALL),
     {0.0F, 8.12F, 0.0F},
     5.0F,
     8.12F,
     0.0F,
     {0},
     0,
     NULL},
    {SHAPE_CYLINDER | SHAPE_HURTABLE | SHAPE_NO_PUSH,
     0,
     PUSH_ALL | PUSH_FIRM(PUSH_ALL),
     {0.0F, 9.0F, 0.0F},
     2.0F,
     9.0F,
     0.0F,
     {0},
     0,
     NULL},
    {SHAPE_CYLINDER | SHAPE_NO_PUSH,
     SHAPE_GUARD,
     PUSH_ALL | PUSH_FIRM(PUSH_ALL),
     {0.0F, 2.5F, 0.0F},
     5.0F,
     2.5F,
     0.0F,
     {0},
     0,
     NULL},
    {SHAPE_SPHERE | SHAPE_HURTABLE | SHAPE_NO_PUSH,
     0,
     PUSH_ALL | PUSH_FIRM(PUSH_ALL),
     {0.0F, 11.0F, 0.0F},
     9.0F,
     0.0F,
     0.0F,
     {0},
     0,
     NULL},
    {SHAPE_CYLINDER | SHAPE_HURTABLE | SHAPE_NO_PUSH,
     0,
     PUSH_ALL | PUSH_FIRM(PUSH_ALL),
     {0.0F, 3.25F, 0.0F},
     6.75F,
     3.25F,
     0.0F,
     {0},
     0,
     NULL},
    {SHAPE_CYLINDER | SHAPE_NO_PUSH,
     SHAPE_GUARD,
     PUSH_ALL | PUSH_FIRM(PUSH_ALL),
     {0.0F, 3.25F, 0.0F},
     6.75F,
     3.25F,
     0.0F,
     {0},
     0,
     NULL},
    {SHAPE_CYLINDER | SHAPE_HURTABLE | SHAPE_NO_PUSH,
     0,
     PUSH_ALL | PUSH_FIRM(PUSH_ALL),
     {0.0F, 9.75F, 0.0F},
     6.0F,
     9.75F,
     0.0F,
     {0},
     0,
     NULL},
    {SHAPE_CYLINDER | SHAPE_HURTABLE | SHAPE_NO_PUSH,
     0,
     PUSH_ALL | PUSH_FIRM(PUSH_ALL),
     {0.0F, 10.5F, 0.0F},
     3.0F,
     10.5F,
     0.0F,
     {0},
     0,
     NULL},
    {SHAPE_CYLINDER | SHAPE_HURTABLE | SHAPE_NO_PUSH,
     0,
     PUSH_ALL | PUSH_FIRM(PUSH_ALL),
     {8.0F, 6.5F, 0.0F},
     0.8F,
     6.5F,
     0.0F,
     {0},
     0,
     NULL},
    {SHAPE_CYLINDER | SHAPE_NO_PUSH,
     SHAPE_GUARD,
     PUSH_ALL | PUSH_FIRM(PUSH_ALL),
     {0.0F, 1.74F, 0.0F},
     4.58F,
     1.74F,
     0.0F,
     {0},
     0,
     NULL},
};

/* each tank's four lid pieces, for task_create_children. a null step ends each
 * table. a spec's var sets the piece's MODEL(work)->object */
static struct task_spec tank_a_pieces[5] = {
    {tank_piece_step,
     TASK_WORK,
     0,
     {0, 0, 0},
     {2.0F, 7.0F, 3.0F},
     (Sint32)&tank_a_piece_1_object},
    {tank_piece_step,
     TASK_WORK,
     0,
     {0, 0, 0},
     {-3.0F, 12.0F, 3.0F},
     (Sint32)&tank_a_piece_2_object},
    {tank_piece_step,
     TASK_WORK,
     0,
     {0, 0, 0},
     {-4.0F, 10.0F, -2.0F},
     (Sint32)&tank_a_piece_3_object},
    {tank_piece_step,
     TASK_WORK,
     0,
     {0, 0, 0},
     {4.0F, 12.0F, 0.0F},
     (Sint32)&tank_a_piece_4_object},
    {NULL, 0, 0, {0, 0, 0}, {0.0F, 0.0F, 0.0F}, 0},
};
static struct task_spec tank_b_pieces[5] = {
    {tank_piece_step,
     TASK_WORK,
     0,
     {0, 0, 0},
     {2.0F, 11.0F, 3.0F},
     (Sint32)&tank_b_piece_1_object},
    {tank_piece_step,
     TASK_WORK,
     0,
     {0, 0, 0},
     {4.0F, 16.0F, 1.0F},
     (Sint32)&tank_b_piece_2_object},
    {tank_piece_step,
     TASK_WORK,
     0,
     {0, 0, 0},
     {0.0F, 11.0F, -5.0F},
     (Sint32)&tank_b_piece_3_object},
    {tank_piece_step,
     TASK_WORK,
     0,
     {0, 0, 0},
     {-6.0F, 12.0F, 0.0F},
     (Sint32)&tank_b_piece_4_object},
    {NULL, 0, 0, {0, 0, 0}, {0.0F, 0.0F, 0.0F}, 0},
};
static struct task_spec tank_c_pieces[5] = {
    {tank_piece_step,
     TASK_WORK,
     0,
     {0, 0, 0},
     {-3.0F, 7.0F, 3.0F},
     (Sint32)&tank_c_piece_1_object},
    {tank_piece_step,
     TASK_WORK,
     0,
     {0, 0, 0},
     {2.0F, 15.0F, 3.0F},
     (Sint32)&tank_c_piece_2_object},
    {tank_piece_step,
     TASK_WORK,
     0,
     {0, 0, 0},
     {4.0F, 9.0F, -2.0F},
     (Sint32)&tank_c_piece_3_object},
    {tank_piece_step,
     TASK_WORK,
     0,
     {0, 0, 0},
     {-4.0F, 13.0F, -2.0F},
     (Sint32)&tank_c_piece_4_object},
    {NULL, 0, 0, {0, 0, 0}, {0.0F, 0.0F, 0.0F}, 0},
};

/* draw a part at its own position, angles and scale. the parts are the body,
 * the lid or its pieces, the side and tank c's pipe. each is a child of the
 * tank */
static void tank_part_draw(struct task *task)
{
    struct task_work *work = task->work;

    texture_set_stage();
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    TURN(njRotateZ, work->ang.z);
    TURN(njRotateX, work->ang.x);
    TURN(njRotateY, work->ang.y);
    njScaleV(NULL, &work->scl);
    draw_model(MODEL(work)->object->model);
    njPopMatrix(1);
}

static void tank_part_draw_shaken(struct task *task)
{
    struct task_work *work = task->work;
    struct task_work *tank = task->parent->work;

    work->pos.x += SHAKE(tank).x;
    work->pos.z += SHAKE(tank).z;
    tank_part_draw(task);
    work->pos.x -= SHAKE(tank).x;
    work->pos.z -= SHAKE(tank).z;
}

/* fly a part off the burst tank. its first step gives it a random speed. it
 * rises 0.4 to 1.4 a step, and moves -0.5 to 0.5 along x and along z. it then
 * falls and tumbles until it leaves range */
static void tank_part_fly(struct task *task)
{
    struct task_work *work = task->work;

    switch (work->submode) {
    case FLY_START:
        work->submode++;
        SPEED(work).x = (rand() * RAND_SCALE) - 0.5F;
        SPEED(work).y = (rand() * RAND_SCALE) + 0.4F;
        SPEED(work).z = (rand() * RAND_SCALE) - 0.5F;
        break;
    case FLY_FALLING:
        if (placement_try_leave(task)) {
            work->submode++;
        }
        SPEED(work).y -= FLY_FALL;
        work->pos.x += SPEED(work).x;
        work->pos.y += SPEED(work).y;
        work->pos.z += SPEED(work).z;
        work->ang.x += FLY_SPIN;
        work->ang.y += FLY_SPIN;
        work->ang.z += FLY_SPIN;
        break;
    default:
        work->mode++;
        break;
    }
    tank_part_draw(task);
}

/* the parts' destructor, which tank_destroy calls for the tank too. outside
 * the set editor, it takes the task's object out of the collision world. it
 * then gives the object back to world_object_alloc's pool. neither call changes
 * anything: nothing here puts an object in the world or takes one from the
 * pool */
static void tank_part_destroy(struct task *task)
{
    struct task_work *work = task->work;

    if (!placement_editing(task) && MODEL(work)->object != NULL) {
        world_remove(task, MODEL(work)->object);
        world_object_free(MODEL(work)->object);
    }
}

static void tank_destroy(struct task *task)
{
    task_destroy_children(task);
    tank_part_destroy(task);
}

/* the body's step. it stays, drawn, once the tank bursts */
static void tank_body_step(struct task *task)
{
    struct task_work *work = task->work;
    struct task_work *tank = task->parent->work;

    switch (work->mode) {
    case PART_INIT:
        work->mode = PART_STAND;
        if (STATE(tank) & TANK_A) {
            MODEL(work)->object = &tank_a_body_object;
        } else if (STATE(tank) & TANK_B) {
            MODEL(work)->object = &tank_b_body_object;
        } else {
            MODEL(work)->object = &tank_c_body_object;
        }
        work->scl.x = work->scl.y = work->scl.z = 1.0F;
        work->ang.x = work->ang.z = 0;
        task->destructor = tank_part_destroy;
        task->draw = tank_part_draw;
        break;
    case PART_STAND:
        if (STATE(tank) & TANK_SHAKEN) {
            work->mode = PART_SHAKE;
        }
        draw_highlight_near(work, 1 << CHARACTER_KNUCKLES);
        tank_part_draw(task);
        draw_set_material(0.0F, 0.0F, 0.0F, 0.0F);
        break;
    case PART_SHAKE:
        if (STATE(tank) & TANK_BURST) {
            work->mode = PART_LEFT;
        } else if (!(STATE(tank) & TANK_SHAKEN)) {
            work->mode = PART_STAND;
        }
        tank_part_draw_shaken(task);
        break;
    case PART_LEFT:
        tank_part_draw(task);
        break;
    /* @hack redundant: case PART_GONE, which no code sets and which shares the
     * default's code, keeps CMP/EQ #5 at 0x0c90a9bc; without it, 72 of 102
     * words differ */
    case PART_GONE:
    default:
        tank_destroy(task);
        break;
    }
}

/* the lid's step. as the first hit's shake ends, the lid draws once more and
 * ends. its pieces take its place */
static void tank_top_step(struct task *task)
{
    struct task_work *work = task->work;
    struct task_work *tank = task->parent->work;

    switch (work->mode) {
    case PART_INIT:
        work->mode = PART_STAND;
        if (STATE(tank) & TANK_A) {
            MODEL(work)->object = &tank_a_top_object;
        } else if (STATE(tank) & TANK_B) {
            MODEL(work)->object = &tank_b_top_object;
        } else {
            MODEL(work)->object = &tank_c_top_object;
        }
        work->scl.x = work->scl.y = work->scl.z = 1.0F;
        work->ang.x = work->ang.z = 0;
        task->destructor = tank_part_destroy;
        task->draw = tank_part_draw;
        break;
    case PART_STAND:
        if (STATE(tank) & TANK_CRACKED) {
            tank_part_draw(task);
            tank_part_destroy(task);
            task_kill(task);
        } else if (STATE(tank) & TANK_SHAKEN) {
            tank_part_draw_shaken(task);
        } else {
            draw_highlight_near(work, 1 << CHARACTER_KNUCKLES);
            tank_part_draw(task);
            draw_set_material(0.0F, 0.0F, 0.0F, 0.0F);
        }
        break;
    }
}

/* a lid piece's step. it flies off as the tank bursts */
static void tank_piece_step(struct task *task)
{
    struct task_work *work = task->work;
    struct task_work *tank = task->parent->work;

    if (placement_try_leave(task)) {
        return;
    }
    switch (work->mode) {
    case PART_INIT:
        work->mode = PART_STAND;
        task->destructor = tank_part_destroy;
        task->draw = tank_part_draw;
        work->ang.x = work->ang.z = 0;
        work->scl.x = work->scl.y = work->scl.z = 1.0F;
        /* fall through */
    case PART_STAND:
        if (STATE(tank) & TANK_BURST) {
            tank_part_fly(task);
        } else if (STATE(tank) & TANK_CRACKED) {
            if (STATE(tank) & TANK_SHAKEN) {
                tank_part_draw_shaken(task);
            } else {
                draw_highlight_near(work, 1 << CHARACTER_KNUCKLES);
                tank_part_draw(task);
                draw_set_material(0.0F, 0.0F, 0.0F, 0.0F);
            }
        }
        break;
    }
}

static void tank_add_pieces(struct task *task)
{
    struct task_work *work = task->work;

    if (STATE(work) & TANK_A) {
        task_create_children(tank_a_pieces, task);
    } else if (STATE(work) & TANK_B) {
        task_create_children(tank_b_pieces, task);
    } else {
        task_create_children(tank_c_pieces, task);
    }
}

/* the side's step. tank_add_side gives it the tank's scale x, which chooses its
 * object. it flies off as the tank bursts */
static void tank_side_step(struct task *task)
{
    struct task_work *work = task->work;
    struct task_work *tank = task->parent->work;

    if (placement_try_leave(task)) {
        return;
    }
    switch (work->mode) {
    case PART_INIT:
        work->mode = PART_STAND;
        if (work->scl.x < TANK_B_SCALE) {
            MODEL(work)->object = &tank_a_side_object;
        } else if (work->scl.x < TANK_C_SCALE) {
            MODEL(work)->object = &tank_b_side_object;
        } else {
            MODEL(work)->object = &tank_c_side_object;
        }
        work->scl.x = work->scl.y = work->scl.z = 1.0F;
        work->ang.x = work->ang.z = 0;
        task->draw = tank_part_draw;
        task->destructor = tank_part_destroy;
        break;
    case PART_STAND:
        if (STATE(tank) & TANK_BURST) {
            tank_part_fly(task);
        } else if (STATE(tank) & TANK_SHAKEN) {
            tank_part_draw_shaken(task);
        } else {
            draw_highlight_near(work, 1 << CHARACTER_KNUCKLES);
            tank_part_draw(task);
            draw_set_material(0.0F, 0.0F, 0.0F, 0.0F);
        }
        break;
    }
}

static void tank_add_side(struct task *task)
{
    struct task_work *work = task->work;
    struct task *side;
    struct task_work *side_work;
    NJS_VECTOR offset;
    NJS_VECTOR moved;

    /* @hack spelling: the side assigned inside its test, which tests R0 at
     * 0x0c90adf6 and copies the side to R4 in the slot of the BT/S; assigned
     * first, 3 of 117 words differ */
    if ((side = task_create_child(TASK_WORK, tank_side_step, task)) != NULL) {
        /* @hack redundant: the side stored again as the first child, which
         * task_create_child made it, at 0x0c90adfe; without it, 9 of 117 words
         * differ */
        task->child = side;
        side_work = side->work;
        if (work->scl.x < TANK_B_SCALE) {
            offset.x = 6.0F;
            offset.y = 8.0F;
        } else if (work->scl.x < TANK_C_SCALE) {
            offset.x = 9.0F;
            offset.y = 8.0F;
        } else {
            offset.x = -7.0F;
            offset.y = 8.0F;
        }
        offset.z = 0.0F;
        njPushMatrix(&_unit_matrix_);
        TURN(njRotateZ, work->ang.z);
        TURN(njRotateX, work->ang.x);
        TURN(njRotateY, work->ang.y);
        njCalcVector(NULL, &offset, &moved);
        njPopMatrix(1);
        side_work->pos.x = work->pos.x + moved.x;
        side_work->pos.y = work->pos.y + moved.y;
        side_work->pos.z = work->pos.z + moved.z;
        side_work->ang.x = side_work->ang.z = 0;
        side_work->ang.y = work->ang.y;
        side_work->scl.x = work->scl.x;
    }
}

/* tank c's pipe's step. it flies off as the tank bursts */
static void tank_pipe_step(struct task *task)
{
    struct task_work *work = task->work;
    struct task_work *tank = task->parent->work;

    if (placement_try_leave(task)) {
        return;
    }
    switch (work->mode) {
    case PART_INIT:
        work->mode = PART_STAND;
        work->scl.x = work->scl.y = work->scl.z = 0.8F;
        work->ang.x = work->ang.z = 0;
        MODEL(work)->object = &tank_c_pipe_object;
        task->destructor = tank_part_destroy;
        break;
    case PART_STAND:
        if (STATE(tank) & TANK_BURST) {
            tank_part_fly(task);
        } else if (STATE(tank) & TANK_SHAKEN) {
            tank_part_draw_shaken(task);
        } else {
            draw_highlight_near(work, 1 << CHARACTER_KNUCKLES);
            tank_part_draw(task);
            draw_set_material(0.0F, 0.0F, 0.0F, 0.0F);
        }
        break;
    }
}

static void tank_add_pipe(struct task *task)
{
    /* @hack spelling: the threshold on the left, which loads the scale into FR3
     * first, at 0x0c90afec; as scl.x >= TANK_C_SCALE, 7 of 18 words differ */
    if (TANK_C_SCALE <= task->work->scl.x) {
        task_create_child(TASK_WORK, tank_pipe_step, task);
    }
}

/* a hit from Knuckles shakes the tank for eight steps. the tank takes no hit
 * while it shakes. after the first hit's shake, the lid cracks into its pieces.
 * after the third's, the tank bursts: it explodes, and its pieces, side and
 * pipe fly off */
void tank_step(struct task *task)
{
    struct task_work *work = task->work;

    if (placement_try_leave(task)) {
        return;
    }
    if (placement_editing(task)) {
        njPrintC(NJM_LOCATION(12, 5), "        XSCL < 10.0: TANK A");
        njPrintC(NJM_LOCATION(12, 6), "10.0 <= XSCL < 20.0: TANK B");
        njPrintC(NJM_LOCATION(12, 7), "20.0 <= XSCL       : TANK C");
        njPushMatrix(NULL);
        njTranslateV(NULL, &work->pos);
        TURN(njRotateY, work->ang.y);
        if (work->scl.x < TANK_B_SCALE) {
            njPrintC(NJM_LOCATION(10, 5), "*");
            texture_set_stage();
            draw_tree(&tank_a_body_object);
            draw_tree(&tank_a_top_object);
            draw_tree(&tank_a_side_object);
        } else if (work->scl.x < TANK_C_SCALE) {
            njPrintC(NJM_LOCATION(10, 6), "*");
            texture_set_stage();
            draw_tree(&tank_b_body_object);
            draw_tree(&tank_b_top_object);
            draw_tree(&tank_b_side_object);
        } else {
            njPrintC(NJM_LOCATION(10, 7), "*");
            texture_set_stage();
            draw_tree(&tank_c_body_object);
            draw_tree(&tank_c_top_object);
            draw_tree(&tank_c_pipe_object);
            draw_tree(&tank_c_side_object);
        }
        njPopMatrix(1);
        return;
    }
    switch (work->mode) {
    case TANK_INIT:
        work->mode = TANK_STAND;
        STATE(work) = 0;
        if (work->scl.x < TANK_B_SCALE) {
            STATE(work) |= TANK_A;
            collision_init(task, tank_shapes, COLLISION_OBJECT, 1, 2);
        } else if (work->scl.x < TANK_C_SCALE) {
            STATE(work) |= TANK_B;
            collision_init(task, &tank_shapes[3], COLLISION_OBJECT, 1, 2);
        } else {
            STATE(work) |= TANK_C;
            collision_init(task, &tank_shapes[6], COLLISION_OBJECT, 1, 3);
        }
        task->destructor = tank_destroy;
        work->ang.x = 0;
        task_create_child(TASK_WORK, tank_body_step, task);
        tank_add_side(task);
        tank_add_pipe(task);
        task_create_child(TASK_WORK, tank_top_step, task);
        work->scl.x = 1.0F;
        knuckles_find_piece(PIECE(work), &work->pos);
        /* @hack spelling: a goto to the end in place of a return, after which
         * the next case loads placement_editing's address into R2 at
         * 0x0c90b258; as a return, 2 of 549 words differ */
        goto done;
    case TANK_STAND:
        if (!placement_editing(task)) {
            work->submode = 0;
            if ((work->flags & COLLISION_DAMAGED) &&
                PLAYER_VARS(work->collision->hit->owner->work)->character ==
                    CHARACTER_KNUCKLES) {
                work->mode = TANK_SHAKE;
                STATE(work) |= TANK_SHAKEN;
                sound_play(SE_PUNCH, 0, 0, 0);
            }
            collision_register(work);
        }
        knuckles_find_piece(PIECE(work), &work->pos);
        break;
    case TANK_SHAKE: {
        NJS_POINT3 pos;
        Float shake;

        knuckles_find_piece(PIECE(work), &work->pos);
        if (STATE(work) & TANK_SHAKEN) {
            switch (work->submode++) {
            case 0:
            case 7:
                shake = 0.1F;
                break;
            case 1:
            case 2:
            case 5:
            case 6:
                shake = 0.2F;
                break;
            case 3:
            case 4:
                shake = 0.25F;
                break;
            case 8:
                STATE(work) &= ~TANK_SHAKEN;
                if (STATE(work) & TANK_HIT_TWICE) {
                    work->mode = TANK_LEFT;
                    STATE(work) |= TANK_BURST;
                    explosion_create_at(work->pos.x, work->pos.y + 5.0F,
                                        work->pos.z, 1.0F);
                    knuckles_take_piece(PIECE(work), &work->pos);
                    /* @bug memory: collision_init gives the tank a second
                     * record, of its base alone. the first record leaks, with
                     * its shapes: nothing frees it (JSR at 0x0c90b3a8, and at
                     * 0x0c90b400 for the other two) */
                    if (STATE(work) & TANK_A) {
                        collision_init(task, &tank_shapes[2], COLLISION_OBJECT,
                                       1, 1);
                    } else if (STATE(work) & TANK_B) {
                        collision_init(task, &tank_shapes[5], COLLISION_OBJECT,
                                       1, 1);
                    } else {
                        collision_init(task, &tank_shapes[9], COLLISION_OBJECT,
                                       1, 1);
                    }
                } else if (STATE(work) & TANK_CRACKED) {
                    work->mode = TANK_STAND;
                    STATE(work) |= TANK_HIT_TWICE;
                } else {
                    STATE(work) |= TANK_CRACKED;
                    work->mode = TANK_CRACK;
                }
                /* fall through */
            default:
                shake = 0.0F;
                break;
            }
            SHAKE(work).x = shake * (work->pos.x - player_work[0]->pos.x);
            SHAKE(work).z = shake * (work->pos.z - player_work[0]->pos.z);
        } else {
            work->mode = TANK_STAND;
        }
        /* a flash each step of the shake, at the world's origin. hit_create
         * ignores pos, 5 above the midpoint of the tank and player 0 */
        pos = work->pos;
        njAddVector(&pos, &player_work[0]->pos);
        pos.x *= 0.5F;
        pos.y *= 0.5F;
        pos.z *= 0.5F;
        pos.y += 5.0F;
        hit_create(&pos, 1.0F);
        collision_register(work);
        /* @hack spelling: the call here and a goto past the one after the
         * switch, in place of a break, which keep the case's own call of
         * collision_register at 0x0c90b4da and give 1 R8 and
         * knuckles_find_piece's address R9 (0x0c90b18e, 0x0c90b196); as a
         * break, the case shares TANK_LEFT's call, the two registers trade, and
         * 57 of 549 words differ */
        task_step_children(task);
        goto done;
    }
    case TANK_CRACK: {
        NJS_VECTOR to;
        int bearing;
        Angle turn;

        work->mode = TANK_STAND;
        /* the pieces' layout turns with the tank as tank_add_pieces makes
         * them. bearing is player 0's yaw from the tank's +z. turn is that yaw
         * to the nearest sixth of a turn. the layout's +z then faces
         * player 0 to the nearest sixth */
        to.x = player_work[0]->pos.x - work->pos.x;
        to.z = player_work[0]->pos.z - work->pos.z;
        bearing = -(int)(atan2f(to.z, to.x) * ANGLE_PER_RADIAN) - work->ang.y +
                  0x4000;
        bearing = (Uint16)bearing;
        /* past 0xeaaa, the nearest sixth is the whole turn: no turn at all */
        if (bearing > 0xeaaa) {
            turn = 0;
        } else if (bearing > 0xc000) {
            turn = 0xd555;
        } else if (bearing > 0x9555) {
            turn = 0xaaaa;
        } else if (bearing > 0x6aaa) {
            turn = 0x8000;
        } else if (bearing > 0x4000) {
            turn = 0x5555;
        } else if (bearing > 0x1555) {
            turn = 0x2aaa;
        } else {
            turn = 0;
        }
        work->ang.y += turn;
        tank_add_pieces(task);
        work->ang.y -= turn;
        collision_register(work);
        knuckles_find_piece(PIECE(work), &work->pos);
        break;
    }
    case TANK_LEFT:
        collision_register(work);
        break;
    /* @hack redundant: case TANK_GONE, which no code sets and which shares the
     * default's code, keeps CMP/EQ #5 at 0x0c90b1ba; without it, 115 of 549
     * words differ */
    case TANK_GONE:
    default:
        return;
    }
    task_step_children(task);
done:;
}
