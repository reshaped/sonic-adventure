/* SCENE CHANGE, a cylinder that sends player 0 to another Egg Carrier act or
 * to Sky Deck */
#include <ninja.h>
#include <sg_xpt.h>

#include "adv0100/scene_change.h"
#include "collision/collision.h"
#include "debug/editor.h"
#include "draw/draw.h"
#include "draw/tree.h"
#include "object/placement.h"
#include "player/player.h"
#include "progress/progress.h"
#include "stage/stage.h"
#include "system/task.h"

/* the set entry's x angle picks where to go: an Egg Carrier act from 0 to
 * TO_ACT_LAST, or Sky Deck */
#define TO_ACT_LAST 5
#define TO_SKY_DECK 0x100
/* the z angle's bits 4 to 7 pick a test of the character's transform flag,
 * TEST_SET or TEST_CLEAR. any other value tests nothing */
#define CHANGE_TEST(work) ((work)->ang.z & 0xf0)
#define TEST_SET 0x10
#define TEST_CLEAR 0x20
/* the z angle's low four bits pick the entrance */
#define CHANGE_ENTRANCE(work) ((work)->ang.z & 0xf)

enum scene_change_mode { CHANGE_WAIT, CHANGE_DONE };

static Bool scene_change_allowed(struct task_work *work);
static void scene_change_go(struct task_work *work);

/* player 0 is in the cylinder within scl.x of the object across and half of
 * scl.y above or below it. once player 0 is in it and the flag test passes,
 * go where the x angle says. while the set editor holds the task, draw the
 * cylinder instead */
void egg_carrier_scene_change_step(struct task *task)
{
    struct task_work *work = task->work;
    Float radius;
    Float height;
    NJS_POINT3 center;

    if (placement_try_leave(task)) {
        return;
    }
    if (placement_editing(task)) {
        njPushMatrix(NULL);
        radius = work->scl.x * MARKER_FIT;
        height = work->scl.y * MARKER_FIT;
        njTranslate(NULL, work->pos.x, work->pos.y, work->pos.z);
        draw_set_material(0.1F, 1.0F, 0.8F, 0.2F);
        njScale(NULL, radius, height, radius);
        njTranslate(NULL, 0.0F, 10.0F, 0.0F);
        draw_tree(&marker_cylinder_object);
        njPopMatrix(1);
        draw_set_material(0.0F, 0.0F, 0.0F, 0.0F);
    } else {
        switch (work->mode) {
        case CHANGE_WAIT:
            /* @hack redundant: center, set and never read, whose x, y and z
             * the image stores to the stack frame (0x0c923c10 to 0x0c923c28);
             * the test takes pos. without it, the stack frame takes 4 bytes,
             * not 16, and 74 of 126 words differ */
            center.x = work->pos.x;
            center.y = work->pos.y + (work->scl.y * 0.5F);
            center.z = work->pos.z;
            if (player_in_cylinder(&work->pos, work->scl.x,
                                   work->scl.y * 0.5F) == 1) {
                if (scene_change_allowed(work)) {
                    scene_change_go(work);
                    work->mode = CHANGE_DONE;
                }
            }
            break;
        case CHANGE_DONE:
            break;
        }
    }
}

/* TRUE when the character's EC_TRANSFORM flag is as the test asks, or there
 * is no test. for Eggman and Tikal, who have no flag, every test passes */
static Bool scene_change_allowed(struct task_work *work)
{
    switch (CHANGE_TEST(work)) {
    case TEST_SET:
        switch (get_character()) {
        case CHARACTER_SONIC:
            return event_flags[FLAG_SONIC_EC_TRANSFORM] == 1;
        case CHARACTER_TAILS:
            return event_flags[FLAG_TAILS_EC_TRANSFORM] == 1;
        case CHARACTER_KNUCKLES:
            return event_flags[FLAG_KNUCKLES_EC_TRANSFORM] == 1;
        case CHARACTER_AMY:
            return event_flags[FLAG_AMY_EC_TRANSFORM] == 1;
        case CHARACTER_E102:
            return event_flags[FLAG_E102_EC_TRANSFORM] == 1;
        case CHARACTER_BIG:
            return event_flags[FLAG_BIG_EC_TRANSFORM] == 1;
        }
        break;
    case TEST_CLEAR:
        switch (get_character()) {
        case CHARACTER_SONIC:
            return event_flags[FLAG_SONIC_EC_TRANSFORM] == 0;
        case CHARACTER_TAILS:
            return event_flags[FLAG_TAILS_EC_TRANSFORM] == 0;
        case CHARACTER_KNUCKLES:
            return event_flags[FLAG_KNUCKLES_EC_TRANSFORM] == 0;
        case CHARACTER_AMY:
            return event_flags[FLAG_AMY_EC_TRANSFORM] == 0;
        case CHARACTER_E102:
            return event_flags[FLAG_E102_EC_TRANSFORM] == 0;
        case CHARACTER_BIG:
            return event_flags[FLAG_BIG_EC_TRANSFORM] == 0;
        }
        break;
    }
    return TRUE;
}

/* set the entrance, and go to an Egg Carrier act or to Sky Deck. Sky Deck's
 * act is 0 for Sonic and Tails and 2 for Knuckles, and the others go
 * nowhere */
static void scene_change_go(struct task_work *work)
{
    int to = work->ang.x;
    Uint8 stage;
    Uint8 act;

    stage_set_entrance(CHANGE_ENTRANCE(work));
    if (to >= 0 && to <= TO_ACT_LAST) {
        stage = STAGE_EGG_CARRIER;
        /* @hack redundant: to & 0xf, which changes no value from 0 to
         * TO_ACT_LAST, and which the image makes (MOV #15,R13 at 0x0c923e12,
         * AND R14,R13 at 0x0c923e18); as act = to, 4 of 56 words differ */
        act = to & 0xf;
        /* NOLINTNEXTLINE(clang-diagnostic-sometimes-uninitialized) */
    } else if (to == TO_SKY_DECK) {
        switch (get_character()) {
        case CHARACTER_SONIC:
        case CHARACTER_TAILS:
            act = 0;
            break;
        case CHARACTER_KNUCKLES:
            act = 2;
            break;
        default:
            return;
        }
        stage = STAGE_SKY_DECK;
    }
    /* @bug uninitialized: an x angle outside 0 to TO_ACT_LAST, and not
     * TO_SKY_DECK, reaches story_go_to with stage and act unwritten. BF at
     * 0x0c923e44 jumps to 0x0c923e64, which passes the low bytes of R12 and
     * R13 as egg_carrier_scene_change_step left them: for act, the task's
     * address (MOV R4,R13 at 0x0c923b6e), and for stage, the next task's in
     * its level, or 0 for the last (task_step_level's MOV.L @R4,R12 at
     * 0x8c09e260). no set entry of the outside has one */
    story_go_to(stage, act);
}
