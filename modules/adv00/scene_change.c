/* SCENE CHANGE box that sends player 0 to the stage its set entry gives */
#include <ninja.h>
#include <sg_xpt.h>

#include "adv00/scene_change.h"
#include "debug/debug.h"
#include "debug/editor.h"
#include "draw/draw.h"
#include "draw/tree.h"
#include "event/play.h"
#include "object/placement.h"
#include "player/player.h"
#include "progress/progress.h"
#include "stage/stage.h"
#include "system/task.h"
#include "system/util.h"

/* the box's half size on each axis is BOX_HALF * (scale + 1) */
#define BOX_HALF 5.0F
/* the floor of the box player 0 is tested against lies BOX_DROP below the
 * placement's position */
#define BOX_DROP 10.0F

/* the console row where the step names the destination's stage and act */
#define EDITOR_ROW_STAGE 23

/* the set entry's angle z packs the destination. bits 8 to 15 hold its
 * stage, bits 4 to 7 its entrance and bits 0 to 3 its act. TO_STAGE_ACT
 * keeps the stage and act as STAGE_ACT packs them */
#define TO_STAGE(to) (((to) >> 8) & 0xff)
#define TO_ENTRANCE(to) (((to) >> 4) & 0xf)
#define TO_ACT(to) ((to) & 0xf)
#define TO_STAGE_ACT(to) ((to) & 0xff0f)

struct scene_change_vars {
    NJS_VECTOR half; /* the box's half size on each axis */
};

#define VARS(work) ((struct scene_change_vars *)&(work)->vars)

static void station_square_scene_change_step(struct task *task);
static void station_square_scene_change_destroy(struct task *task);
static void station_square_scene_change_go(int stage, int act);

/* TRUE while player 0 is inside the box */
static Bool station_square_scene_change_inside(struct task_work *work)
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
    y = pos.y - work->pos.y - VARS(work)->half.y + BOX_DROP;
    z = pos.z - work->pos.z;
    if (_builtin_fabsf(y) > VARS(work)->half.y) {
        return FALSE;
    }
    ang = -work->ang.y;
    sin = njSin(ang);
    cos = njCos(ang);
    across = (z * cos) - (x * sin);
    along = (z * sin) + (x * cos);
    if (_builtin_fabsf(along) < VARS(work)->half.x &&
        _builtin_fabsf(across) < VARS(work)->half.z) {
        return TRUE;
    }
    return FALSE;
}

/* unless the set editor holds the task, add 1 to each axis of its scale and
 * size the box from it. edit mode's draw adds 1 again, so the box it draws is
 * BOX_HALF larger in each half size than the box tested */
void station_square_scene_change_init(struct task *task)
{
    struct task_work *work = task->work;
    Float x;
    Float y;
    Float z;

    if (!placement_editing(task)) {
        /* @hack spelling: each scale summed inside the product that sizes it,
         * which scales the sum from its register (FMOV FR3,FR6 at
         * 0x0c90cca6); as separate statements, SHC reads each scale back
         * from the work and 32 of 45 words differ */
        /* @hack spelling: the three sizes held in x, y and z, which keeps them
         * in FR6, FR7 and FR4 until three stores at the end (0x0c90ccc2 to
         * 0x0c90ccca); stored straight into half, 30 of 45 words differ */
        x = (work->scl.x += 1.0F) * BOX_HALF;
        y = (work->scl.y += 1.0F) * BOX_HALF;
        z = (work->scl.z += 1.0F) * BOX_HALF;
        VARS(work)->half.x = x;
        VARS(work)->half.y = y;
        VARS(work)->half.z = z;
    }
    task->destructor = station_square_scene_change_destroy;
    task->step = station_square_scene_change_step;
}

/* once player 0 is inside the box, go to the destination and kill the
 * placement. while the set editor holds the task, label the editor's rows and
 * name the destination's stage and act. while the editor is open, draw the
 * box. while an event has control, the step does nothing, drawing included */
static void station_square_scene_change_step(struct task *task)
{
    struct task_work *work = task->work;
    Sint16 stage;
    Sint16 act;
    Sint16 stage_act;
    Uint32 destination;
    Angle ang;

    if (placement_try_leave(task)) {
        return;
    }
    if (event_in_control()) {
        return;
    }
    if (!placement_editing(task)) {
        if (station_square_scene_change_inside(work)) {
            player_get_character(0);
            destination = work->ang.z;
            stage = TO_STAGE(destination);
            act = TO_ACT(destination);
            stage_set_entrance(TO_ENTRANCE(destination));
            station_square_scene_change_go(stage, act);
            placement_kill(task);
            return;
        } else {
            /* @hack redundant: an empty else, which has the editor's mask
             * 0xff0f load into R2 and angle z into R3 (0x0c90cda0, 0x0c90cda2);
             * without it, the two trade and 4 of 163 words differ */
        }
    } else {
        stage_act = TO_STAGE_ACT(work->ang.z);
        njPrintC(EDITOR_LABEL(EDITOR_ROW_ANG_Y), "-> ROTATION Y");
        njPrintC(EDITOR_LABEL(EDITOR_ROW_ANG_Z), "-> STAGE & ENTRANCE & ACT");
        njPrintC(EDITOR_LABEL(EDITOR_ROW_SCL_X), "-> SCALE X");
        njPrintC(EDITOR_LABEL(EDITOR_ROW_SCL_Y), "-> SCALE Y");
        njPrintC(EDITOR_LABEL(EDITOR_ROW_SCL_Z), "-> SCALE Z");
        debug_print_stage_name(EDITOR_LABEL(EDITOR_ROW_STAGE), stage_act);
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
}

static void station_square_scene_change_destroy(struct task *task)
{
}

/* go to the stage, at the act the set entry gives. on Emerald Coast, Twinkle
 * Park, Speed Highway and Casinopolis, some characters take an act of their
 * own in its place. the cart stage takes act 0, and stage 0 goes nowhere */
static void station_square_scene_change_go(int stage, int act)
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
                /* no break ends the last case. with one, story_go_to's
                 * address takes R3, not R2 (0x0c90cfd0), and 2 of 84 words
                 * differ */
                act = 1;
            }
            break;
        }
    } else {
        if (stage == STAGE_CART) {
            act = 0;
        }
    }
    if (stage) {
        story_go_to(stage, act);
    }
}
