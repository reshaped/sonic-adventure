/* O KARTCHANGER invisible box that sends player 0 to the Twinkle Circuit cart
 * race */
#include <ninja.h>
#include <sg_xpt.h>

#include "adv00/kart_changer.h"
#include "debug/editor.h"
#include "draw/draw.h"
#include "draw/tree.h"
#include "object/placement.h"
#include "player/player.h"
#include "progress/progress.h"
#include "stage/stage.h"
#include "system/task.h"
#include "system/util.h"

/* half the box's size on each axis at a scale of 0. each unit of scale adds
 * BOX_HALF more */
#define BOX_HALF 5.0F

struct kart_changer_vars {
    NJS_VECTOR half; /* half the box's size on each axis */
};

#define VARS(work) ((struct kart_changer_vars *)&(work)->vars)

static void kart_changer_step(struct task *task);
static void kart_changer_destroy(struct task *task);
static void kart_changer_go(int stage, int act);

/* TRUE while player 0 is inside the box, which turns with the set entry's y
 * angle. the box's bottom lies BOX_HALF below the entry's position */
static Bool kart_changer_inside(struct task_work *work)
{
    NJS_POINT3 pos;
    Float x;
    Float y;
    Float z;
    Float sin;
    Float cos;
    Float across;
    Float along;
    Angle ang;

    player_get_history(0, 0, &pos, NULL);
    x = pos.x - work->pos.x;
    y = pos.y - work->pos.y - VARS(work)->half.y + BOX_HALF;
    z = pos.z - work->pos.z;
    if (_builtin_fabsf(y) <= VARS(work)->half.y) {
        ang = -work->ang.y;
        sin = njSin(ang);
        cos = njCos(ang);
        across = (z * cos) - (x * sin);
        along = (z * sin) + (x * cos);
        if (_builtin_fabsf(along) <= VARS(work)->half.x &&
            _builtin_fabsf(across) <= VARS(work)->half.z) {
            return TRUE;
        }
    }
    return FALSE;
}

void kart_changer_init(struct task *task)
{
    struct task_work *work = task->work;
    Float x;
    Float y;
    Float z;

    x = (work->scl.x + 1.0F) * BOX_HALF;
    y = (work->scl.y + 1.0F) * BOX_HALF;
    z = (work->scl.z + 1.0F) * BOX_HALF;
    VARS(work)->half.x = x;
    VARS(work)->half.y = y;
    VARS(work)->half.z = z;
    task->step = kart_changer_step;
    task->destructor = kart_changer_destroy;
}

static void kart_changer_step(struct task *task)
{
    struct task_work *work = task->work;
    Angle ang;

    if (placement_try_leave(task)) {
        return;
    }
    if (edit_mode) {
        njPushMatrix(NULL);
        njTranslate(NULL, work->pos.x,
                    work->pos.y + ((work->scl.y + 1.0F) * BOX_HALF),
                    work->pos.z);
        TURN_VIA(njRotateY, work->ang.y, ang);
        njScale(NULL, work->scl.x + 1.0F, work->scl.y + 1.0F,
                work->scl.z + 1.0F);
        draw_set_material(0.1F, 1.0F, 0.8F, 0.2F);
        draw_tree(&marker_box_object);
        njPopMatrix(1);
        draw_set_material(0.0F, 0.0F, 0.0F, 0.0F);
    }
    if (placement_editing(task)) {
        njPrintC(EDITOR_LABEL(EDITOR_ROW_ANG_Y), "-> ROTATION Y");
        njPrintC(EDITOR_LABEL(EDITOR_ROW_SCL_X), "-> SCALE X");
        njPrintC(EDITOR_LABEL(EDITOR_ROW_SCL_Y), "-> SCALE Y");
        njPrintC(EDITOR_LABEL(EDITOR_ROW_SCL_Z), "-> SCALE Z");
    } else if (kart_changer_inside(work)) {
        stage_set_entrance(0);
        kart_changer_go(STAGE_CART, 0);
        placement_kill(task);
    }
}

static void kart_changer_destroy(struct task *task)
{
}

/* go to stage at act, as station_square_scene_change_go does. a character
 * that plays Emerald Coast, Twinkle Park, Speed Highway or Casinopolis goes to
 * its own act there. the cart stage has only act 0, and a stage of 0 goes
 * nowhere */
static void kart_changer_go(int stage, int act)
{
    int character = get_character();

    if (stage <= STAGE_BOSS_LAST) {
        switch (stage) {
        case STAGE_BEACH:
            switch (character) {
            case CHARACTER_SONIC:
            case CHARACTER_E102:
                act = 0;
                break;
            case CHARACTER_BIG:
                act = 2;
                break;
            }
            break;
        case STAGE_TWINKLE:
            switch (character) {
            case CHARACTER_SONIC:
                act = 0;
                break;
            case CHARACTER_AMY:
            case CHARACTER_BIG:
                act = 1;
                break;
            }
            break;
        case STAGE_HIGHWAY:
            switch (character) {
            case CHARACTER_SONIC:
            case CHARACTER_TAILS:
                act = 0;
                break;
            case CHARACTER_KNUCKLES:
                act = 2;
                break;
            }
            break;
        case STAGE_CASINO:
            switch (character) {
            case CHARACTER_SONIC:
            case CHARACTER_KNUCKLES:
                act = 0;
                break;
            case CHARACTER_TAILS:
                act = 1;
                break;
            }
            break;
        }
    } else {
        switch (stage) {
        case STAGE_CART:
            act = 0;
            break;
        /* @hack redundant: default: break, which has SHC load story_go_to's
         * address into R2 (JMP @R2 at 0x0c9127d0); without it, the address
         * takes R3 and 2 of 84 words differ */
        default:
            break;
        }
    }
    if (stage) {
        story_go_to(stage, act);
    }
}
