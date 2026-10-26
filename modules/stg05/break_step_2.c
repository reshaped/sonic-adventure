/* O BREAKSTEP2, runs of rock blocks, with the same code as O BREAKSTEP. a
 * block landed on shakes and falls, and its neighbors fall after it */
#include <ninja.h>
#include <sg_xpt.h>
#include <stdlib.h>

#include "collision/collision.h"
#include "debug/editor.h"
#include "draw/model.h"
#include "effect/dust.h"
#include "object/placement.h"
#include "sound/sound.h"
#include "stg05/break_step_2.h"
#include "system/task.h"
#include "system/util.h"

#define TEXT_1_LENGTH 10
#include "stg05/break_step_2/text.inc"

extern NJS_TEXLIST obj_mountain_texlist;
extern NJS_MODEL break_step_piece_0_model;
extern NJS_MODEL break_step_piece_1_model;
extern NJS_MODEL break_step_piece_2_model;
extern NJS_MODEL break_step_piece_3_model;
extern NJS_MODEL break_step_piece_4_model;
extern NJS_MODEL break_step_piece_5_model;
extern NJS_MODEL break_step_piece_6_model;
extern NJS_MODEL break_step_piece_7_model;
extern NJS_MODEL break_step_piece_8_model;
extern NJS_MODEL break_step_piece_9_model;
extern NJS_MODEL break_step_piece_10_model;
extern NJS_MODEL break_step_piece_11_model;

/* a block's fall and its wobble. break_step_2_block_init puts it on the heap,
 * and break_step_2_block_destroy frees it */
struct break_step_2_fall {
    NJS_VECTOR speed;
    NJS_VECTOR spin; /* the turns each step about x and z, in angle units */
    /* the turns so far about x and z, which the draw takes while the block
     * falls */
    Angle ang_x;
    Angle ang_z;
    /* the wobble's offset this step along x and z, in tenths of a unit */
    Sint8 wobble_x;
    Sint8 wobble_z;
    /* the wobble's largest offset along x and z, in tenths of a unit */
    Sint8 amplitude_x;
    Sint8 amplitude_z;
};

/* a run's work->vars. the run's work->submode counts its blocks landed on */
struct break_step_2_run_vars {
    /* the run's blocks, four to a row, from the run's +z end.
     * break_step_2_run_destroy frees the array */
    struct task **blocks;
    Uint8 unk_04[4];
    Sint16 rows;
};

/* a block's work->vars. its work->tick counts down the steps left in a
 * wobble. its TASK_TIMER(work) counts the steps it has shaken, and then those
 * it has fallen */
struct break_step_2_block_vars {
    /* @hack type: the block's setup, its piece of the pattern and its
     * throws along x and z, at the indexes below, in an array of bytes. a
     * throw is a Sint8 that sets the block's speed and spin as it starts to
     * fall. as a Uint8 and two Sint8 fields, the run's build is 12 bytes
     * shorter and 147 of its 187 words differ */
    Uint8 setup[4];
    Uint8 unk_04[2];
    /* the phase of the block's quake, as an angle. the draw bobs the block by
     * up to 0.5 at this phase. it does so once any block of its run is landed
     * on, until the block falls */
    Sint16 quake;
    struct break_step_2_fall *fall;
};

#define RUN(work) ((struct break_step_2_run_vars *)&(work)->vars)
#define BLOCK(work) ((struct break_step_2_block_vars *)&(work)->vars)

#define PIECE 0
#define THROW_X 2
#define THROW_Z 3

/* the modes of the object, its runs and their blocks. only a block reaches
 * BREAK_STEP_GONE, once it has fallen for BLOCK_FALL_STEPS */
enum break_step_2_mode { BREAK_STEP_INIT, BREAK_STEP_RUN, BREAK_STEP_GONE };

/* in a block's work->submode */
enum break_step_2_block_state { BLOCK_WAIT, BLOCK_SHAKE, BLOCK_FALLING };

