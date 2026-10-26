/* Casinopolis's pinball state, ring payouts, table countdowns and StartShot,
 * the launcher. the first table is act 2, and the second act 3 */
#include <ninja.h>
#include <sg_xpt.h>
#include <stdlib.h>

#include "camera/camera.h"
#include "collision/collision.h"
#include "draw/draw.h"
#include "draw/model.h"
#include "hud/hud.h"
#include "object/common.h"
#include "object/ring.h"
#include "player/player.h"
#include "sound/sound.h"
#include "stage/stage.h"
#include "stg09/casino.h"
#include "stg09/obj_casino_textures.h" /* NOLINT(misc-include-cleaner) */
#include "stg09/pinball.h"
#include "system/frame.h"
#include "system/task.h"
#include "system/texlists.h" /* NOLINT(misc-include-cleaner) */
#include "system/util.h"
#include "vmu/vibration.h"

extern NJS_MODEL countdown_model;

#define HISTORY_ENTRIES 30

/* a ring task's TASK_TIMER(work) while it takes rings. a task that gives
 * them counts its steps to the next ring there, from RINGS_FIRST_DELAY and
 * then RINGS_DELAY */
#define RINGS_TAKEN 0x1234
#define RINGS_FIRST_DELAY 5
#define RINGS_DELAY 6

#define LAUNCH_SPEED_MAX 8.0F

#define STEPS_PER_SECOND 60

#define SE_LAUNCH 248
#define SE_COUNTDOWN_OVER 243

struct pinball_history_entry {
    NJS_VECTOR speed;
    NJS_POINT3 pos;
    struct angles ang;
    struct angles aim; /* the angles player 0 turns toward */
};

Uint8 casino_entrance = CASINO_FROM_START;
Sint8 pinball_target_bonuses = 0;
Uint8 pinball_countdown2_place = 0;
Sint8 pinball_right_lane_touches = 0;
Sint8 pinball_ride_touches = 0;
/* a byte the image holds here, which no code reads */
static Uint8 dat_0caf1b05 = 0;
Uint8 pinball_cards[PINBALL_CARDS] = {0};
Uint8 pinball_cards_dealt = 0;
Uint8 pinball_balls = PINBALL_BALLS;
Uint8 pinball_idea_draws = 0;
Uint8 pinball_ideas = 1;
Uint8 pinball_view = 0;
Uint32 casino_flags = 0;

/* the countdowns' digits */
static NJS_TEXLIST countdown_digit_texlist = {
#include "stg09/pinball/countdown_digit_texlist.inc"
};
static NJS_TEXANIM countdown_digit_texanims[10] = {
#include "stg09/pinball/countdown_digit_texanims.inc"
};
static NJS_SPRITE countdown_digit_sprite = {
#include "stg09/pinball/countdown_digit_sprite.inc"
};

static NJS_TEXANIM card_white_texanim = {
#include "stg09/pinball/card_white_texanim.inc"
};
NJS_SPRITE card_white_sprite = {
#include "stg09/pinball/card_white_sprite.inc"
};

static struct pinball_history_entry pinball_history[HISTORY_ENTRIES] = {0};
static Uint8 pinball_history_index = 0;
/* a word the image holds here, which no code reads */
static Uint32 dat_0caf21dc = 0;

static struct collision_shape start_shot_shape = {
    SHAPE_RECTANGLE,
    ATTACK(3),
    PUSH_FIRM(PUSH_ALL),
    {0.0F, 0.0F, 0.0F},
    8.0F,
    5.0F,
    5.0F,
    {0},
    0,
    NULL,
};

/* the entry restored is the one after pinball_history_index. once the
 * history has wrapped, that is the second oldest */
void pinball_history_restore(void)
{
    NJS_VECTOR *speed = &player_get(0)->speed;
    NJS_POINT3 *pos = &player_work[0]->pos;
    struct angles *ang = &player_work[0]->ang;
    struct angles *aim = &player_motion[0]->aim;
    Uint8 at = (pinball_history_index + 1) % HISTORY_ENTRIES;

    speed->x = pinball_history[at].speed.x;
    speed->y = pinball_history[at].speed.y;
    speed->z = pinball_history[at].speed.z;
    pos->x = pinball_history[at].pos.x;
    pos->y = pinball_history[at].pos.y;
    pos->z = pinball_history[at].pos.z;
    ang->x = pinball_history[at].ang.x;
    ang->y = pinball_history[at].ang.y;
    ang->z = pinball_history[at].ang.z;
    aim->x = pinball_history[at].aim.x;
    aim->y = pinball_history[at].aim.y;
    aim->z = pinball_history[at].aim.z;
    casino_flags &= ~CASINO_HISTORY_RECORDING;
}

