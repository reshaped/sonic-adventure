/* O Big Light lamp on a post, glaring in three directions */
#include <ninja.h>
#include <sg_xpt.h>

#include "collision/collision.h"
#include "debug/editor.h"
#include "debug/print.h"
#include "draw/tree.h"
#include "effect/glare.h"
#include "object/placement.h"
#include "stg10/big_light.h"
#include "system/task.h"
#include "system/texture.h"
#include "system/util.h"

extern NJS_OBJECT big_light_object;

/* the lamp is destroyed beyond 500 units of the camera and player 0 */
#define RANGE (500.0F * 500.0F)
/* the head, HEAD_Y up from the lamp's position and HEAD_Z ahead of it */
#define HEAD_Y 41.0F
#define HEAD_Z 30.0F

enum big_light_mode { BIG_LIGHT_INIT, BIG_LIGHT_RUN };

static void big_light_init(struct task *task);
static void big_light_draw(struct task *task);
static void big_light_destroy(struct task *task);

static struct collision_shape big_light_shape = {
    SHAPE_RECTANGLE,
    0,
    PUSH_ALL | PUSH_FIRM(PUSH_ALL),
    {0.0F, 6.0F, 0.0F},
    6.0F,
    6.0F,
    8.0F,
    {0},
    0,
    NULL,
};

/* where the glares start, from the head */
static NJS_POINT3 big_light_glare_pos = {0.0F, 1.06F, 22.7F};
static Float big_light_alpha = 12.0F;
static Float big_light_red = 0.5F;
static Float big_light_green = 0.4F;
static Float big_light_blue = 0.7F;
static Float big_light_glare_size = 2.0F;

/* @hack spelling: collision_register through an inline function, which
 * reads task->work again at each call and keeps task at @R15 (0x0c911544 to
 * 0x0c911548, 0x0c91158c to 0x0c911590). called with big_light_step's own
 * work, 63 of its 78 words differ */
static void big_light_register(struct task *task)
{
    collision_register(task->work);
}

/* the set editor labels all three angles, though only angle y turns the
 * lamp */
void big_light_step(struct task *task)
{
    struct task_work *work = task->work;

    if (placement_try_leave_by(task, RANGE)) {
        return;
    }
    if (!placement_editing(task)) {
        switch (work->mode) {
        case BIG_LIGHT_INIT:
            big_light_init(task);
            break;
        case BIG_LIGHT_RUN:
            big_light_draw(task);
            big_light_register(task);
            break;
        }
    } else {
        switch (work->mode) {
        case BIG_LIGHT_INIT:
            big_light_init(task);
            break;
        case BIG_LIGHT_RUN:
            debug_print(EDITOR_LABEL(EDITOR_ROW_ANG_X), "-> ROTATE X");
            debug_print(EDITOR_LABEL(EDITOR_ROW_ANG_Y), "-> ROTATE Y");
            debug_print(EDITOR_LABEL(EDITOR_ROW_ANG_Z), "-> ROTATE Z");
            big_light_draw(task);
            big_light_register(task);
            break;
        }
    }
}

static void big_light_init(struct task *task)
{
    struct task_work *work = task->work;

    task->draw = big_light_draw;
    task->destructor = big_light_destroy;
    collision_init(task, &big_light_shape, COLLISION_OBJECT, 1, 1);
    work->mode = BIG_LIGHT_RUN;
}

/* the side glares point 30 degrees either side of ahead */
static void big_light_draw(struct task *task)
{
    struct task_work *work = task->work;
    NJS_VECTOR ahead = {0.0F, 0.0F, 1.0F};
    NJS_VECTOR left = {0.5F, 0.0F, 0.87F};
    NJS_VECTOR right = {-0.5F, 0.0F, 0.87F};
    NJS_ARGB color;
    /* @hack redundant: origin, initialized and never read, whose initializer
     * the image copies from the constants to sp+16 (0x0c911662 to
     * 0x0c911676); without it, the constants run 12 bytes short and 160 of 187
     * words differ */
    NJS_POINT3 origin = {0.0F, 0.0F, 0.0F};
    NJS_POINT3 pos;

    color.a = big_light_alpha;
    color.r = big_light_red;
    color.g = big_light_green;
    color.b = big_light_blue;
    texture_set_stage();
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    TURN(njRotateY, work->ang.y);
    draw_tree(&big_light_object);
    njPopMatrix(1);
    njPushMatrix(NULL);
    njUnitMatrix(NULL);
    njTranslate(NULL, work->pos.x, work->pos.y, work->pos.z);
    TURN(njRotateY, work->ang.y);
    njTranslate(NULL, 0.0F, HEAD_Y, HEAD_Z);
    njCalcPoint(NULL, &big_light_glare_pos, &pos);
    njPopMatrix(1);
    njPushMatrix(NULL);
    njUnitMatrix(NULL);
    TURN(njRotateY, work->ang.y);
    njCalcVector(NULL, &ahead, &ahead);
    njCalcVector(NULL, &left, &left);
    njCalcVector(NULL, &right, &right);
    njPopMatrix(1);
    glare_draw(&pos, &ahead, big_light_glare_size, &color);
    glare_draw(&pos, &left, big_light_glare_size, &color);
    glare_draw(&pos, &right, big_light_glare_size, &color);
}

static void big_light_destroy(struct task *task)
{
}
