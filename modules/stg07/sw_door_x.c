/* the rising door: O SW DOOR X. it rises while player 0 stands at its
 * front, and sinks once the player leaves. until it first rises, its front
 * turns to the side player 0 comes from */
#include <ninja.h>
#include <sg_xpt.h>
#include <stdlib.h>

#include "collision/collision.h"
#include "debug/editor.h"
#include "draw/tree.h"
#include "effect/dust.h"
#include "object/placement.h"
#include "player/player.h"
#include "sound/sound.h"
#include "stg07/obj_ruin_textures.h"
#include "stg07/sw_door_x.h"
#include "system/task.h"
#include "system/util.h"

extern NJS_OBJECT sw_door_x_object;

struct sw_door_x_vars {
    Float lift; /* how far the door has risen */
    Float shake; /* its offset along its z axis this step, as it shakes */
    Bool opened; /* TRUE once it has started to rise */
};

#define VARS(work) ((struct sw_door_x_vars *)&(work)->vars)

/* the direction the door's front faces, in the work's scale */
#define FRONT_X(work) ((work)->scl.x)
#define FRONT_Z(work) ((work)->scl.z)

enum sw_door_x_mode {
    DOOR_SHUT,
    DOOR_UNUSED,
    DOOR_RISE,
    DOOR_OPEN,
    DOOR_SINK,
    DOOR_STILL
};

/* player 0 opens the door within DOOR_REACH of it across, and
 * DOOR_REACH_HEIGHT above or below it */
#define DOOR_REACH 26.0F
#define DOOR_REACH_HEIGHT 3.0F

/* the door moves DOOR_SPEED each step, up to DOOR_HEIGHT and back down */
#define DOOR_HEIGHT 44.0F
#define DOOR_SPEED 0.2F

/* as it moves, the door shakes SHAKE_SOFT aside. it shakes SHAKE_HARD
 * while its timer is a multiple of SHAKE_HARD_EVERY. the offset cycles
 * through 0, the shake and its negative */
#define SHAKE_SOFT 0.2F
#define SHAKE_HARD 0.4F
#define SHAKE_HARD_EVERY 17

/* as the door rises, dust starts at its two sides in turn, DUST_SIDE from its
 * middle. the dust moves the way the door's front faces. each puff starts at a
 * random height on the risen door. none starts higher than DUST_TOP above the
 * door's set place */
#define DUST_SIDE 24.0F
#define DUST_TOP 22.0F
#define DUST_SCALE 1.0F

/* its shape's middle, half the door's height above its foot */
#define SHAPE_HALF 22.0F

/* the door's sound as it moves */
#define SE_DOOR 194

/* clang-format off */
static struct collision_shape sw_door_x_shape = {
    SHAPE_RECTANGLE, 0, PUSH_ALL | PUSH_FIRM(PUSH_ALL), {0.0F, 22.0F, 0.0F},
    2.0F, 22.0F, 26.0F, {0}, 0, NULL,
};
/* clang-format on */

static void sw_door_x_run(struct task *task);
static void sw_door_x_draw(struct task *task);

void sw_door_x_step(struct task *task)
{
    struct task_work *work = task->work;

    TASK_TIMER(work) = 0;
    work->mode = DOOR_SHUT;
    VARS(work)->lift = 0.0F;
    VARS(work)->shake = 0.0F;
    VARS(work)->opened = FALSE;
    FRONT_X(work) = -njCos(work->ang.y);
    FRONT_Z(work) = njSin(work->ang.y);
    if (!placement_editing(task)) {
        collision_init(task, &sw_door_x_shape, COLLISION_OBJECT, 1, 1);
    }
    task->step = sw_door_x_run;
    task->draw = sw_door_x_draw;
}

