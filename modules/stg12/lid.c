/* the lid over the hole under act 0's upper pool. it breaks into falling
 * pieces as the pool starts to pour */
#include <ninja.h>
#include <sg_xpt.h>
#include <stdlib.h>

#include "draw/model.h"
#include "draw/tree.h"
#include "stg12/lid.h"
#include "stg12/upper_pool.h"
#include "stg12/water.h"
#include "system/frame.h"
#include "system/task.h"
#include "system/texture.h"
#include "system/util.h"

extern NJS_OBJECT shelter_lid_object;
extern NJS_OBJECT shelter_lid_pieces_object;

/* the lid breaks into PIECES pieces, one for each child of
 * shelter_lid_pieces_object. it stays broken for BROKEN_STEPS steps, counting
 * the one it breaks in, and kills itself in the last. its destructor destroys
 * the pieces */
#define PIECES 14
#define BROKEN_STEPS 400

/* the speed a piece gains downward each step */
#define PIECE_GRAVITY 0.1F
/* a piece turns about x and about y by fixed angles each step. it picks them
 * at random as it starts. PIECE_SPIN_X and PIECE_SPIN_Y are the widths of
 * their ranges, each centered on 0 */
#define PIECE_SPIN_X 3000.0F
#define PIECE_SPIN_Y 1800.0F

/* the lid's TASK_TIMER(work) counts down the steps it has left. the count
 * starts from BROKEN_STEPS as the lid breaks */
enum shelter_lid_mode { LID_WHOLE, LID_BROKEN };

/* a piece starts in PIECE_FALL and stays in it, so the step's PIECE_GONE
 * case never runs */
enum shelter_lid_piece_mode { PIECE_FALL = 2, PIECE_GONE };

/* node is the piece's child of shelter_lid_pieces_object, whose model the
 * piece draws. alpha starts at 0, and no code reads it */
struct shelter_lid_piece_vars {
    Uint8 unk_00[4];
    NJS_OBJECT *node;
    Float alpha;
};

struct shelter_lid_piece_motion {
    Uint8 unk_00[4];
    NJS_VECTOR speed;
    Uint8 unk_10[24];
    Angle spin_x;
    Angle spin_y;
};

#define PIECE(work) ((struct shelter_lid_piece_vars *)&(work)->vars)
#define PIECE_MOTION(task) ((struct shelter_lid_piece_motion *)(task)->motion)

static void shelter_lid_step(struct task *task);
static void shelter_lid_draw(struct task *task);
static void shelter_lid_destroy(struct task *task);
static void shelter_lid_piece_start(struct task *task);
static void shelter_lid_piece_step(struct task *task);
static void shelter_lid_piece_draw(struct task *task);
static void shelter_lid_piece_destroy(struct task *task);

void shelter_lid_start(struct task *task)
{
    struct task_work *work = task->work;

    TASK_TIMER(work) = 0;
    work->mode = LID_WHOLE;
    task->step = shelter_lid_step;
    task->draw = shelter_lid_draw;
    task->destructor = shelter_lid_destroy;
}

static void shelter_lid_step(struct task *task)
{
    struct task_work *work = task->work;
    struct task_work *piece;
    struct task *child;
    NJS_OBJECT *node;
    Float *at;
    Float x;
    Float y;
    Float z;
    Float dx;
    Float dy;
    Float dz;
    int i;

    switch (work->mode) {
    case LID_WHOLE:
        if ((shelter_big_flags & BIG_PULLED) &&
            shelter_water_state == SHELTER_WATER_RISEN) {
            node = shelter_lid_pieces_object.child;
            x = work->pos.x;
            y = work->pos.y;
            z = work->pos.z;
            for (i = 0; i < PIECES; i++) {
                /* @hack spelling: a cursor through node->pos, which SHC reads
                 * with FMOV.S @R4+ at 0x0c90fc8e and 0x0c90fc94; read as
                 * node->pos[0], [1] and [2], 73 of 124 words differ */
                at = node->pos;
                dx = *at++;
                dy = *at++;
                dz = *at++;
                child = task_create_child(TASK_WORK | TASK_MOTION,
                                          shelter_lid_piece_start, task);
                piece = child->work;
                piece->pos.x = x + dx;
                piece->pos.y = y + dy;
                piece->pos.z = z + dz;
                PIECE(piece)->node = node;
                node = node->sibling;
            }
            work->mode = LID_BROKEN;
            TASK_TIMER(work) = BROKEN_STEPS;
            task->draw = NULL;
        }
        shelter_lid_draw(task);
    /* @bug logic: LID_WHOLE's case falls through to the count at
     * 0x0c90fcd2, so the whole lid counts down too. the count wraps from 0 to
     * 65,535 (EXTU.W at 0x0c90fcda), and a lid that never breaks kills itself
     * 65,536 steps after it starts */
    case LID_BROKEN:
        TASK_TIMER(work)--;
        if (TASK_TIMER(work) <= 0) {
            task_kill(task);
        } else {
            task_step_children(task);
        }
        break;
    }
}