/* the object destroys itself once neither the camera nor player 0 is
 * within 550 of it. player 1 as Sonic within 40 also keeps it. the range is
 * squared, as placement_try_leave_by takes it */
#define BREAK_STEP_LEAVE_RANGE (550.0F * 550.0F)

/* the set entry's scale x gives the number of runs, and its scale y the rows
 * in each. each is clamped to 1 to BREAK_STEP_MOST */
#define BREAK_STEP_MOST 50

/* break_step_2_pieces is a pattern of three rows of four, 60 units long along
 * z. a run repeats the pattern along -z, a row at a time */
#define BREAK_STEP_PATTERN_LENGTH 60.0F
#define BREAK_STEP_ROW_LENGTH (BREAK_STEP_PATTERN_LENGTH * (1.0F / 3.0F))

/* a block landed on shakes for BLOCK_SHAKE_STEPS before it falls. a block
 * falls for BLOCK_FALL_STEPS */
#define BLOCK_SHAKE_STEPS 30
#define BLOCK_FALL_STEPS 240
/* a block falls once a neighbor has fallen for more than BLOCK_PULL_STEPS */
#define BLOCK_PULL_STEPS 8

/* the share of its speed and spin a block keeps each step */
#define BLOCK_DRAG 0.98F
/* the downward speed a falling block gains each step */
#define BLOCK_GRAVITY (98.0F / (30.0F * 30.0F))
/* a block's wobble */
#define BLOCK_WOBBLE_CHANCE 0.005F /* its chance of starting each step */
#define BLOCK_WOBBLE_STEPS 20 /* the steps it lasts */
#define BLOCK_WOBBLE_TURNS 0.15F /* the turns its wave makes a step */
/* its largest amplitude along x and z, either way, in tenths of a unit */
#define BLOCK_WOBBLE_MOST 5.0F
/* each unit of a throw gives a falling block THROW_SPEED of speed along the
 * throw's axis. the block's angles turn that axis. a unit also gives
 * THROW_SPIN angle units a step of spin, which tips the block toward the
 * throw. a random 0.7 to 1.0 scales each */
#define THROW_SPEED 0.24F
#define THROW_SPIN 768
/* the sizes of the throws */
#define THROW_OUTER 4 /* along x, from an outer column */
#define THROW_INNER 2 /* along x, from an inner column */
/* along z, from a pattern's first row or a short run's last */
#define THROW_END 3

struct break_step_2_piece {
    NJS_POINT3 pos; /* in the pattern */
    NJS_MODEL *model;
};

static void break_step_2_init(struct task *task);
static void break_step_2_assemble(struct task *task);
static void break_step_2_destroy(struct task *task);
static void break_step_2_run_step(struct task *task);
static void break_step_2_run_init(struct task *task);
static void break_step_2_run_build(struct task *task);
static void break_step_2_run_pull(struct task *task);
static void break_step_2_run_destroy(struct task *task);
static void break_step_2_block_step(struct task *task);
static void break_step_2_block_init(struct task *task);
static void break_step_2_block_move(struct task *task);
static void break_step_2_block_draw(struct task *task);
static void break_step_2_block_destroy(struct task *task);
static void break_step_2_block_drop(struct task *task, NJS_VECTOR *push);

/* four names of the rock texture ai128_iwa7, and their texlist. nothing reads
 * either. OBJ_MOUNTAIN, whose textures the blocks draw with, doesn't hold
 * the texture */
static NJS_TEXNAME break_step_2_textures[4] = {
    {TEXT_1, 0, 0},
    {TEXT_1, 0, 0},
    {TEXT_1, 0, 0},
    {TEXT_1, 0, 0},
};
static NJS_TEXLIST break_step_2_texlist = {
#include "stg05/break_step_2/break_step_2_texlist.inc"
};

