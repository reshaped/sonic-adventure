/* O SnowBoard, the stand player 0 takes the snowboard from. a player's touch
 * takes the board, and the stand's pieces then spin, sink and shrink away */
#include <ninja.h>
#include <sg_xpt.h>

#include "collision/collision.h"
#include "debug/editor.h"
#include "draw/model.h"
#include "object/common.h"
#include "object/placement.h"
#include "player/player.h"
#include "sound/sound.h"
#include "stage/stage.h"
#include "stg08/icecap.h"
#include "stg08/snow_board.h"
#include "system/pad.h"
#include "system/task.h"
#include "system/texture.h"
#include "system/util.h"

extern NJS_MODEL snow_board_post_model;
extern NJS_MODEL snow_board_plank_model;
extern NJS_MODEL snow_board_sign_top_1_model;
extern NJS_MODEL snow_board_sign_top_2_model;
extern NJS_MODEL snow_board_sign_top_3_model;
extern NJS_MODEL snow_board_sign_top_4_model;
extern NJS_MODEL snow_board_sign_top_5_model;
extern NJS_MODEL snow_board_sign_bottom_1_model;
extern NJS_MODEL snow_board_sign_bottom_2_model;
extern NJS_MODEL snow_board_sign_bottom_3_model;

/* in work->flags: the board has been taken */
#define BOARD_TAKEN 0x100

#define SE_BOARD_TAKEN 222

/* the pieces stand PIECES_START above the task until the board is taken.
 * they then fall, gaining PIECES_ACCEL of downward speed each step. the task
 * ends once their rise is below PIECES_GONE. their scale is 1 plus
 * PIECE_SHRINK times the rise, so they shrink to nothing as it nears
 * PIECES_GONE */
#define PIECES_START 3.0F
#define PIECES_ACCEL 0.2F
#define PIECES_GONE (-10.0F)
#define PIECE_SHRINK 0.1F
/* the turn the pieces gain each step once the board is taken. each piece
 * turns about z by its own multiple of it, -5 to 5. it turns half that about
 * x */
#define PIECES_TURN 0x200

/* the heights of the stand's three rows of pieces, from the pieces' height */
#define ROW_TOP 0.0F
#define ROW_MIDDLE (-6.0F)
#define ROW_BOTTOM (-13.0F)

struct snow_board_vars {
    Uint8 unk_00[4];
    Angle turn; /* the turn the pieces have gained */
};

#define VARS(work) ((struct snow_board_vars *)&(work)->vars)
/* the pieces' height above the task, and how far they rise a step: below 0
 * as they fall */
#define PIECES_HEIGHT(work) ((work)->scl.x)
#define PIECES_RISE(work) ((work)->scl.y)

/* a box around the stand: 20 units along its own x, 90 tall and 160 along
 * its z. its three-quarter turn about y lays the 160 along the stand's x */
static struct collision_shape snow_board_shape = {
    SHAPE_RECTANGLE,
    0,
    PUSH_FIRM(PUSH_ALL) | PUSH_ALL,
    {9.0F, 20.0F, 0.0F},
    10.0F,
    45.0F,
    80.0F,
    {0, 0xc000, 0},
    0,
    NULL,
};

static void snow_board_destroy(struct task *task)
{
}

/* draw a piece's model at x and y from the stand. it turns ang about z and
 * half that about x, and shrinks as rise falls below 0 */
static void snow_board_piece(Float rise, Angle ang, Float x, Float y,
                             NJS_MODEL *model)
{
    Float size = 1.0F + (rise * PIECE_SHRINK);

    njPushMatrix(NULL);
    njTranslate(NULL, x, y, 0.0F);
    TURN(njRotateZ, ang);
    TURN(njRotateX, ang / 2);
    njScale(NULL, size, size, size);
    draw_model(model);
    njPopMatrix(1);
}

/* the stand's step and its draw. the pieces move in the draw too, so they
 * fall on while the game is paused. a player's touch takes the board. the
 * run then stops player 0 and takes away its pad. it also stores
 * ICECAP_BOARD_TAKEN in the stage task's work->flags */
