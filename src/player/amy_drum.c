/* the drums Amy takes up and sets down */
#include <mathf.h>
#include <ninja.h>
#include <sg_xpt.h>

#include "collision/collision.h"
#include "draw/model.h"
#include "player/amy_drum.h"
#include "player/player.h"
#include "player/textures.h"
#include "stage/stage.h"
#include "system/frame.h"
#include "system/task.h"
#include "system/util.h"

/* a drum further than this from player 0 neither draws nor collides */
#define DRUM_RANGE 400.0F

/* a player touching a drum marks it touched only within this height of it,
 * and facing it within this angle */
#define TOUCH_HEIGHT 2.0F
#define TOUCH_FACING NJM_DEG_ANG(45)

/* the stage_act of drum_places's last row, which no act has */
#define DRUM_PLACES_END 0x7fff

/* the act stage_act sets out count drums, at pos[0] to pos[count - 1] */
struct drum_place {
    Uint16 stage_act;
    Uint16 count;
    NJS_POINT3 *pos;
};

extern NJS_MODEL amy_drum_model;

static void drum_step(struct task *task);
static void drum_init(struct task *task);

/* a drum's body, and a cylinder of radius 11.5 round it that pushes
 * nothing, which a player touches before the body */
/* clang-format off */
static struct collision_shape drum_shapes[] = {
    {SHAPE_CYLINDER,
     GUARD(3) | SHAPE_HURTS_PLAYERS | SHAPE_HURTS_ENEMIES | SHAPE_HURTS_OBJECTS,
     PUSH_ALL | PUSH_FIRM(PUSH_ENEMIES | PUSH_OTHERS), {0.0F, 5.0F, 0.0F}, 4.0F,
     5.0F, 0.0F, {0}, 0, NULL},
    {SHAPE_CYLINDER,
     GUARD(3) | SHAPE_HURTS_PLAYERS | SHAPE_HURTS_ENEMIES | SHAPE_HURTS_OBJECTS,
     PUSH_FIRM(PUSH_PLAYERS | PUSH_ENEMIES | PUSH_OTHERS), {0.0F, 5.0F, 0.0F},
     11.5F, 5.0F, 0.0F, {0}, 0, NULL},
};
/* clang-format on */

static NJS_POINT3 drum_twinkle_pos[] = {
    {510.0F, 100.0F, 480.0F},
};
static NJS_POINT3 drum_final_pos[] = {
    {1485.0F, 167.0F, -1570.0F},
    {2278.0F, 5238.0F, -2276.0F},
};
static struct drum_place drum_places[] = {
    {STAGE_ACT(STAGE_TWINKLE, 1), 1, drum_twinkle_pos},
    {STAGE_ACT(STAGE_TWINKLE, 2), 1, drum_twinkle_pos},
    {STAGE_ACT(STAGE_FINAL, 0), 2, drum_final_pos},
    {DRUM_PLACES_END, 0, NULL},
};

static struct task *drum_touched;

/* the step of a drum. within DRUM_RANGE of player 0, it draws the drum and
 * registers its collision, and a player's touch marks it touched as
 * TOUCH_HEIGHT and TOUCH_FACING allow. the drum ends once player 0 has no
 * task */
static void drum_step(struct task *task)
{
    struct task_work *work = task->work;
    struct task_work *player = player_work[0];
    struct collision_work *hit;
    struct task_work *toucher;
    NJS_VECTOR away;
    Angle ang;

    if (player == NULL) {
        task_kill(task);
        return;
    }
    if (njDistanceP2P(&work->pos, &player->pos) > DRUM_RANGE) {
        return;
    }
    if (work->flags & COLLISION_TOUCHING) {
        hit = work->collision->hit;
        if (hit != NULL && hit->kind == COLLISION_PLAYER) {
            toucher = hit->owner->work;
            away = work->pos;
            njSubVector(&away, &toucher->pos);
            ang = (Angle)(atan2f(away.z, away.x) * ANGLE_PER_RADIAN);
            if (_builtin_fabsf(away.y) < TOUCH_HEIGHT &&
                angle_distance(toucher->ang.y, ang) < TOUCH_FACING) {
                drum_touched = task;
            }
        }
    }
    if (frame_step == 0) {
        njSetTexture(&amy_texlist);
        njPushMatrix(NULL);
        njTranslateV(NULL, &work->pos);
        draw_model(&amy_drum_model);
        njPopMatrix(1);
    }
    collision_register(work);
}

static void drum_init(struct task *task)
{
    collision_init(task, drum_shapes, COLLISION_OBJECT, 1, 2);
    task->step = drum_step;
}

Bool amy_drum_create(NJS_POINT3 *pos)
{
    struct task *task;

    /* @hack spelling: the task assigned in the test, which tests R0 and copies
     * it to R4 in the BT/S's slot (MOV R0,R4 at 0x8c0e8e58); assigned in a
     * statement of its own, SHC copies it first and tests R4, and 3 of 28
     * words differ */
    if ((task = task_create(TASK_WORK, 6, drum_init)) != NULL) {
        task->work->pos = *pos;
        return TRUE;
    }
    return FALSE;
}

Bool amy_drum_create_all(void)
{
    struct drum_place *place;
    Uint16 stage_act;
    int count;
    NJS_POINT3 *pos;

    if (get_character() != CHARACTER_AMY) {
        return FALSE;
    }
    stage_act = stage_get_stage_act();
    place = drum_places;
    while (place->stage_act != stage_act) {
        if (place->stage_act == DRUM_PLACES_END) {
            return FALSE;
        }
        place++;
    }
    count = place->count;
    pos = place->pos;
    for (; count > 0; count--, pos++) {
        if (!amy_drum_create(pos)) {
            return FALSE;
        }
    }
    drum_touched = NULL;
    return TRUE;
}

struct task *amy_drum_get_touched(void)
{
    return drum_touched;
}

void amy_drum_clear_touched(void)
{
    drum_touched = NULL;
}