static struct break_step_2_piece break_step_2_pieces[12] = {
    {{-14.0F, -6.0F, 20.0F}, &break_step_piece_0_model},
    {{-4.0F, -7.0F, 18.0F}, &break_step_piece_1_model},
    {{5.9999972F, -7.0F, 20.0F}, &break_step_piece_2_model},
    {{14.9999972F, -8.0F, 20.0F}, &break_step_piece_3_model},
    {{-15.0F, -7.0F, 0.0F}, &break_step_piece_4_model},
    {{-4.0F, -7.0F, -1.0F}, &break_step_piece_5_model},
    {{3.9999972F, -7.0F, 0.0F}, &break_step_piece_6_model},
    {{14.9999972F, -7.0F, 0.0F}, &break_step_piece_7_model},
    {{-15.0F, -7.0F, -20.0F}, &break_step_piece_8_model},
    {{-6.0F, -7.0F, -20.0F}, &break_step_piece_9_model},
    {{3.9999972F, -7.0F, -20.0F}, &break_step_piece_10_model},
    {{14.9999972F, -6.0F, -19.0F}, &break_step_piece_11_model},
};

static struct collision_shape break_step_2_block_shape = {
    SHAPE_RECTANGLE,
    0,
    PUSH_ALL | PUSH_FIRM(PUSH_ALL),
    {0.0F, -2.0F, 0.0F},
    5.0F,
    9.0F,
    10.0F,
    {0, 0, 0},
    0,
    NULL,
};

/* the specs of a block's task and a run's. break_step_2_run_build and
 * break_step_2_assemble set pos before each create */
static struct task_spec break_step_2_block_spec[2] = {
    {break_step_2_block_step, TASK_WORK, 0, {0, 0, 0}, {0.0F, 0.0F, 0.0F}, 0},
    {NULL, 0, 0, {0, 0, 0}, {0.0F, 0.0F, 0.0F}, 0},
};
static struct task_spec break_step_2_run_spec[2] = {
    {break_step_2_run_step, TASK_WORK, 0, {0, 0, 0}, {0.0F, 0.0F, 0.0F}, 0},
    {NULL, 0, 0, {0, 0, 0}, {0.0F, 0.0F, 0.0F}, 0},
};

static NJS_VECTOR break_step_2_pattern_offset = {0.0F, 0.0F,
                                                 -BREAK_STEP_PATTERN_LENGTH};

static NJS_VECTOR break_step_2_no_push = {0.0F, 0.0F, 0.0F};

/* create the runs and step them. in the set editor, the step creates the runs
 * again each step, and labels the scale fields */
void break_step_2_step(struct task *task)
{
    struct task_work *work = task->work;

    if (placement_try_leave_by(task, BREAK_STEP_LEAVE_RANGE)) {
        return;
    }
    if (!placement_editing(task)) {
        switch (work->mode) {
        case BREAK_STEP_INIT:
            break_step_2_init(task);
            break;
        case BREAK_STEP_RUN:
            task_step_children(task);
            break;
        }
    } else {
        switch (work->mode) {
        case BREAK_STEP_INIT:
            break_step_2_init(task);
            break;
        case BREAK_STEP_RUN:
            task_destroy_children(task);
            break_step_2_assemble(task);
            task_step_children(task);
            njPrintC(EDITOR_LABEL(EDITOR_ROW_SCL_X), "-> ASSEMBLES.");
            njPrintC(EDITOR_LABEL(EDITOR_ROW_SCL_Y), "-> BLOCKS");
            break;
        }
    }
}

static void break_step_2_init(struct task *task)
{
    struct task_work *work = task->work;

    task->draw = NULL;
    task->destructor = break_step_2_destroy;
    break_step_2_assemble(task);
    work->mode = BREAK_STEP_RUN;
}

/* create the runs back to back along -z, and step them */
static void break_step_2_assemble(struct task *task)
{
    struct task_work *work = task->work;
    int runs = work->scl.x;
    int i;
    int rows;
    struct task *child;

    if (runs < 1) {
        runs = 1;
    } else if (runs > BREAK_STEP_MOST) {
        runs = BREAK_STEP_MOST;
    }
    rows = work->scl.y;
    if (rows < 1) {
        rows = 1;
    } else if (rows > BREAK_STEP_MOST) {
        rows = BREAK_STEP_MOST;
    }
    for (i = 0; i < runs; i++) {
        break_step_2_run_spec[0].pos.x = 0.0F;
        break_step_2_run_spec[0].pos.y = 0.0F;
        break_step_2_run_spec[0].pos.z = -rows * i * BREAK_STEP_ROW_LENGTH;
        child = task_create_children(break_step_2_run_spec, task);
        child->work->scl.y = rows;
    }
    task_step_children(task);
}

