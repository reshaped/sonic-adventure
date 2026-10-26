/* SCENE CHANGE, a cylinder that sends player 0 to another act of the Egg
 * Carrier inside, or to Hot Shelter */
#include <ninja.h>
#include <sg_xpt.h>

#include "adv0130/scene_change.h"
#include "collision/collision.h"
#include "debug/editor.h"
#include "draw/draw.h"
#include "draw/tree.h"
#include "object/placement.h"
#include "player/player.h"
#include "progress/progress.h"
#include "stage/stage.h"
#include "system/task.h"

/* the set entry's x angle names the destination: TO_ACT_FIRST plus an act of
 * the inside, or TO_SHELTER */
#define TO_ACT_FIRST 0x30
#define TO_ACT_LAST 0x35
#define TO_SHELTER 0x200
/* bits 4 to 7 of the x angle hold a test of the character's EC_TRANSFORM
 * flag. TEST_SET asks for the flag set, TEST_CLEAR for it clear, and any
 * other value for neither */
#define CHANGE_TEST(work) ((work)->ang.x & 0xf0)
#define TEST_SET 0x10
#define TEST_CLEAR 0x20
/* the entrance, in the low four bits of the set entry's z angle */
#define CHANGE_ENTRANCE(work) ((work)->ang.z & 0xf)

enum scene_change_mode { CHANGE_WAIT, CHANGE_DONE };

static Bool scene_change_allowed(struct task_work *work);
static void scene_change_go(struct task_work *work);

/* player 0 is in the cylinder within scl.x of the object across and half of
 * scl.y above or below it. once player 0 is in it and the flag test passes,
 * go where the x angle says. while the set editor holds the task, draw the
 * cylinder instead */
void egg_carrier_inside_scene_change_step(struct task *task)
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
            /* @hack redundant: center, which nothing reads, stored to the
             * frame (FMOV.S FR3,@R4 at 0x0c921b90, then at 0x0c921ba0 and
             * 0x0c921ba8); without it, the function runs 24 bytes shorter and
             * 74 of 126 words differ */
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
    /* the test is read from the x angle, which names the destination, where
     * adv0100/scene_change.c reads it from the z angle. no destination's x
     * angle holds TEST_SET or TEST_CLEAR, so a change that goes anywhere
     * passes. no set entry of the inside asks for a test in either angle */
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

/* set the entrance, then go to the destination. Hot Shelter is act 0 for Amy
 * and Big, and act 2 for E-102. for any other character, the change only sets
 * the entrance */
static void scene_change_go(struct task_work *work)
{
    int to = work->ang.x;
    Uint8 stage;
    Uint8 act;

    stage_set_entrance(CHANGE_ENTRANCE(work));
    if (to >= TO_ACT_FIRST && to <= TO_ACT_LAST) {
        stage = STAGE_EGG_CARRIER_INSIDE;
        act = to & 0xf;
        /* NOLINTNEXTLINE(clang-diagnostic-sometimes-uninitialized) */
    } else if (to == TO_SHELTER) {
        switch (get_character()) {
        case CHARACTER_AMY:
        case CHARACTER_BIG:
            act = 0;
            break;
        case CHARACTER_E102:
            act = 2;
            break;
        default:
            return;
        }
        stage = STAGE_SHELTER;
    }
    /* @bug uninitialized: an x angle that names no destination, as any with
     * a test does, leaves stage and act unset for story_go_to. BF at
     * 0x0c921dc4 jumps to 0x0c921de4, which passes the low bytes of R12 and
     * R13 as egg_carrier_inside_scene_change_step left them: for act, the
     * task's address (MOV R4,R13 at 0x0c921aee), and for stage, the next
     * task's in its level, or 0 for the last (task_step_level's MOV.L @R4,R12
     * at 0x8c09e260). no set entry of the inside has one */
    story_go_to(stage, act);
}
