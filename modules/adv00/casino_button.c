/* O CASINO BUTTON button that opens the casino when a player touches it */
#include <ninja.h>
#include <sg_xpt.h>

#include "adv00/casino_button.h"
#include "adv00/station_square.h"
#include "adv00/texlists.h"
#include "collision/collision.h"
#include "debug/editor.h"
#include "draw/model.h"
#include "object/placement.h"
#include "player/player.h"
#include "progress/progress.h"
#include "sound/stream.h"
#include "stage/stage.h"
#include "system/task.h"
#include "system/util.h"

extern NJS_OBJECT casino_button_object;

/* the button's distance out from its base, before and after the push */
#define BUTTON_OUT 11.25F
#define BUTTON_IN 8.25F
#define BUTTON_SPEED 0.15F /* how far the button sinks a step */
/* the button's turn a step as it sinks */
#define BUTTON_ROLL ((0.1F * NJD_PI) * ANGLE_PER_RADIAN)
/* the collision shape's turn a step about y */
#define SPIN_SPEED 500.0F

/* while the button waits, its light is lit until work->timer reaches
 * BLINK_DARK. it is dark from there until the count reaches BLINK_PERIOD and
 * starts over */
#define BLINK_DARK 30
#define BLINK_PERIOD 60

enum casino_button_mode { BUTTON_WAITING, BUTTON_SINKING, BUTTON_PUSHED };

struct casino_button_vars {
    Float depth; /* the button's distance out from its base */
    Float spin; /* the collision shape's y angle */
};

#define VARS(work) ((struct casino_button_vars *)&(work)->vars)
#define LIGHT_OFF(work) ((work)->submode)

static void casino_button_step(struct task *task);
static void casino_button_draw(struct task *task);

/* clang-format off */
static struct collision_shape casino_button_shape[] = {
    {SHAPE_SPHERE, 0, PUSH_FIRM(PUSH_ALL) | PUSH_ALL, {0.0F, 0.0F, 14.0F},
     8.0F, 0.0F, 0.0F, {0, 0, 0x4000}, 0, NULL},
};
/* clang-format on */

static Bool casino_button_pushed(void)
{
    switch (player_get_character(0)) {
    case CHARACTER_SONIC:
        return event_flags[FLAG_SONIC_SS_ENTRANCE_CASINO];
    case CHARACTER_TAILS:
        return event_flags[FLAG_TAILS_SS_ENTRANCE_CASINO];
    case CHARACTER_KNUCKLES:
        return event_flags[FLAG_KNUCKLES_SS_ENTRANCE_CASINO];
    case CHARACTER_AMY:
        return event_flags[FLAG_AMY_SS_ENTRANCE_CASINO];
    case CHARACTER_E102:
    case CHARACTER_BIG:
    default:
        return FALSE;
    }
}

void casino_button_init(struct task *task)
{
    struct task_work *work = task->work;

    if ((Uint8)stage_get_stage_act() != STATION_SQUARE_CASINO) {
        njPrintC(NJM_LOCATION(10, 8), "ACT CASINO ONLY !");
        return;
    }
    work->ang.x = 0;
    work->ang.z = 0;
    work->timer = 0;
    LIGHT_OFF(work) = 0;
    VARS(work)->spin = 0.0F;
    if (!placement_editing(task)) {
        if (casino_button_pushed()) {
            VARS(work)->depth = BUTTON_IN;
            work->mode = BUTTON_PUSHED;
        } else {
            VARS(work)->depth = BUTTON_OUT;
            work->mode = BUTTON_WAITING;
        }
        collision_init(task, casino_button_shape, COLLISION_OBJECT, 1, 1);
    }
    task->step = casino_button_step;
    task->draw = casino_button_draw;
}

static void casino_button_step(struct task *task)
{
    struct task_work *work = task->work;
    Float depth;

    if (placement_try_leave(task)) {
        return;
    }
    switch (work->mode) {
    case BUTTON_WAITING:
        TASK_TIMER(work)++;
        if (TASK_TIMER(work) < BLINK_DARK) {
            LIGHT_OFF(work) = 0;
        } else if (TASK_TIMER(work) < BLINK_PERIOD) {
            LIGHT_OFF(work) = 1;
        } else {
            work->timer = 0;
        }
        if ((work->flags & COLLISION_TOUCHING) &&
            work->collision->hit->kind == COLLISION_PLAYER) {
            work->flags &= ~COLLISION_TOUCHING;
            work->mode = BUTTON_SINKING;
            LIGHT_OFF(work) = 0;
            switch (player_get_character(0)) {
            case CHARACTER_SONIC:
                event_flags[FLAG_SONIC_SS_ENTRANCE_CASINO] = 1;
                break;
            case CHARACTER_TAILS:
                event_flags[FLAG_TAILS_SS_ENTRANCE_CASINO] = 1;
                break;
            case CHARACTER_KNUCKLES:
                event_flags[FLAG_KNUCKLES_SS_ENTRANCE_CASINO] = 1;
                break;
            case CHARACTER_AMY:
                event_flags[FLAG_AMY_SS_ENTRANCE_CASINO] = 1;
                break;
            case CHARACTER_E102:
            case CHARACTER_BIG:
            default:
                break;
            }
            stream_request_interlude(MUSIC_ITEM1);
        }
        break;
    case BUTTON_SINKING:
        /* @hack spelling: the subtraction in a statement of its own, which
         * loads depth ahead of the constant and subtracts it (FSUB FR3,FR15
         * at 0x0c90734a). in one expression, SHC adds -0.15F and 5 of 174
         * words differ */
        depth = VARS(work)->depth;
        depth -= BUTTON_SPEED;
        if (!placement_editing(task)) {
            work->collision->shapes->center.z = depth + 2.0F;
        }
        if (depth <= BUTTON_IN) {
            depth = BUTTON_IN;
            work->mode = BUTTON_PUSHED;
        }
        work->ang.z -= BUTTON_ROLL;
        VARS(work)->depth = depth;
        break;
    case BUTTON_PUSHED:
        break;
    }
    if (!placement_editing(task)) {
        work->collision->shapes->ang.y = (Angle)VARS(work)->spin;
        VARS(work)->spin += SPIN_SPEED;
        collision_register(work);
    }
    casino_button_draw(task);
}

/* while the light is on, base and button draw with NJD_FLAG_IGNORE_LIGHT, so
 * they glow */
static void casino_button_draw(struct task *task)
{
    struct task_work *work = task->work;
    NJS_OBJECT *button;
    Angle ang;

    njSetTexture(&ss_casino_texlist);
    draw_attr_save();
    if (LIGHT_OFF(work) == 0) {
        draw_attr_set(0, NJD_FLAG_IGNORE_LIGHT);
        draw_attr_clear(NJD_FLAG_IGNORE_LIGHT, 0);
    } else {
        draw_attr_set(NJD_FLAG_IGNORE_LIGHT, 0);
        draw_attr_clear(0, NJD_FLAG_IGNORE_LIGHT);
    }
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    TURN_VIA(njRotateY, work->ang.y, ang);
    draw_model(casino_button_object.model);
    button = casino_button_object.child;
    njTranslate(NULL, 0.0F, 0.0F, VARS(work)->depth);
    TURN_VIA(njRotateX, work->ang.z, ang);
    draw_model(button->model);
    njPopMatrix(1);
    draw_attr_restore();
}