static void break_step_2_destroy(struct task *task)
{
    task_destroy_children(task);
}

/* a run's step: outside the set editor, build the run, then pull its blocks
 * down as their neighbors fall */
static void break_step_2_run_step(struct task *task)
{
    struct task_work *work = task->work;

    if (!placement_editing(task)) {
        switch (work->mode) {
        case BREAK_STEP_INIT:
            break_step_2_run_init(task);
            break;
        case BREAK_STEP_RUN:
            break_step_2_run_pull(task);
            break;
        }
    }
}

static void break_step_2_run_init(struct task *task)
{
    struct task_work *work = task->work;

    task->draw = NULL;
    task->destructor = break_step_2_run_destroy;
    break_step_2_run_build(task);
    work->mode = BREAK_STEP_RUN;
}

/* create the run's blocks row by row, each at its piece's place in the
 * pattern. a block's throw along x points away from the run's middle. the
 * first row of each pattern has a throw toward +z. a row has a throw toward
 * -z when its index in its pattern equals rows - 1. that index is 0 to 2,
 * and rows counts the run's rows. so in a run of two or three rows, the last
 * row has one. in a longer run, no row does */
static void break_step_2_run_build(struct task *task)
{
    struct task_work *work = task->work;
    int rows;
    int count;
    int i;
    int row;
    int column;
    int piece;
    struct task **block;
    struct task *child;
    NJS_POINT3 offset;

    work->submode = 0;
    rows = work->scl.y;
    if (rows < 1) {
        RUN(work)->rows = 1;
    } else if (rows > BREAK_STEP_MOST) {
        RUN(work)->rows = BREAK_STEP_MOST;
    } else {
        RUN(work)->rows = rows;
    }
    count = RUN(work)->rows * 4;
    if (count) {
        RUN(work)->blocks =
            (struct task **)heap_alloc(count * sizeof(struct task *));
    } else {
        RUN(work)->blocks = NULL;
    }
    block = RUN(work)->blocks;
    offset.x = 0.0F;
    offset.y = 0.0F;
    offset.z = 0.0F;
    i = 0;
    for (;;) {
        piece = 0;
        for (row = 0; row < 3; row++) {
            for (column = 0; column < 4; column++) {
                if (i >= count) {
                    goto done;
                }
                break_step_2_block_spec[0].pos.x =
                    offset.x + break_step_2_pieces[piece].pos.x;
                break_step_2_block_spec[0].pos.y =
                    offset.y + break_step_2_pieces[piece].pos.y;
                break_step_2_block_spec[0].pos.z =
                    offset.z + break_step_2_pieces[piece].pos.z;
                child = task_create_children(break_step_2_block_spec, task);
                *block++ = child;
                BLOCK(child->work)->setup[PIECE] = piece;
                if (column == 0) {
                    BLOCK(child->work)->setup[THROW_X] = -THROW_OUTER;
                } else if (column == 3) {
                    BLOCK(child->work)->setup[THROW_X] = THROW_OUTER;
                } else if (column < 2) {
                    BLOCK(child->work)->setup[THROW_X] = -THROW_INNER;
                } else {
                    BLOCK(child->work)->setup[THROW_X] = THROW_INNER;
                }
                if (row == 0) {
                    BLOCK(child->work)->setup[THROW_Z] = THROW_END;
                } else if (row == RUN(work)->rows - 1) {
                    BLOCK(child->work)->setup[THROW_Z] = -THROW_END;
                } else {
                    BLOCK(child->work)->setup[THROW_Z] = 0;
                }
                piece++;
                i++;
            }
        }
        offset.x += break_step_2_pattern_offset.x;
        offset.y += break_step_2_pattern_offset.y;
        offset.z += break_step_2_pattern_offset.z;
    }
done:
    task_step_children(task);
}

