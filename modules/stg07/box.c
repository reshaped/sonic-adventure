/* HakoAF, a box that breaks into its six pieces once something hits it. the
 * pieces fly apart and away from player 0, shrinking as they spread. they
 * rise, then fall. once they fall faster than BOX_GONE_SPEED, the box is
 * gone for good */
#include <mathf.h>
#include <ninja.h>
#include <sg_xpt.h>

#include "collision/collision.h"
#include "debug/editor.h"
#include "draw/model.h"
#include "object/common.h"
#include "object/placement.h"
#include "player/player.h"
#include "stg07/box.h"
#include "system/task.h"
#include "system/texture.h"
#include "system/util.h"

extern NJS_MODEL box_piece0_model;
extern NJS_MODEL box_piece1_model;
extern NJS_MODEL box_piece2_model;
extern NJS_MODEL box_piece3_model;
extern NJS_MODEL box_piece4_model;
extern NJS_MODEL box_piece5_model;

struct box_vars {
    Uint8 unk_00[4];
    Angle away; /* the way the pieces fly, away from player 0 */
};

#define VARS(work) ((struct box_vars *)&(work)->vars)

/* how far the pieces have spread, and how far they rise a step */
#define BOX_SPREAD(work) ((work)->scl.x)
#define BOX_RISE(work) ((work)->scl.y)

/* each step, the pieces spread BOX_SPREAD_STEP, and their rise falls by
 * BOX_GRAVITY. once the rise is below BOX_GONE_SPEED, the box is gone */
#define BOX_SPREAD_STEP 1.0F
#define BOX_GRAVITY 0.2F
#define BOX_GONE_SPEED (-10.0F)

/* a piece moves BOX_AWAY times the spread away from player 0. it also moves
 * the spread toward its own angle, and shrinks BOX_SHRINK for each unit of
 * spread */
#define BOX_AWAY 2.5F
#define BOX_SHRINK 0.05F

/* clang-format off */
static struct collision_shape box_shape = {
    SHAPE_RECTANGLE2, 0, PUSH_FIRM(PUSH_ENEMIES | PUSH_OTHERS),
    {0.0F, 8.0F, 0.0F}, 8.0F, 8.0F, 8.0F, {0}, 0, NULL,
};
/* clang-format on */

static void box_destroy(struct task *task)
{
}

/* draw a piece at x, y and z in the box, with ang its own angle */
static void box_draw_piece(struct task_work *work, Angle ang, Float x, Float y,
                           Float z, NJS_MODEL *model)
{
    Float dx;
    Float dz;
    Float scale;

    if (BOX_SPREAD(work) == 0.0F) {
        njPushMatrix(NULL);
        njTranslate(NULL, x, y, z);
        draw_model(model);
        njPopMatrix(1);
    } else {
        dx = (njSin(VARS(work)->away) * BOX_SPREAD(work) * BOX_AWAY) +
             (njSin(ang) * BOX_SPREAD(work));
        dz = (-njCos(VARS(work)->away) * BOX_SPREAD(work) * BOX_AWAY) +
             (njCos(ang) * BOX_SPREAD(work));
        scale = 1.0F - (BOX_SPREAD(work) * BOX_SHRINK);
        njPushMatrix(NULL);
        njTranslate(NULL, x + dx, y + BOX_RISE(work), z + dz);
        njScale(NULL, scale, scale, scale);
        draw_model(model);
        njPopMatrix(1);
    }
}

static void box_draw(struct task *task)
{
    struct task_work *work = task->work;
    Angle ang;

    texture_set_stage();
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    TURN_VIA(njRotateZ, work->ang.z, ang);
    TURN_VIA(njRotateY, work->ang.y, ang);
    box_draw_piece(work, 0xe000, -5.699998F, 7.499999F, 3.799999F,
                   &box_piece0_model);
    box_draw_piece(work, 0xa000, -3.8F, 7.499996F, -3.899999F,
                   &box_piece1_model);
    box_draw_piece(work, 0x5000, 3.7F, 7.499999F, -2.799999F,
                   &box_piece2_model);
    box_draw_piece(work, 0x1000, 1.8F, 5.5F, 3.7F, &box_piece3_model);
    box_draw_piece(work, 0x6000, 4.0F, 7.3F, -6.5F, &box_piece4_model);
    box_draw_piece(work, 0x3000, 2.0F, 13.200004F, 3.7F, &box_piece5_model);
    njPopMatrix(1);
}

/* the step of a broken box */
static void box_scatter(struct task *task)
{
    struct task_work *work = task->work;

    if (placement_try_leave(task)) {
        return;
    }
    if (BOX_RISE(work) < BOX_GONE_SPEED) {
        placement_kill(task);
        return;
    }
    BOX_SPREAD(work) += BOX_SPREAD_STEP;
    BOX_RISE(work) -= BOX_GRAVITY;
    work->pos.y += BOX_RISE(work);
    box_draw(task);
}

/* the step of a whole box. it is the box's draw too, so it tests for a hit
 * even while the game is paused. a hit sends the pieces away from player 0,
 * and breaks the box */
static void box_wait(struct task *task)
{
    struct task_work *work = task->work;
    NJS_POINT3 *player = &player_work[0]->pos;

    if (placement_try_leave(task)) {
        return;
    }
    box_draw(task);
    if (work->collision->flags & COLLISION_HIT) {
        VARS(work)->away =
            ((Angle)(atan2f(work->pos.z - player->z, work->pos.x - player->x) *
                     ANGLE_PER_RADIAN) +
             work->ang.y + 0x4000) &
            0xffff;
        task->step = box_scatter;
        task->draw = box_draw;
    }
    if (!placement_editing(task)) {
        collision_register(work);
        object_clear_touch(work, NULL);
    }
}

void box_step(struct task *task)
{
    struct task_work *work = task->work;

    BOX_SPREAD(work) = 0.0F;
    BOX_RISE(work) = 0.5F;
    VARS(work)->away = 0;
    collision_init(task, &box_shape, COLLISION_OBJECT, 1, 1);
    task->destructor = box_destroy;
    task->step = box_wait;
    task->draw = box_wait;
}