void pinball_history_record(void)
{
    NJS_VECTOR *speed = &player_get(0)->speed;
    NJS_POINT3 *pos = &player_work[0]->pos;
    struct angles *ang = &player_work[0]->ang;
    struct angles *aim = &player_motion[0]->aim;

    if (casino_flags & CASINO_HISTORY_RECORDING) {
        pinball_history[pinball_history_index].speed.x = speed->x;
        pinball_history[pinball_history_index].speed.y = speed->y;
        pinball_history[pinball_history_index].speed.z = speed->z;
        pinball_history[pinball_history_index].pos.x = pos->x;
        pinball_history[pinball_history_index].pos.y = pos->y;
        pinball_history[pinball_history_index].pos.z = pos->z;
        pinball_history[pinball_history_index].ang.x = ang->x;
        pinball_history[pinball_history_index].ang.y = ang->y;
        pinball_history[pinball_history_index].ang.z = ang->z;
        pinball_history[pinball_history_index].aim.x = aim->x;
        pinball_history[pinball_history_index].aim.y = aim->y;
        pinball_history[pinball_history_index].aim.z = aim->z;
        if (++pinball_history_index == HISTORY_ENTRIES) {
            pinball_history_index = 0;
        }
    }
}

/* a ring task's step. work->tick counts the rings left to give or take */
static void pinball_rings_step(struct task *task)
{
    struct task_work *work = task->work;

    if (TASK_TIMER(work) == RINGS_TAKEN) {
        if (!(Sint16)ring_count() || !work->tick--) {
            task_kill(task);
        } else {
            ring_add(-1);
        }
    } else if (!TASK_TIMER(work)--) {
        if (!work->tick--) {
            task_kill(task);
        } else {
            work->timer = RINGS_DELAY;
            ring_add(1);
            sound_play(SE_RING, 0, 0, 0);
        }
    }
}

void pinball_give_rings(Uint8 count)
{
    struct task *task = task_create(TASK_WORK, 3, pinball_rings_step);

    task->work->timer = RINGS_FIRST_DELAY;
    task->work->tick = count;
}

void pinball_take_rings(Uint8 count)
{
    struct task_work *work =
        task_create(TASK_WORK, 3, pinball_rings_step)->work;

    work->timer = RINGS_TAKEN;
    work->tick = count;
}

Float pinball_cap_speed(Float speed)
{
    if (speed > LAUNCH_SPEED_MAX) {
        speed = LAUNCH_SPEED_MAX;
    }
    return speed;
}

Float pinball_player_speed(void)
{
    Float speed = player_get(0)->speed.x;

    if (speed < 0.0F) {
        speed = -speed;
    }
    return speed;
}

/* draw the countdown's model at twice its size, turned to the camera's y
 * angle. its seconds left show on it as two digits */
static void pinball_countdown_draw(struct task_work *work)
{
    Angle ang;

    njScale(NULL, 2.0F, 2.0F, 2.0F);
    TURN_VIA(njRotateY, camera_work->ang.y, ang);
    draw_model(&countdown_model);
    njRotateX(NULL, NJM_DEG_ANG(180));
    njTranslate(NULL, -9.0F, -9.0F, 0.0F);
    njDrawSprite3D(&countdown_digit_sprite,
                   TASK_TIMER(work) / STEPS_PER_SECOND / 10, NJD_SPRITE_ALPHA);
    njTranslate(NULL, 9.0F, 0.0F, 0.0F);
    njDrawSprite3D(&countdown_digit_sprite,
                   TASK_TIMER(work) / STEPS_PER_SECOND % 10, NJD_SPRITE_ALPHA);
}

/* draw the first table's countdown, 0 to 5, at its place over the table. it
 * draws only on a frame's first step, while pinball_view is view 0 with
 * PINBALL_VIEW_MOVING */