/* drop each block once a neighbor has fallen for more than BLOCK_PULL_STEPS.
 * a block's neighbors are those beside it, ahead of it and behind it. each
 * drop passes a push away from the fallen block, which
 * break_step_2_block_drop doesn't read */
static void break_step_2_run_pull(struct task *task)
{
    struct task_work *work = task->work;
    struct task **block = RUN(work)->blocks;
    int rows = RUN(work)->rows;
    int row;
    int column;
    struct task *child;
    struct task_work *self;
    struct task_work *other;
    NJS_VECTOR push;

    for (row = 0; row < rows; row++) {
        for (column = 0; column < 4; column++) {
            child = *block;
            self = child->work;
            if (column < 3) {
                other = block[1]->work;
                if (other->submode == BLOCK_FALLING &&
                    TASK_TIMER(other) > BLOCK_PULL_STEPS) {
                    push.x = (self->pos.x - other->pos.x) * 0.05F;
                    push.y = (self->pos.y - other->pos.y) * 0.05F;
                    push.z = (self->pos.z - other->pos.z) * 0.05F;
                    break_step_2_block_drop(child, &push);
                }
            }
            if (column > 0) {
                other = block[-1]->work;
                if (other->submode == BLOCK_FALLING &&
                    TASK_TIMER(other) > BLOCK_PULL_STEPS) {
                    push.x = (self->pos.x - other->pos.x) * 0.05F;
                    push.y = (self->pos.y - other->pos.y) * 0.05F;
                    push.z = (self->pos.z - other->pos.z) * 0.05F;
                    break_step_2_block_drop(child, &push);
                }
            }
            if (row < rows - 1) {
                other = block[4]->work;
                if (other->submode == BLOCK_FALLING &&
                    TASK_TIMER(other) > BLOCK_PULL_STEPS) {
                    push.x = (self->pos.x - other->pos.x) * 0.05F;
                    push.y = (self->pos.y - other->pos.y) * 0.05F;
                    push.z = (self->pos.z - other->pos.z) * 0.05F;
                    break_step_2_block_drop(child, &push);
                }
            }
            if (row > 0) {
                other = block[-4]->work;
                if (other->submode == BLOCK_FALLING &&
                    TASK_TIMER(other) > BLOCK_PULL_STEPS) {
                    push.x = (self->pos.x - other->pos.x) * 0.05F;
                    push.y = (self->pos.y - other->pos.y) * 0.05F;
                    push.z = (self->pos.z - other->pos.z) * 0.05F;
                    break_step_2_block_drop(child, &push);
                }
            }
            block++;
        }
    }
    task_step_children(task);
}

static void break_step_2_run_destroy(struct task *task)
{
    task_destroy_children(task);
    if (RUN(task->work)->blocks) {
        heap_free((void *)RUN(task->work)->blocks);
    }
}

/* a block's step: draw and move the block until it reaches
 * BREAK_STEP_GONE */
static void break_step_2_block_step(struct task *task)
{
    struct task_work *work = task->work;

    switch (work->mode) {
    case BREAK_STEP_INIT:
        break_step_2_block_init(task);
        break;
    case BREAK_STEP_RUN:
        break_step_2_block_draw(task);
        break_step_2_block_move(task);
        break;
    case BREAK_STEP_GONE:
        break;
    }
}