static void sw_door_x_run(struct task *task)
{
    struct task_work *work = task->work;
    /* @hack spelling: speed, the dust's speed as the door rises, reused for
     * player 0's place while the door is shut. SHC keeps it at @R15, in a
     * 32-byte frame (0x0c9053b0, 0x0c90547a). with a vector for each, 6 of
     * sw_door_x_run's 350 words differ */
    NJS_VECTOR speed;
    NJS_POINT3 pos;
    Float sin;
    Float cos;
    Float dx;
    Float dz;
    Float height;
    Float shake;
    Float ahead;
    int side;

    if (placement_try_leave(task)) {
        return;
    }
    if (!placement_editing(task)) {
        switch (work->mode) {
        case DOOR_SHUT:
            if (player_in_cylinder(&work->pos, DOOR_REACH, DOOR_REACH_HEIGHT)) {
                player_get_history(0, 0, &speed, NULL);
                dx = speed.x - work->pos.x;
                dz = speed.z - work->pos.z;
                ahead = (dx * FRONT_X(work)) + (dz * FRONT_Z(work));
                if (ahead > 0.0F) {
                    work->mode = DOOR_RISE;
                    TASK_TIMER(work) = 0;
                    VARS(work)->lift = 0.0F;
                    VARS(work)->opened = TRUE;
                    sound_play_held(SE_DOOR, (int)work, 1, 0);
                } else if (!VARS(work)->opened) {
                    FRONT_X(work) *= -1.0F;
                    FRONT_Z(work) *= -1.0F;
                }
            }
            break;
        case DOOR_RISE:
            VARS(work)->lift += DOOR_SPEED;
            if (VARS(work)->lift >= DOOR_HEIGHT) {
                VARS(work)->lift = DOOR_HEIGHT;
                work->collision->shapes->center.y = DOOR_HEIGHT + SHAPE_HALF;
                TASK_TIMER(work) = 0;
                work->mode = DOOR_OPEN;
                sound_stop_id((int)work);
            }
            work->collision->shapes->center.y = VARS(work)->lift + SHAPE_HALF;
            side = TASK_TIMER(work) % 2;
            sin = njSin(work->ang.y);
            cos = njCos(work->ang.y);
            speed.x = FRONT_X(work);
            speed.y = 0.0F;
            speed.z = FRONT_Z(work);
            if (side) {
                pos.z = (cos * DUST_SIDE) + work->pos.z;
                pos.x = (sin * DUST_SIDE) + work->pos.x;
            } else {
                pos.z = -(cos * DUST_SIDE) + work->pos.z;
                pos.x = -(sin * DUST_SIDE) + work->pos.x;
            }
            height = (rand() * (DOOR_HEIGHT * RAND_SCALE)) + VARS(work)->lift;
            if (height <= DUST_TOP) {
                pos.y = height + work->pos.y;
                dust_create(&pos, &speed, DUST_SCALE);
            }
            if (TASK_TIMER(work) % SHAKE_HARD_EVERY == 0) {
                shake = SHAKE_HARD;
            } else {
                shake = SHAKE_SOFT;
            }
            switch (TASK_TIMER(work) % 3) {
            case 0:
                VARS(work)->shake = 0.0F;
                break;
            case 1:
                VARS(work)->shake = shake;
                break;
            case 2:
                VARS(work)->shake = -shake;
                break;
            }
            TASK_TIMER(work)++;
            break;
        case DOOR_OPEN:
            if (!player_in_cylinder(&work->pos, DOOR_REACH,
                                    DOOR_REACH_HEIGHT)) {
                work->mode = DOOR_SINK;
                sound_play_held(SE_DOOR, (int)work, 1, 0);
            }
            break;
        case DOOR_SINK:
            VARS(work)->lift -= DOOR_SPEED;
            if (VARS(work)->lift <= 0.0F) {
                VARS(work)->lift = 0.0F;
                work->collision->shapes->center.y = SHAPE_HALF;
                TASK_TIMER(work) = 0;
                work->mode = DOOR_SHUT;
                sound_stop_id((int)work);
            }
            work->collision->shapes->center.y = VARS(work)->lift + SHAPE_HALF;
            if (TASK_TIMER(work) % SHAKE_HARD_EVERY == 0) {
                shake = SHAKE_HARD;
            } else {
                shake = SHAKE_SOFT;
            }
            switch (TASK_TIMER(work) % 3) {
            case 0:
                VARS(work)->shake = 0.0F;
                break;
            case 1:
                VARS(work)->shake = shake;
                break;
            case 2:
                VARS(work)->shake = -shake;
                break;
            }
            TASK_TIMER(work)++;
            break;
        case DOOR_STILL:
            break;
        }
        collision_register(work);
    }
    sw_door_x_draw(task);
}

static void sw_door_x_draw(struct task *task)
{
    struct task_work *work = task->work;
    Angle ang;

    njSetTexture(&ruin_door_texlist);
    njPushMatrix(NULL);
    njTranslate(NULL, work->pos.x, work->pos.y + VARS(work)->lift, work->pos.z);
    TURN_VIA(njRotateY, work->ang.y, ang);
    if (VARS(work)->shake != 0.0F) {
        njTranslate(NULL, 0.0F, 0.0F, VARS(work)->shake);
    }
    draw_tree(&sw_door_x_object);
    njPopMatrix(1);
}