static void pinball_countdown_draw_act2(struct task_work *work, Uint8 countdown)
{
    if (frame_step != 0) {
        return;
    }
    if (pinball_view != (PINBALL_VIEW_MOVING | PINBALL_VIEW_TABLE)) {
        return;
    }
    casino_set_texture(CASINO_TEXTURES_ACTS_34);
    njPushMatrix(NULL);
    switch (countdown) {
    case 0:
        if (!(casino_flags & CASINO_COUNTDOWN0_HIDE_A)) {
            njPushMatrix(NULL);
            njTranslate(NULL, -19.0F, 30.0F, -94.5F);
            pinball_countdown_draw(work);
            njPopMatrix(1);
        }
        if (!(casino_flags & CASINO_COUNTDOWN0_HIDE_B)) {
            casino_set_texture(CASINO_TEXTURES_ACTS_34);
            njTranslate(NULL, 60.0F, 30.0F, -52.0F);
            pinball_countdown_draw(work);
        }
        break;
    case 1:
        njTranslate(NULL, 92.0F, 30.0F, -54.0F);
        pinball_countdown_draw(work);
        break;
    case 2:
        switch (pinball_countdown2_place) {
        case 0:
            njTranslate(NULL, -1.0F, 30.0F, -103.0F);
            break;
        case 1:
            njTranslate(NULL, 92.0F, 30.0F, -54.0F);
            break;
        case 2:
            njTranslate(NULL, 44.0F, 30.0F, -68.0F);
            break;
        default:
            njTranslate(NULL, -94.0F, 30.0F, -136.0F);
            break;
        }
        pinball_countdown_draw(work);
        break;
    case 3:
        njPushMatrix(NULL);
        njTranslate(NULL, -74.0F, 30.0F, -136.0F);
        pinball_countdown_draw(work);
        njPopMatrix(1);
        casino_set_texture(CASINO_TEXTURES_ACTS_34);
        njTranslate(NULL, 8.0F, 30.0F, 163.0F);
        pinball_countdown_draw(work);
        break;
    case 4:
        njTranslate(NULL, 95.0F, 30.0F, -6.0F);
        pinball_countdown_draw(work);
        break;
    case 5:
        njTranslate(NULL, 27.0F, 30.0F, -103.0F);
        pinball_countdown_draw(work);
        break;
    }
    njPopMatrix(1);
}

/* draw the second table's countdown, 6 to 9, at its place over the table. it
 * draws only on a frame's first step */
static void pinball_countdown_draw_act3(struct task_work *work, Uint8 countdown)
{
    if (frame_step != 0) {
        return;
    }
    casino_set_texture(CASINO_TEXTURES_ACTS_34);
    njPushMatrix(NULL);
    switch (countdown) {
    case 6:
        njTranslate(NULL, -650.5F, -225.0F, -15.0F);
        pinball_countdown_draw(work);
        break;
    case 7:
        njTranslate(NULL, -568.0F, -205.0F, -5.0F);
        pinball_countdown_draw(work);
        break;
    case 8:
        njTranslate(NULL, -605.5F, -215.0F, 8.5F);
        pinball_countdown_draw(work);
        break;
    case 9:
        njTranslate(NULL, -520.0F, -215.0F, -52.0F);
        pinball_countdown_draw(work);
        break;
    }
    njPopMatrix(1);
}

void pinball_act2_countdown0_step(struct task *task)
{
    struct task_work *work;

    if (stage_get_stage_act() != STAGE_ACT(STAGE_CASINO, 2)) {
        task_kill(task);
        casino_flags &= ~CASINO_COUNTDOWN_ACT2_0;
    } else if (!(casino_flags & CASINO_COUNTDOWNS_HELD)) {
        work = task->work;
        if (!--TASK_TIMER(work)) {
            task_kill(task);
            casino_flags &= ~CASINO_COUNTDOWN_ACT2_0;
        } else {
            pinball_countdown_draw_act2(work, 0);
        }
    }
}

