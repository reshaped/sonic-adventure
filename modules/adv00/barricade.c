/* O BARRICADDE police barricade across the road, which blocks Sonic and Tails
 * until their stories clear it */
#include <ninja.h>
#include <sg_xpt.h>

#include "adv00/barricade.h"
#include "adv00/station_square.h"
#include "adv00/texlists.h"
#include "collision/collision.h"
#include "debug/editor.h"
#include "draw/tree.h"
#include "event/play.h"
#include "object/placement.h"
#include "player/player.h"
#include "progress/progress.h"
#include "stage/stage.h"
#include "system/task.h"
#include "system/texture.h"
#include "system/util.h"

extern NJS_OBJECT barricade_barrier_object;
extern NJS_OBJECT police_car_object;

enum barricade_mode { BARRICADE_UP, BARRICADE_GONE };

/* the collision record's radius. it reaches past the shapes, whose farthest
 * point lies about 383 from the barricade's position */
#define RECORD_RADIUS 420.0F

static void barricade_step(struct task *task);
static void barricade_draw(struct task *task);

/* a wall 200 units wide across the road, and a post 80 units to each side of
 * its center */
/* clang-format off */
static struct collision_shape barricade_shapes[] = {
    {SHAPE_RECTANGLE | SHAPE_NO_PUSH, 0, PUSH_FIRM(PUSH_ALL) | PUSH_ALL,
     {0.0F, 170.0F, 0.0F}, 100.0F, 200.0F, 3.0F, {0}, 0, NULL},
    {SHAPE_CYLINDER | SHAPE_NO_PUSH, 0, PUSH_FIRM(PUSH_ALL) | PUSH_ALL,
     {80.0F, 170.0F, 0.0F}, 20.0F, 200.0F, 0.0F, {0}, 0, NULL},
    {SHAPE_CYLINDER | SHAPE_NO_PUSH, 0, PUSH_FIRM(PUSH_ALL) | PUSH_ALL,
     {-80.0F, 170.0F, 0.0F}, 20.0F, 200.0F, 0.0F, {0}, 0, NULL},
};
/* clang-format on */

static Bool barricade_gone(void)
{
    switch (player_get_character(0)) {
    case CHARACTER_SONIC:
        if (!event_in_control()) {
            return event_flags[FLAG_SONIC_SS_BARRICADE];
        }
        return TRUE;
    case CHARACTER_TAILS:
        return event_flags[FLAG_TAILS_SS_BARRICADE];
    case CHARACTER_KNUCKLES:
    case CHARACTER_AMY:
    case CHARACTER_E102:
    case CHARACTER_BIG:
        return TRUE;
    }
    return FALSE;
}

void barricade_init(struct task *task)
{
    struct task_work *work = task->work;

    if ((Uint8)stage_get_stage_act() != STATION_SQUARE_STATION) {
        njPrintC(NJM_LOCATION(10, 8), "ACT STATION ONLY !");
        return;
    }
    if (!placement_editing(task)) {
        if (barricade_gone()) {
            work->mode = BARRICADE_GONE;
            task->draw = NULL;
        } else {
            work->mode = BARRICADE_UP;
            task->draw = barricade_draw;
        }
        collision_init(task, barricade_shapes, COLLISION_OBJECT, 1, 3);
        work->collision->radius = RECORD_RADIUS;
    }
    work->ang.x = 0;
    work->ang.z = 0;
    task->step = barricade_step;
}

static void barricade_step(struct task *task)
{
    struct task_work *work = task->work;

    if (placement_try_leave(task)) {
        return;
    }
    if (placement_editing(task)) {
        return;
    }
    switch (work->mode) {
    case BARRICADE_UP:
        collision_register(work);
        barricade_draw(task);
        if (barricade_gone()) {
            task->draw = NULL;
            work->mode = BARRICADE_GONE;
        }
        break;
    case BARRICADE_GONE:
        break;
    }
}

static void barricade_draw(struct task *task)
{
    struct task_work *work = task->work;
    Angle ang;

    njSetTexture(&sscar_texlist);
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    TURN_VIA(njRotateY, work->ang.y, ang);
    njTranslate(NULL, 0.0F, 0.0F, 30.0F);
    texture_set_stage();
    njPushMatrix(NULL);
    njTranslate(NULL, 50.0F, 0.0F, 0.0F);
    draw_tree(&barricade_barrier_object);
    njTranslate(NULL, -100.0F, 0.0F, 0.0F);
    draw_tree(&barricade_barrier_object);
    njPopMatrix(1);
    njSetTexture(&sscar_texlist);
    njRotateY(NULL, 0xc000);
    njTranslate(NULL, 0.0F, -4.0F, 18.0F);
    draw_tree(&police_car_object);
    njTranslate(NULL, 0.0F, 0.0F, -36.0F);
    draw_tree(&police_car_object);
    njPopMatrix(1);
}
