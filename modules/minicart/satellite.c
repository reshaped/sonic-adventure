/* O SATELLITE, a dish that turns and lifts the players who touch it */
#include <ninja.h>
#include <sg_xpt.h>

#include "collision/collision.h"
#include "debug/editor.h"
#include "draw/draw.h"
#include "minicart/satellite.h"
#include "minicart/shareobj_textures.h"
#include "object/placement.h"
#include "player/player.h"
#include "stage/stage.h"
#include "system/frame.h"
#include "system/task.h"
#include "system/util.h"

extern NJS_OBJECT satellite_object;
extern NJS_ACTION satellite_action;

/* the dish's action advances one motion frame a frame, from 0 up to
 * SATELLITE_FRAMES, then starts over. its collision shapes make a whole turn
 * about z over those motion frames */
#define SATELLITE_FRAMES 119.0F
/* the collision record's radius, in place of the one collision_init finds */
#define SATELLITE_REACH 175.0F

/* a player that touches the satellite has its speed set to this, straight
 * up */
#define SATELLITE_LIFT 2.0F

/* the dish's model is drawn turned a quarter turn about z. its capsule
 * shape is turned as much */
#define QUARTER_TURN NJM_DEG_ANG(90)

/* the dish's action, and its motion frame */
struct satellite_vars {
    NJS_ACTION *action;
    Float frame;
};

#define VARS(work) ((struct satellite_vars *)&(work)->vars)

enum satellite_mode { SATELLITE_INIT, SATELLITE_TURNING, SATELLITE_STILL };

/* the satellite's shapes, a sphere and a capsule. each pushes every group,
 * and resists every group's push */
static struct collision_shape satellite_shapes[2] = {
    {SHAPE_SPHERE,
     0,
     PUSH_ALL | PUSH_FIRM(PUSH_ALL),
     {0.0F, 0.0F, 0.0F},
     45.0F,
     0.0F,
     0.0F,
     {0},
     0,
     NULL},
    {SHAPE_CAPSULE | SHAPE_TURN_YZX,
     0,
     PUSH_ALL | PUSH_FIRM(PUSH_ALL),
     {0.0F, 0.0F, 0.0F},
     15.0F,
     140.0F,
     0.0F,
     {0, 0, QUARTER_TURN},
     0,
     NULL},
};

static void satellite_destroy(struct task *task)
{
    task_kill(task);
}

/* on a frame's first step, draw the dish at its action's motion frame, then
 * advance that motion frame. this function is also the task's draw, which
 * the game runs while paused. the dish keeps turning through a pause */
static void satellite_draw(struct task *task)
{
    struct task_work *work = task->work;
    Angle ang;

    if (frame_step == 0) {
        njSetTexture(&obj_shareobj_texlist);
        njPushMatrix(NULL);
        njTranslateV(NULL, &work->pos);
        TURN_VIA(njRotateY, work->ang.y, ang);
        njRotateZ(NULL, QUARTER_TURN);
        draw_action(VARS(work)->action, VARS(work)->frame);
        njPopMatrix(1);
        VARS(work)->frame += 1.0F;
        if (VARS(work)->frame >= SATELLITE_FRAMES) {
            VARS(work)->frame = 0.0F;
        }
    }
}

/* turn the satellite's collision shapes with the dish, and lift a player
 * that touches it */
void satellite_step(struct task *task)
{
    struct task_work *work = task->work;
    Angle ang;

    if (!placement_try_leave(task)) {
        if (stage_get_stage_act() == STAGE_ACT(STAGE_TWINKLE, 0) ||
            IN_STAGE(STAGE_CART)) {
            /* in the set editor, the dish draws its object at rest, and
             * neither turns nor collides */
            if (edit_mode != 0) {
                njSetTexture(&obj_shareobj_texlist);
                njPushMatrix(NULL);
                njTranslateV(NULL, &work->pos);
                TURN_VIA(njRotateY, work->ang.y, ang);
                njRotateZ(NULL, QUARTER_TURN);
                njEasyDrawObject(&satellite_object);
                njPopMatrix(1);
            } else {
                switch (work->mode) {
                case SATELLITE_INIT:
                    work->mode = SATELLITE_TURNING;
                    VARS(work)->action = &satellite_action;
                    collision_init(task, satellite_shapes, COLLISION_OBJECT, 1,
                                   2);
                    work->collision->radius = SATELLITE_REACH;
                    task->destructor = satellite_destroy;
                    task->draw = satellite_draw;
                    break;
                case SATELLITE_TURNING:
                    work->ang.z = (Angle)(VARS(work)->frame *
                                          ((360.0F / SATELLITE_FRAMES) *
                                           ANGLE_PER_DEGREE));
                    if (work->collision->flags & COLLISION_HIT) {
                        if (work->collision->hit->kind == COLLISION_PLAYER) {
                            /* @hack type: player a Sint8, where the field
                             * and player_set_speed's parameter are Uint8.
                             * SHC keeps the byte on the stack across the
                             * call's loads (MOV.B R0,@R15 at 0x0c916562); as
                             * a Uint8, it goes straight to R4, the function
                             * is 2 bytes shorter and 34 of 146 words
                             * differ */
                            Sint8 player;

                            player =
                                PLAYER_VARS(work->collision->hit->owner->work)
                                    ->player;
                            player_set_speed(player, 0.0F, SATELLITE_LIFT,
                                             0.0F);
                        }
                    }
                    collision_register(work);
                    satellite_draw(task);
                    break;
                case SATELLITE_STILL:
                    break;
                }
            }
        }
    }
}