static void snow_board_run(struct task *task)
{
    struct task_work *work = task->work;
    Float height = PIECES_HEIGHT(work);

    if (placement_try_leave(task)) {
        return;
    }
    texture_set_stage();
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    TURN(njRotateY, work->ang.y);
    snow_board_piece(PIECES_RISE(work), VARS(work)->turn * 4, -20.0F,
                     height + ROW_TOP, &snow_board_post_model);
    snow_board_piece(PIECES_RISE(work), -VARS(work)->turn * 4, 20.0F,
                     height + ROW_TOP, &snow_board_post_model);
    snow_board_piece(PIECES_RISE(work), VARS(work)->turn * 3, -15.0F,
                     height + ROW_MIDDLE, &snow_board_plank_model);
    snow_board_piece(PIECES_RISE(work), -VARS(work)->turn * 3, 15.0F,
                     height + ROW_MIDDLE, &snow_board_plank_model);
    snow_board_piece(PIECES_RISE(work), VARS(work)->turn * 5, -25.0F,
                     height + ROW_MIDDLE, &snow_board_plank_model);
    snow_board_piece(PIECES_RISE(work), -VARS(work)->turn * 5, 25.0F,
                     height + ROW_MIDDLE, &snow_board_plank_model);
    snow_board_piece(PIECES_RISE(work), VARS(work)->turn * 2, -10.0F,
                     height + ROW_TOP, &snow_board_sign_top_1_model);
    snow_board_piece(PIECES_RISE(work), VARS(work)->turn, -5.0F,
                     height + ROW_TOP, &snow_board_sign_top_2_model);
    snow_board_piece(PIECES_RISE(work), VARS(work)->turn, -5.0F,
                     height + ROW_BOTTOM, &snow_board_sign_bottom_1_model);
    /* the top row's middle piece shows only until the board is taken */
    if (!(work->flags & BOARD_TAKEN)) {
        snow_board_piece(PIECES_RISE(work), 0, 0.0F, height + ROW_TOP,
                         &snow_board_sign_top_3_model);
    }
    snow_board_piece(PIECES_RISE(work), 0, 0.0F, height + ROW_BOTTOM,
                     &snow_board_sign_bottom_2_model);
    snow_board_piece(PIECES_RISE(work), -VARS(work)->turn, 5.0F,
                     height + ROW_TOP, &snow_board_sign_top_4_model);
    snow_board_piece(PIECES_RISE(work), -VARS(work)->turn, 5.0F,
                     height + ROW_BOTTOM, &snow_board_sign_bottom_3_model);
    snow_board_piece(PIECES_RISE(work), -VARS(work)->turn * 2, 10.0F,
                     height + ROW_TOP, &snow_board_sign_top_5_model);
    njPopMatrix(1);
    if (work->flags & BOARD_TAKEN) {
        if (PIECES_RISE(work) < PIECES_GONE) {
            placement_kill(task);
        } else {
            VARS(work)->turn += PIECES_TURN;
            PIECES_RISE(work) -= PIECES_ACCEL;
            height += PIECES_RISE(work);
            PIECES_HEIGHT(work) = height;
        }
    } else if ((work->collision->flags & COLLISION_HIT) &&
               work->collision->hit->kind == COLLISION_PLAYER) {
        /* BOARD_TAKEN is clear here, so the add sets it */
        work->flags += BOARD_TAKEN;
        pad_disable_player(0);
        player_clear_speed(player_motion[0], player_get(0));
        player_work[0]->flags &= ~SONIC_CHARGED;
        stage_task->work->flags = ICECAP_BOARD_TAKEN;
        sound_play(SE_BOARD_TAKEN, 0, 0, 0);
    } else if (!placement_editing(task)) {
        collision_register(work);
        object_clear_touch(work, NULL);
    }
}

void snow_board_step(struct task *task)
{
    struct task_work *work = task->work;

    collision_init(task, &snow_board_shape, COLLISION_OBJECT, 1, 1);
    work->flags &= ~BOARD_TAKEN;
    VARS(work)->turn = 0;
    PIECES_HEIGHT(work) = PIECES_START;
    PIECES_RISE(work) = 0.0F;
    task->step = snow_board_run;
    task->draw = snow_board_run;
    task->destructor = snow_board_destroy;
}
