/* O BOOSTER, E-102's jet booster, shown as a spinning marker cube until he
 * picks it up */
#include <ninja.h>
#include <sg_xpt.h>

#include "adv0130/booster.h"
#include "collision/collision.h"
#include "debug/editor.h"
#include "draw/draw.h"
#include "draw/tree.h"
#include "object/placement.h"
#include "player/player.h"
#include "progress/progress.h"
#include "stage/stage.h"
#include "system/task.h"
#include "system/util.h"

/* the cube's turn about y a step, which work->timer sums. the cube's blue
 * is the turn's sine, made positive */
#define BOOSTER_SPIN 0x200

enum booster_mode { BOOSTER_INIT, BOOSTER_RUN };

static struct collision_shape booster_shapes[] = {
    {SHAPE_SPHERE,
     0,
     PUSH_FIRM(PUSH_ENEMIES | PUSH_OTHERS),
     {0.0F, 5.0F, 0.0F},
     0.0F,
     5.0F,
     0.0F,
     {0, 0, 0},
     0,
     NULL},
};

static void booster_draw(struct task *task);
static void booster_destroy(struct task *task);
static void booster_draw_work(struct task_work *work);
static Bool booster_wanted(void);

/* spin the cube until it is hit, which sets FLAG_E102_BOOSTER. for any
 * character but E-102, or once the flag is set, kill the placement */
void egg_carrier_booster_step(struct task *task)
{
    struct task_work *work = task->work;

    if (placement_try_leave(task)) {
        return;
    }
    if (!booster_wanted()) {
        placement_kill(task);
        return;
    }
    switch (work->mode) {
    case BOOSTER_INIT:
        if (!placement_editing(task)) {
            collision_init(task, booster_shapes, COLLISION_OBJECT, 1, 1);
        }
        task->destructor = booster_destroy;
        task->draw = booster_draw;
        work->mode = BOOSTER_RUN;
        break;
    case BOOSTER_RUN:
        work->timer += BOOSTER_SPIN;
        if (work->collision->flags & COLLISION_HIT) {
            event_flags[FLAG_E102_BOOSTER] = 1;
        }
        break;
    }
    booster_draw_work(work);
    if (!placement_editing(task)) {
        collision_register(work);
    }
}

static void booster_draw(struct task *task)
{
    booster_draw_work(task->work);
}

static void booster_destroy(struct task *task)
{
}

static void booster_draw_work(struct task_work *work)
{
    Angle ang;

    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    TURN_VIA(njRotateY, TASK_TIMER(work), ang);
    draw_set_material(1.0F, 1.0F, 0.0F,
                      _builtin_fabsf(njSin(TASK_TIMER(work))));
    njScale(NULL, 1.0F, 2.0F, 1.0F);
    draw_tree(marker_cube_object);
    njPopMatrix(1);
}

static Bool booster_wanted(void)
{
    return get_character() == CHARACTER_E102 &&
           event_flags[FLAG_E102_BOOSTER] == 0;
}