void pinball_act2_countdown1_step(struct task *task)
{
    struct task_work *work;
    struct task_work *ring;
    int i;

    if (stage_get_stage_act() != STAGE_ACT(STAGE_CASINO, 2)) {
        task_kill(task);
        casino_flags &= ~CASINO_COUNTDOWN_ACT2_1;
    } else if (!(casino_flags & CASINO_COUNTDOWNS_HELD)) {
        work = task->work;
        if (!--TASK_TIMER(work)) {
            task_kill(task);
            casino_flags &= ~CASINO_COUNTDOWN_ACT2_1;
            /* @hack redundant: the test of task->child as a select of TRUE or
             * FALSE, which has the else's call load task_step_children's
             * address into R2 (MOV.L and JSR @R2 at 0x0c901858). as a plain
             * test, R1 takes it, and 2 of 141 words differ */
        } else if (task->child == NULL ? TRUE : FALSE) {
            /* once the countdown's rings are all gone, put them out again,
             * fire both Houdai and give 20 rings */
            for (i = 0; i < PINBALL_RING_PLACES; i++) {
                ring = task_create_child(TASK_WORK, ring_step, task)->work;
                ring->pos.x = pinball_ring_places[i].x;
                ring->pos.y = pinball_ring_places[i].y;
                ring->pos.z = pinball_ring_places[i].z;
            }
            casino_flags |= CASINO_HOUDAI_L_FIRES;
            casino_flags |= CASINO_HOUDAI_R_FIRES;
            pinball_give_rings(20);
        } else {
            task_step_children(task);
            pinball_countdown_draw_act2(work, 1);
        }
    }
}

void pinball_act2_countdown2_step(struct task *task)
{
    struct task_work *work;

    if (stage_get_stage_act() != STAGE_ACT(STAGE_CASINO, 2)) {
        task_kill(task);
        casino_flags &= ~CASINO_COUNTDOWN_ACT2_2;
    } else if (!(casino_flags & CASINO_COUNTDOWNS_HELD)) {
        work = task->work;
        if (!--TASK_TIMER(work)) {
            task_kill(task);
            casino_flags &= ~CASINO_COUNTDOWN_ACT2_2;
        } else {
            pinball_countdown_draw_act2(work, 2);
        }
    }
}

void pinball_act2_countdown3_step(struct task *task)
{
    struct task_work *work;

    if (stage_get_stage_act() != STAGE_ACT(STAGE_CASINO, 2)) {
        task_kill(task);
        casino_flags &= ~(CASINO_COUNTDOWN_ACT2_3 | CASINO_STOPPER_C_UP);
    } else if (!(casino_flags & CASINO_COUNTDOWNS_HELD)) {
        work = task->work;
        if (!--TASK_TIMER(work)) {
            task_kill(task);
            casino_flags &= ~(CASINO_COUNTDOWN_ACT2_3 | CASINO_STOPPER_C_UP);
        } else {
            pinball_countdown_draw_act2(work, 3);
        }
    }
}

void pinball_act2_countdown4_step(struct task *task)
{
    struct task_work *work;

    if (stage_get_stage_act() != STAGE_ACT(STAGE_CASINO, 2)) {
        task_kill(task);
        casino_flags &= ~CASINO_COUNTDOWN_ACT2_4;
    } else if (!(casino_flags & CASINO_COUNTDOWNS_HELD)) {
        work = task->work;
        if (!--TASK_TIMER(work)) {
            task_kill(task);
            casino_flags &= ~CASINO_COUNTDOWN_ACT2_4;
        } else {
            pinball_countdown_draw_act2(work, 4);
        }
    }
}

void pinball_act2_countdown5_step(struct task *task)
{
    struct task_work *work;

    if (stage_get_stage_act() != STAGE_ACT(STAGE_CASINO, 2)) {
        task_kill(task);
        casino_flags &= ~CASINO_COUNTDOWN_ACT2_5;
    } else if (!(casino_flags & CASINO_COUNTDOWNS_HELD)) {
        work = task->work;
        if (!--TASK_TIMER(work)) {
            task_kill(task);
            casino_flags &= ~CASINO_COUNTDOWN_ACT2_5;
        } else {
            pinball_countdown_draw_act2(work, 5);
        }
    }
}

