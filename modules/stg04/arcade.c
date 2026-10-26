/* O ARCADE01 to O ARCADE03 pavement awnings that bounce players up */
#include <ninja.h>

#include "collision/collision.h"
#include "debug/editor.h"
#include "draw/tree.h"
#include "object/common.h"
#include "object/placement.h"
#include "player/player.h"
#include "stg04/arcade.h"
#include "system/task.h"
#include "system/texture.h"
#include "system/util.h"

extern NJS_OBJECT arcade_1_object;
extern NJS_OBJECT arcade_2_object;
extern NJS_OBJECT arcade_3_object;

/* upward speed of a player's bounce off an awning */
#define ARCADE_BOUNCE 3.0F

enum arcade_mode { ARCADE_INIT, ARCADE_STAND };

static void arcade_destroy(struct task *task);
static void arcade_draw(struct task *task);
static void arcade_draw_at(struct task_work *work);

static struct collision_shape arcade_shapes[2] = {
    {SHAPE_RECTANGLE,
     0,
     PUSH_FIRM(PUSH_ENEMIES | PUSH_OTHERS),
     {10.0F, -4.0F, 0.0F},
     10.0F,
     2.0F,
     28.0F,
     {0},
     0,
     NULL},
    {SHAPE_RECTANGLE,
     0,
     PUSH_ALL | PUSH_FIRM(PUSH_ENEMIES | PUSH_OTHERS),
     {10.0F, -6.0F, 0.0F},
     10.0F,
     2.0F,
     28.0F,
     {0},
     0,
     NULL},
};

static NJS_OBJECT *arcade_objects[3] = {
    &arcade_1_object,
    &arcade_2_object,
    &arcade_3_object,
};

static void arcade_step(struct task *task, int kind);

void arcade_1_step(struct task *task)
{
    arcade_step(task, 0);
}

void arcade_2_step(struct task *task)
{
    arcade_step(task, 1);
}

void arcade_3_step(struct task *task)
{
    arcade_step(task, 2);
}

/* the step of the awning arcade_objects[kind]. on a step that finds
 * COLLISION_UNDER set, the awning clears it. it then bounces player 0 or 1
 * straight up at ARCADE_BOUNCE. that is the player whose task owns the record
 * the awning touched */
static void arcade_step(struct task *task, int kind)
{
    struct task_work *work = task->work;
    int player;

    if (!placement_try_leave(task)) {
        switch (work->mode) {
        case ARCADE_INIT:
            MODEL(work)->object = arcade_objects[kind];
            if (!placement_editing(task)) {
                collision_init(task, arcade_shapes, COLLISION_OBJECT, 1, 2);
            }
            task->destructor = arcade_destroy;
            task->draw = arcade_draw;
            work->mode = ARCADE_STAND;
            break;
        case ARCADE_STAND:
            break;
        }
        if (work->flags & COLLISION_UNDER) {
            work->flags &= ~COLLISION_UNDER;
            for (player = 0; player < 2; player++) {
                /* @hack spelling: player_get_task called before the hit's owner
                 * is read, which reads the owner into R1 after the call
                 * (0x0c903992); read first, the owner waits across the call
                 * in R8, which SHC then saves, and the function runs 4 bytes
                 * longer and 84 of 103 words differ */
                struct task *player_task = player_get_task(player);

                if (work->collision->hit->owner == player_task) {
                    player_bounce(player, 0.0F, ARCADE_BOUNCE, 0.0F);
                }
            }
        }
        arcade_draw_at(work);
        collision_register(work);
    }
}

static void arcade_destroy(struct task *task)
{
}

static void arcade_draw(struct task *task)
{
    arcade_draw_at(task->work);
}

static void arcade_draw_at(struct task_work *work)
{
    texture_set_stage();
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    TURN(njRotateY, work->ang.y);
    draw_tree(MODEL(work)->object);
    njPopMatrix(1);
}