static void shelter_lid_draw(struct task *task)
{
    struct task_work *work = task->work;

    if (frame_step == 0) {
        texture_set_stage();
        njPushMatrix(NULL);
        njTranslateV(NULL, &work->pos);
        draw_tree(&shelter_lid_object);
        njPopMatrix(1);
    }
}

static void shelter_lid_destroy(struct task *task)
{
    task_destroy_children(task);
}

static void shelter_lid_piece_start(struct task *task)
{
    struct task_work *work = task->work;
    struct shelter_lid_piece_motion *motion = PIECE_MOTION(task);

    TASK_TIMER(work) = 0;
    PIECE(work)->alpha = 0.0F;
    work->mode = PIECE_FALL;
    motion->speed.x = 0.0F;
    motion->speed.y = 0.0F;
    motion->speed.z = 0.0F;
    motion->spin_y = (int)(((rand() * RAND_SCALE) - 0.5F) * PIECE_SPIN_Y);
    motion->spin_x = (int)(((rand() * RAND_SCALE) - 0.5F) * PIECE_SPIN_X);
    task->step = shelter_lid_piece_step;
    task->draw = shelter_lid_piece_draw;
    task->destructor = shelter_lid_piece_destroy;
    shelter_lid_piece_draw(task);
}

static void shelter_lid_piece_step(struct task *task)
{
    struct task_work *work = task->work;
    struct shelter_lid_piece_motion *motion = PIECE_MOTION(task);
    /* @hack spelling: cursors through work->pos and motion->speed. SHC reads
     * through them with FMOV.S @R14+ and @R13+ from 0x0c90fe30, and stores
     * through them from 0x0c90fe86. read and stored through the members, 55
     * of 78 words differ */
    Float *pos = &work->pos.x;
    Float *speed = &motion->speed.x;
    Float x = *pos++;
    Float y = *pos++;
    Float z = *pos++;
    Float vx = *speed++;
    Float vy = *speed++;
    Float vz = *speed++;

    pos -= 3;
    speed -= 3;
    switch (work->mode) {
    case PIECE_FALL:
        x += vx;
        y += vy;
        z += vz;
        vy -= PIECE_GRAVITY;
        work->ang.y += motion->spin_y;
        work->ang.x += motion->spin_x;
        shelter_lid_piece_draw(task);
        break;
    case PIECE_GONE:
        task->draw = NULL;
        break;
    }
    *pos++ = x;
    *pos++ = y;
    *pos++ = z;
    *speed++ = vx;
    *speed++ = vy;
    *speed++ = vz;
}

static void shelter_lid_piece_draw(struct task *task)
{
    struct task_work *work = task->work;
    Angle ang;

    if (frame_step == 0) {
        texture_set_stage();
        njPushMatrix(NULL);
        njTranslateV(NULL, &work->pos);
        TURN_VIA(njRotateX, work->ang.x, ang);
        TURN_VIA(njRotateY, work->ang.y, ang);
        draw_model(PIECE(work)->node->model);
        njPopMatrix(1);
    }
}

static void shelter_lid_piece_destroy(struct task *task)
{
}