void pinball_act3_countdown0_step(struct task *task)
{
    struct task_work *work;

    if (stage_get_stage_act() != STAGE_ACT(STAGE_CASINO, 3)) {
        task_kill(task);
        casino_flags &= ~CASINO_COUNTDOWN_ACT3_0;
        casino_flags |= CASINO_BANJU_DOOR_OPEN;
        return;
    } else if (!(casino_flags & CASINO_COUNTDOWNS_HELD)) {
        work = task->work;
        if (!--TASK_TIMER(work)) {
            task_kill(task);
            casino_flags &= ~CASINO_COUNTDOWN_ACT3_0;
            casino_flags |= CASINO_BANJU_DOOR_OPEN;
            sound_play(SE_COUNTDOWN_OVER, 0, 0, 0);
        } else {
            pinball_countdown_draw_act3(work, 6);
        }
    }
}

void pinball_act3_countdown1_step(struct task *task)
{
    struct task_work *work;

    if (stage_get_stage_act() != STAGE_ACT(STAGE_CASINO, 3)) {
        task_kill(task);
        casino_flags &= ~CASINO_COUNTDOWN_ACT3_1;
        casino_flags |= CASINO_BANJU_DOOR_OPEN;
        return;
    } else if (!(casino_flags & CASINO_COUNTDOWNS_HELD)) {
        work = task->work;
        if (!--TASK_TIMER(work)) {
            task_kill(task);
            casino_flags &= ~CASINO_COUNTDOWN_ACT3_1;
            casino_flags |= CASINO_BANJU_DOOR_OPEN;
            sound_play(SE_COUNTDOWN_OVER, 0, 0, 0);
        } else {
            pinball_countdown_draw_act3(work, 7);
        }
    }
}

void pinball_act3_countdown2_step(struct task *task)
{
    struct task_work *work;

    if (stage_get_stage_act() != STAGE_ACT(STAGE_CASINO, 3)) {
        task_kill(task);
        casino_flags &= ~CASINO_COUNTDOWN_ACT3_2;
        casino_flags |= CASINO_BANJU_DOOR_OPEN;
        return;
    } else if (!(casino_flags & CASINO_COUNTDOWNS_HELD)) {
        work = task->work;
        if (!--TASK_TIMER(work)) {
            task_kill(task);
            casino_flags &= ~CASINO_COUNTDOWN_ACT3_2;
            casino_flags |= CASINO_BANJU_DOOR_OPEN;
            sound_play(SE_COUNTDOWN_OVER, 0, 0, 0);
        } else {
            pinball_countdown_draw_act3(work, 8);
        }
    }
}

void pinball_act3_countdown3_step(struct task *task)
{
    struct task_work *work;

    if (stage_get_stage_act() != STAGE_ACT(STAGE_CASINO, 3)) {
        task_kill(task);
        casino_flags &= ~CASINO_COUNTDOWN_ACT3_3;
        casino_flags |= CASINO_BANJU_DOOR_OPEN;
        return;
    } else if (!(casino_flags & CASINO_COUNTDOWNS_HELD)) {
        work = task->work;
        if (!--TASK_TIMER(work)) {
            task_kill(task);
            casino_flags &= ~CASINO_COUNTDOWN_ACT3_3;
            casino_flags |= CASINO_BANJU_DOOR_OPEN;
            sound_play(SE_COUNTDOWN_OVER, 0, 0, 0);
        } else {
            pinball_countdown_draw_act3(work, 9);
        }
    }
}

static void start_shot_destroy(struct task *task)
{
}

static void start_shot_run(struct task *task)
{
    struct task_work *work = task->work;
    NJS_VECTOR speed;
    Angle ang[3];

    if (work->collision->flags & COLLISION_HIT) {
        ang[0] = 0;
        ang[1] = NJM_DEG_ANG(180);
        ang[2] = 0;
        speed.x = work->scl.z + (rand() * work->scl.y * RAND_SCALE);
        speed.y = 0.0F;
        speed.z = 0.0F;
        player_send_flying(0, &speed, ang, 0);
        sound_play(SE_LAUNCH, 0, 0, 0);
        vibration_start(0, 4, 15, 0);
        casino_flags &= ~CASINO_UNK_100;
    }
    collision_register(work);
    object_clear_touch(work, NULL);
}

void start_shot_step(struct task *task)
{
    collision_init(task, &start_shot_shape, COLLISION_OBJECT, 1, 1);
    task->work->tick = 60;
    task->step = start_shot_run;
    task->destructor = start_shot_destroy;
}