static void break_step_2_block_init(struct task *task)
{
    struct task_work *work = task->work;
    struct break_step_2_fall *fall;

    work->mode = BREAK_STEP_RUN;
    work->submode = BLOCK_WAIT;
    BLOCK(work)->quake = 0;
    work->tick = 0;
    /* @hack spelling: heap_alloc's result tested in the assignment, which tests
     * R0 and copies it to R4 in the branch's slot (0x0c90ded8, 0x0c90dedc);
     * tested apart, the copy comes first and 3 of 74 words differ */
    if ((fall = heap_alloc(sizeof(struct break_step_2_fall))) == NULL) {
        njPrintC(NJM_LOCATION(3, 7), "Memory shortage");
        return;
    }
    BLOCK(work)->fall = fall;
    fall->speed.x = 0.0F;
    fall->speed.y = 0.0F;
    fall->speed.z = 0.0F;
    fall->spin.x = 0.0F;
    fall->spin.y = 0.0F;
    fall->spin.z = 0.0F;
    fall->ang_x = 0;
    fall->ang_z = 0;
    fall->wobble_x = 0;
    fall->wobble_z = 0;
    task->draw = break_step_2_block_draw;
    task->destructor = break_step_2_block_destroy;
    if (!placement_editing(task->parent)) {
        collision_init(task, &break_step_2_block_shape, COLLISION_OBJECT, 1, 1);
        work->collision->flags &= ~COLLISION_HIT;
    }
}

/* move and turn the block by its fall's speed and spin, then slow both by
 * BLOCK_DRAG. outside the set editor, a block landed on shakes, then falls,
 * raising dust. a block not wobbling starts a wobble with a chance of
 * BLOCK_WOBBLE_CHANCE each step */
static void break_step_2_block_move(struct task *task)
{
    struct task_work *work = task->work;
    struct break_step_2_fall *fall = BLOCK(work)->fall;
    NJS_VECTOR speed;
    Float wave;

    work->pos.x += fall->speed.x;
    work->pos.y += fall->speed.y;
    work->pos.z += fall->speed.z;
    fall->ang_x += (Angle)fall->spin.x;
    fall->ang_z += (Angle)fall->spin.z;
    fall->speed.x *= BLOCK_DRAG;
    fall->speed.z *= BLOCK_DRAG;
    fall->speed.y *= BLOCK_DRAG;
    fall->spin.x *= BLOCK_DRAG;
    fall->spin.y *= BLOCK_DRAG;
    fall->spin.z *= BLOCK_DRAG;
    if (!placement_editing(task->parent)) {
        if (work->submode == BLOCK_WAIT) {
            if (work->collision->flags & COLLISION_HIT) {
                work->submode = BLOCK_SHAKE;
                TASK_TIMER(work) = 0;
                task->parent->work->submode++;
            }
        } else if (work->submode == BLOCK_SHAKE) {
            sound_play_for(SE_RUBBLE_BLAST, (int)work, 1, 0, 30);
            TASK_TIMER(work)++;
            if (TASK_TIMER(work) >= BLOCK_SHAKE_STEPS) {
                break_step_2_block_drop(task, &break_step_2_no_push);
            }
        } else if (work->submode == BLOCK_FALLING) {
            if (TASK_TIMER(work) < BLOCK_FALL_STEPS) {
                if (rand() * RAND_SCALE < 0.1F) {
                    speed.x = fall->speed.x + (RANDOM(-1.0F, 1.0F) * 0.6F);
                    speed.z = fall->speed.z + (RANDOM(-1.0F, 1.0F) * 0.6F);
                    speed.y = fall->speed.y;
                    dust_create(&work->pos, &speed, 6.0F);
                }
                fall->speed.y -= BLOCK_GRAVITY;
                TASK_TIMER(work)++;
            } else {
                work->mode = BREAK_STEP_GONE;
            }
        }
        BLOCK(work)->quake += (int)RANDOM(6600.0F, 20600.0F);
        collision_register(work);
    }
    if (work->tick) {
        if (work->tick != 1) {
            wave = njSin((Angle)(work->tick * (65536.0F * BLOCK_WOBBLE_TURNS)));
            fall->wobble_x = fall->amplitude_x * wave;
            fall->wobble_z = fall->amplitude_z * wave;
        } else {
            fall->wobble_x = 0;
            fall->wobble_z = 0;
        }
        work->tick--;
    } else if (rand() * RAND_SCALE < BLOCK_WOBBLE_CHANCE) {
        fall->amplitude_x = RANDOM(-1.0F, 1.0F) * BLOCK_WOBBLE_MOST;
        fall->amplitude_z = RANDOM(-1.0F, 1.0F) * BLOCK_WOBBLE_MOST;
        work->tick = BLOCK_WOBBLE_STEPS;
    }
}

