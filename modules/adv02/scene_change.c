/* O SCENECHG cylinder that sends player 0 into the stage its set entry names */
#include <ninja.h>
#include <sg_xpt.h>

#include "adv02/mystic_ruins.h"
#include "adv02/scene_change.h"
#include "collision/collision.h"
#include "debug/editor.h"
#include "draw/draw.h"
#include "draw/tree.h"
#include "event/play.h"
#include "object/placement.h"
#include "player/player.h"
#include "progress/progress.h"
#include "stage/stage.h"
#include "system/task.h"

/* destinations, by the set entry's x angle */
enum scene_change_to {
    TO_MOUNTAIN,
    TO_ICECAP,
    TO_FINAL_E102, /* E-102's own act of the Final Egg */
    TO_FINAL,
    TO_JUNGLE,
    TO_BASE,
    TO_RUIN,
    TO_RUIN_KNUCKLES,
    TO_SAND_BOARD
};

/* the entrances a cylinder sends player 0 to: JUNGLE_ENTRANCE in
 * MYSTIC_RUINS_JUNGLE, and BASE_ENTRANCE in MYSTIC_RUINS_BASE */
#define JUNGLE_ENTRANCE 4
#define BASE_ENTRANCE 0

static Bool mystic_ruins_scene_change_go(int to);

void mystic_ruins_scene_change_step(struct task *task)
{
    struct task_work *work = task->work;

    if (!placement_try_leave(task)) {
        if (placement_editing(task)) {
            Float radius;
            Float height;

            njPushMatrix(NULL);
            radius = work->scl.x * MARKER_FIT;
            height = work->scl.y * MARKER_FIT;
            njTranslate(NULL, work->pos.x, work->pos.y, work->pos.z);
            draw_set_material(0.1F, 1.0F, 0.8F, 0.2F);
            njScale(NULL, radius, height, radius);
            draw_tree(&marker_cylinder_object);
            njPopMatrix(1);
            draw_set_material(0.0F, 0.0F, 0.0F, 0.0F);
        } else if (!event_in_control()) {
            if (player_in_cylinder(&work->pos, work->scl.x, work->scl.y) == 1) {
                if (mystic_ruins_scene_change_go(work->ang.x)) {
                    placement_kill(task);
                }
            }
        }
    }
}

/* send player 0 to destination to, if the character may go there. it returns
 * TRUE if player 0 goes, and FALSE if not. an x angle outside the enum sends
 * player 0 to act 0 of STAGE_PRACTICE */
static Bool mystic_ruins_scene_change_go(int to)
{
    int stage = STAGE_PRACTICE;
    int act = 0;
    int character = get_character();

    switch (to) {
    case TO_MOUNTAIN:
        switch (character) {
        case CHARACTER_SONIC:
            if (!event_flag_check(FLAG_SONIC_MR_ENTRANCE_MOUNTAIN)) {
                return FALSE;
            }
            act = 0;
            break;
        case CHARACTER_KNUCKLES:
            if (!event_flag_check(FLAG_KNUCKLES_MR_ENTRANCE_MOUNTAIN)) {
                return FALSE;
            }
            act = 2;
            break;
        case CHARACTER_E102:
            if (!event_flag_check(FLAG_E102_MR_ENTRANCE_MOUNTAIN)) {
                return FALSE;
            }
            act = 1;
            break;
        default:
            return FALSE;
        }
        stage = STAGE_MOUNTAIN;
        break;
    case TO_ICECAP:
        switch (character) {
        case CHARACTER_SONIC:
            act = 0;
            break;
        case CHARACTER_TAILS:
            act = 2;
            break;
        case CHARACTER_BIG:
            act = 3;
            break;
        default:
            return FALSE;
        }
        stage = STAGE_ICECAP;
        break;
    case TO_FINAL_E102:
        if (character != CHARACTER_E102) {
            return FALSE;
        }
        stage = STAGE_FINAL;
        act = 2;
        break;
    case TO_FINAL:
        switch (character) {
        case CHARACTER_SONIC:
            if (!event_flag_check(FLAG_SONIC_MR_ENTRANCE_FINAL_EGG)) {
                return FALSE;
            }
            break;
        case CHARACTER_AMY:
            if (!event_flag_check(FLAG_AMY_MR_ENTRANCE_FINAL_EGG)) {
                return FALSE;
            }
            break;
        default:
            return FALSE;
        }
        stage = STAGE_FINAL;
        break;
    case TO_JUNGLE:
        switch (character) {
        case CHARACTER_SONIC:
        case CHARACTER_AMY:
            break;
        case CHARACTER_E102:
            if (!event_flag_check(FLAG_E102_MR_FREE_PASS)) {
                return FALSE;
            }
            break;
        default:
            return FALSE;
        }
        stage = STAGE_MYSTIC_RUINS;
        act = MYSTIC_RUINS_JUNGLE;
        stage_set_entrance(JUNGLE_ENTRANCE);
        break;
    case TO_BASE:
        switch (character) {
        case CHARACTER_SONIC:
            if (!event_flag_check(FLAG_SONIC_MR_APPEAR_FINAL_EGG)) {
                return FALSE;
            }
            break;
        case CHARACTER_AMY:
            if (!event_flag_check(FLAG_AMY_MR_APPEAR_FINAL_EGG)) {
                return FALSE;
            }
            break;
        case CHARACTER_E102:
            if (!event_flag_check(FLAG_E102_MR_APPEAR_FINAL_EGG)) {
                return FALSE;
            }
            break;
        default:
            return FALSE;
        }
        stage = STAGE_MYSTIC_RUINS;
        act = MYSTIC_RUINS_BASE;
        stage_set_entrance(BASE_ENTRANCE);
        break;
    case TO_RUIN:
        if (character == CHARACTER_SONIC &&
            event_flag_check(FLAG_SONIC_MR_ENTRANCE_RUIN)) {
            stage = STAGE_RUIN;
            act = 0;
        } else {
            return FALSE;
        }
        break;
    case TO_RUIN_KNUCKLES:
        if (character == CHARACTER_KNUCKLES &&
            event_flag_check(FLAG_KNUCKLES_MR_ENTRANCE_RUIN)) {
            stage = STAGE_RUIN;
            act = 1;
        } else {
            return FALSE;
        }
        break;
    case TO_SAND_BOARD:
        switch (character) {
        case CHARACTER_SONIC:
            if (!event_flag_check(FLAG_TAILS_CLEAR_SAND_BOARD)) {
                return FALSE;
            }
            break;
        case CHARACTER_TAILS:
            break;
        default:
            return FALSE;
        }
        stage = STAGE_SUNABOARD;
        break;
    }
    story_go_to(stage, act);
    return TRUE;
}