/* draw the block's piece, offset by its wobble. a falling block tumbles by its
 * fall's turns. before it falls, it bobs with its quake once any block of its
 * run has been landed on */
static void break_step_2_block_draw(struct task *task)
{
    struct task_work *work = task->work;
    struct break_step_2_fall *fall = BLOCK(work)->fall;
    Angle ang;

    if (!draw_in_view(work->pos.x, work->pos.y, work->pos.z, 10.0F)) {
        return;
    }
    njPushMatrix(NULL);
    njSetTexture(&obj_mountain_texlist);
    njTranslate(NULL, work->pos.x + (fall->wobble_x * 0.1F), work->pos.y,
                work->pos.z + (fall->wobble_z * 0.1F));
    TURN_VIA(njRotateZ, work->ang.z, ang);
    TURN_VIA(njRotateX, work->ang.x, ang);
    TURN_VIA(njRotateY, work->ang.y, ang);
    if (work->submode == BLOCK_FALLING) {
        /* @hack redundant: an empty else, which loads ang_x for its test into
         * R3 at 0x0c90e32a; without it, R2 takes it and 2 of 131 words differ
         */
        if (fall->ang_z != 0) {
            njRotateZ(NULL, fall->ang_z);
        } else {
        }
        /* @hack redundant: an empty else, which loads the task into R2 and its
         * parent into R3 at 0x0c90e3a0; without it, the two trade and 3 of 131
         * words differ */
        if (fall->ang_x != 0) {
            njRotateX(NULL, fall->ang_x);
        } else {
        }
    } else if (task->parent->work->submode != 0) {
        njTranslate(NULL, 0.0F, njSin(BLOCK(work)->quake) * 0.5F, 0.0F);
    }
    draw_model(break_step_2_pieces[BLOCK(work)->setup[PIECE]].model);
    njPopMatrix(1);
}

static void break_step_2_block_destroy(struct task *task)
{
    struct task_work *work = task->work;

    heap_free(BLOCK(work)->fall);
}

/* start the block falling, unless it already is. its throws set its speed
 * and spin, by THROW_SPEED and THROW_SPIN. push is not read */
static void break_step_2_block_drop(struct task *task, NJS_VECTOR *push)
{
    struct task_work *work = task->work;
    struct break_step_2_fall *fall = BLOCK(work)->fall;
    NJS_VECTOR speed;
    Angle ang;

    if (work->submode != BLOCK_FALLING) {
        speed.x = (Sint8)BLOCK(work)->setup[THROW_X] *
                  ((rand() * (0.3F * RAND_SCALE)) + 0.7F) * THROW_SPEED;
        speed.z = (Sint8)BLOCK(work)->setup[THROW_Z] *
                  ((rand() * (0.3F * RAND_SCALE)) + 0.7F) * THROW_SPEED;
        speed.y = 0.0F;
        njPushMatrix(&_unit_matrix_);
        TURN_VIA(njRotateZ, work->ang.z, ang);
        TURN_VIA(njRotateX, work->ang.x, ang);
        TURN_VIA(njRotateY, work->ang.y, ang);
        njCalcVector(NULL, &speed, &fall->speed);
        njPopMatrix(1);
        fall->spin.z = (-(Sint8)BLOCK(work)->setup[THROW_X] * THROW_SPIN) *
                       ((rand() * (0.3F * RAND_SCALE)) + 0.7F);
        fall->spin.x = ((Sint8)BLOCK(work)->setup[THROW_Z] * THROW_SPIN) *
                       ((rand() * (0.3F * RAND_SCALE)) + 0.7F);
        work->submode = BLOCK_FALLING;
        TASK_TIMER(work) = 0;
    }
}
