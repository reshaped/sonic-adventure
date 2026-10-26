/* the set editor, which adds an act's objects to its set at a cursor, and
 * takes them out */
#include <ninja.h>
#include <sg_xpt.h>
#include <shinobi.h>
#include <stdio.h>

#include "camera/camera.h"
#include "collision/polygon.h"
#include "collision/world.h"
#include "debug/debug.h"
#include "debug/editor.h"
#include "object/placement.h"
#include "player/player.h"
#include "stage/stage.h"
#include "system/pad.h"
#include "system/task.h"

/* the objects the editor can hold at once: the place mode's one, or the
 * multi mode's row */
#define OBJECT_SLOTS 16

/* how far from the cursor set_take looks for a placement */
#define TAKE_RANGE 200.0F

/* held, X, Y and B pick the axis to edit: x, y or z */
#define AXIS_BUTTONS (PDD_DGT_TX | PDD_DGT_TY | PDD_DGT_TB)

/* the steps left or right is held between changes of kind. the change
 * comes on the step after them */
#define KIND_REPEAT_WAIT 4

/* the bit of a print count that blinks a digit. it flips every 16 calls, so
 * the digit is blank for 16 calls of every 32 */
#define BLINK_BIT 16

/* 1 when value / delta is within a tenth of 1 */
#define NEAR(value, delta)                                                     \
    ((value) / (delta) > 0.9F && (value) / (delta) < 1.1F)

enum editor_mode {
    EDITOR_INIT = 0,
    EDITOR_CLOSED = 1,
    /* editor_request_open asks for it, and the next step opens the editor */
    EDITOR_OPENING = 3,
    EDITOR_OPEN = 4,
    EDITOR_CLOSING = 5
};

/* ACT MODE on the console: what the pad does */
enum act_mode {
    ACT_PLACE, /* one object at the cursor */
    ACT_MULTI, /* a row of objects */
    ACT_SYSTEM, /* the set as a whole */
    ACT_MODES
};

/* SUB MODE on the console: what the axis buttons edit */
enum sub_mode { SUB_POSITION, SUB_ROTATION, SUB_SCALE, SUB_MODES };

static void editor_step(struct task *task);
static void editor_destroy(struct task *task);
static void editor_reset(void);
static void editor_open(struct task_work *work);
static void editor_close(void);
static void editor_run(struct task *task);
static void editor_print(struct task_work *work);
static void editor_place(struct task_work *work);
static void editor_multi(struct task_work *work);
static void editor_system(struct task_work *work);
static void slot_create(struct task_work *work, Sint16 slot);
static void slot_change_kind(struct task_work *work, Sint16 slot);
static void cursor_move(struct task_work *work);
static void edit_pos(Float pos[3], struct task *task);
static void edit_ang(Angle ang[3], struct task *task);
static void edit_scl(Float scl[3], struct task *task);
static void set_add(NJS_POINT3 pos, struct angles ang, NJS_POINT3 scl);
static void set_take(struct task_work *work);
static void edit_vector(Float value[3], NJS_POINT3 *to);
static void edit_angles(Angle value[3], struct angles *to);
static void activate_placements(void);
static void print_angle(const Angle *ang, int x, int y, Uint32 digit);
static void print_float(const Float *value, int x, int y, int digit);
static void multi_line_up(struct task_work *work);

static const char act_names[ACT_MODES][15] = {"PLACE", "MULTI", "SYSTEM"};
static const char sub_names[SUB_MODES][15] = {"POSITION", "ROTATION", "SCALE"};

Sint16 edit_mode = 0;
/* editor_reset sets dat_8c1b1844, dat_8c1b1848 and dat_8c1b184c, and
 * nothing reads them */
static int dat_8c1b1844 = 1;
static int dat_8c1b1848 = 0;
static int dat_8c1b184c = 0;
static Sint16 act_mode = ACT_PLACE;
static Sint16 sub_mode = SUB_POSITION;
/* nothing reads or writes dat_8c1b1854 or dat_8c1b1858 */
static Uint32 dat_8c1b1854 = 0;
static Uint32 dat_8c1b1858 = 1;
/* TRUE while the editor adds to the common set, FALSE while it adds to the
 * player-dependent one */
static Bool common_set = TRUE;
/* each stage's folder of set files, and in set_codes the two characters
 * that name the stage in them. rows 0 to 15 are stages 0 to 15, and rows
 * 16 to 24 name no folder. row 25 holds STAGE_STATION_SQUARE, and from
 * there on row r is stage r + 1. nothing reads either table */
static char set_dirs[33][32] = {
    "Stg00_Practice",
    "Stg01_Emerald",
    "Stg02_Windy",
    "Stg03_Twinkle",
    "Stg04_Highway",
    "Stg05_Mountain",
    "Stg06_SkyDeck",
    "Stg07_Ruin",
    "Stg08_Icecap",
    "Stg09_Casino",
    "Stg10",
    "Stg11",
    "Stg12",
    "Stg13",
    "Stg14",
    "Boss00_Chaos0",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "Adv00_StationSquare",
    "Adv00_StationSquare",
    "Adv00_StationSquare",
    "Adv01_EggCarrier\\AB_Before",
    "Adv01_EggCarrier\\A_After",
    "Adv01_EggCarrier\\B_AFter",
    "Adv01_EggCarrier\\C_Part",
};
static char set_codes[33][5] = {
    "00", "01", "02", "03", "04", "05", "06", "07", "08", "09", "10",
    "11", "12", "13", "14", "15", "16", "17", "18", "19", "20", "21",
    "22", "23", "24", "SS", "SS", "SS", "EC", "EC", "EC", "EC", "  ",
};
Sint16 editor_kind = 0;
/* how many slots are in use, from slot 0 */
static Sint16 slot_count = 1;
/* the multi mode's row: how far apart its objects stand, and how high
 * above the ground */
static Float multi_spacing = 5.0F;
static Float multi_height = 5.0F;
/* editor_reset clears it, and nothing reads it */
static Sint16 dat_8c1b1d34 = 0;
/* what a press adds to a position or scale */
static Float float_delta = 1.0F;
/* what a press adds to an angle: 16 to the power (angle_digit - 1) */
static Uint32 angle_delta = 0x10;
/* the hex digit a press changes, counting from 1 at the right. it blinks
 * while it's edited */
static int angle_digit = 2;
/* the decimal place a press changes, counted out from the point: 1 is the
 * ones and -1 the tenths. it blinks while it's edited */
static int float_digit = 1;
/* the count of print_float calls, which times the decimal digit's blink */
static Sint16 float_blink_count = 0;
/* how far the cursor moves a step */
static Float move_speed = 1.0F;
/* steps with left or right held since the kind last changed */
static Sint16 kind_repeat = 0;
/* the count of print_angle calls, which times the hex digit's blink */
static Sint16 angle_blink_count = 0;

struct task_work *editor_work;
/* the tasks the editor holds, each with a placement and set entry of its
 * own */
static struct task *slot_tasks[OBJECT_SLOTS];
static struct set_entry slot_entries[OBJECT_SLOTS];
static struct placement slot_placements[OBJECT_SLOTS];

static void editor_step(struct task *task)
{
    struct task_work *work;

    work = task->work;
    switch (work->mode) {
    case EDITOR_INIT:
        editor_reset();
        work->mode = EDITOR_CLOSED;
        editor_work = work;
        task->destructor = editor_destroy;
        break;
    case EDITOR_CLOSED:
        break;
    case EDITOR_OPENING:
        work->mode = EDITOR_OPEN;
        editor_open(work);
        pad_disable();
        pause_disable();
        break;
    case EDITOR_OPEN:
        editor_run(task);
        break;
    case EDITOR_CLOSING:
        work->mode = EDITOR_CLOSED;
        editor_close();
        pad_enable();
        pause_enable();
        break;
    default:
        work->mode = EDITOR_INIT;
        break;
    }
}

/* the task's destructor: clear the debug flag if the editor dies while
 * opening, open or closing */
static void editor_destroy(struct task *task)
{
    struct task_work *work;

    work = task->work;
    if (work->mode == EDITOR_OPENING || work->mode == EDITOR_OPEN ||
        work->mode == EDITOR_CLOSING) {
        debug_set_flag(0);
    }
}

static void editor_reset(void)
{
    Sint16 i;

    edit_mode = 0;
    editor_work = NULL;
    dat_8c1b1844 = 1;
    dat_8c1b1848 = 0;
    dat_8c1b184c = 0;
    act_mode = ACT_PLACE;
    sub_mode = SUB_POSITION;
    editor_kind = 0;
    for (i = 0; i < OBJECT_SLOTS; i++) {
        slot_tasks[i] = NULL;
    }
    dat_8c1b1d34 = 0;
    float_delta = 1.0F;
    angle_delta = 0x10;
    angle_digit = 2;
    float_digit = 1;
    float_blink_count = 0;
    move_speed = 1.0F;
}

static void editor_open(struct task_work *work)
{
    edit_mode = 1;
    act_mode = ACT_PLACE;
    camera_take(work, camera_orbit);
    work->pos = player_work[0]->pos;
    work->ang = player_work[0]->ang;
    slot_create(work, 0);
    slot_count = 1;
}

static void editor_close(void)
{
    Sint16 i;

    camera_release();
    for (i = 0; i < OBJECT_SLOTS; i++) {
        slot_tasks[i] = NULL;
    }
    edit_mode = 0;
}

static void editor_run(struct task *task)
{
    struct task_work *work;

    work = task->work;
    editor_print(work);
    switch (act_mode) {
    case ACT_PLACE:
        editor_place(work);
        break;
    case ACT_MULTI:
        editor_multi(work);
        break;
    case ACT_SYSTEM:
        editor_system(work);
        break;
    default:
        act_mode = ACT_PLACE;
        break;
    }
}

/* print the modes, the kind, the cursor and the set on the console from
 * row 10. while an axis button is held, the digit under edit blinks in the
 * value that button edits */
static void editor_print(struct task_work *work)
{
    Sint16 row;
    Sint16 value_row;

    row = value_row = 10;
    njPrintC(NJM_LOCATION(0, row++), "ACT MODE:");
    njPrintC(NJM_LOCATION(9, value_row++), act_names[act_mode]);
    njPrintC(NJM_LOCATION(0, row++), "SUB MODE:");
    njPrintC(NJM_LOCATION(9, value_row++), sub_names[sub_mode]);
    njPrintC(NJM_LOCATION(0, row++), "ITEM NAME:");
    njPrintC(NJM_LOCATION(10, value_row++),
             placement_list->kinds[editor_kind].name);
    njPrintC(NJM_LOCATION(0, row++), "POS X :");
    njPrintC(NJM_LOCATION(0, row++), "    Y :");
    njPrintC(NJM_LOCATION(0, row++), "    Z :");
    if (sub_mode == SUB_POSITION && (per_info[0]->on & PDD_DGT_TX)) {
        print_float(&work->pos.x, 8, value_row++, float_digit);
    } else {
        print_float(&work->pos.x, 8, value_row++, 0);
    }
    if (sub_mode == SUB_POSITION && (per_info[0]->on & PDD_DGT_TY)) {
        print_float(&work->pos.y, 8, value_row++, float_digit);
    } else {
        print_float(&work->pos.y, 8, value_row++, 0);
    }
    if (sub_mode == SUB_POSITION && (per_info[0]->on & PDD_DGT_TB)) {
        print_float(&work->pos.z, 8, value_row++, float_digit);
    } else {
        print_float(&work->pos.z, 8, value_row++, 0);
    }
    njPrintC(NJM_LOCATION(0, row++), "ANG X :");
    njPrintC(NJM_LOCATION(0, row++), "    Y :");
    njPrintC(NJM_LOCATION(0, row++), "    Z :");
    if (sub_mode == SUB_ROTATION && (per_info[0]->on & PDD_DGT_TX)) {
        print_angle(&work->ang.x, 8, value_row++, angle_digit);
    } else {
        print_angle(&work->ang.x, 8, value_row++, 0);
    }
    if (sub_mode == SUB_ROTATION && (per_info[0]->on & PDD_DGT_TY)) {
        print_angle(&work->ang.y, 8, value_row++, angle_digit);
    } else {
        print_angle(&work->ang.y, 8, value_row++, 0);
    }
    if (sub_mode == SUB_ROTATION && (per_info[0]->on & PDD_DGT_TB)) {
        print_angle(&work->ang.z, 8, value_row++, angle_digit);
    } else {
        print_angle(&work->ang.z, 8, value_row++, 0);
    }
    njPrintC(NJM_LOCATION(0, row++), "SCL X :");
    njPrintC(NJM_LOCATION(0, row++), "    Y :");
    njPrintC(NJM_LOCATION(0, row++), "    Z :");
    if (sub_mode == SUB_SCALE && (per_info[0]->on & PDD_DGT_TX)) {
        print_float(&work->scl.x, 8, value_row++, float_digit);
    } else {
        print_float(&work->scl.x, 8, value_row++, 0);
    }
    if (sub_mode == SUB_SCALE && (per_info[0]->on & PDD_DGT_TY)) {
        print_float(&work->scl.y, 8, value_row++, float_digit);
    } else {
        print_float(&work->scl.y, 8, value_row++, 0);
    }
    if (sub_mode == SUB_SCALE && (per_info[0]->on & PDD_DGT_TB)) {
        print_float(&work->scl.z, 8, value_row++, float_digit);
    } else {
        print_float(&work->scl.z, 8, value_row++, 0);
    }
    if (common_set) {
        njPrintC(NJM_LOCATION(0, row), "SET DATA TYPE : COMMON SET");
    } else {
        njPrintC(NJM_LOCATION(0, row), "SET DATA TYPE : PLAYER DEPENDENT SET");
    }
}

/* the place mode: one object at the cursor. with Start held, A takes a
 * placement, B adds one, and X or Y puts the cursor on a floor. with A
 * held, the directions change the kind, X the sub mode and B the act mode,
 * and R and L the cursor's speed. otherwise the pad moves the cursor, or
 * edits the object while an axis button is held */
static void editor_place(struct task_work *work)
{
    struct height_query query;
    struct world_object *object;
    Sint16 i;
    Float top;
    Float best;

    if (slot_tasks[0] == NULL) {
        slot_create(work, 0);
    }
    if (per_info[0]->on & PDD_DGT_ST) {
        if (per_info[0]->press & PDD_DGT_TA) {
            set_take(work);
        } else if (per_info[0]->press & PDD_DGT_TB) {
            set_add(work->pos, work->ang, work->scl);
        } else if (per_info[0]->press & PDD_DGT_TX) {
            /* onto the first floor found below */
            query.pos = work->pos;
            world_list_near(work->pos.x, work->pos.y, work->pos.z, 20.0F);
            i = 0;
            object = world_objects;
            while (i < world_count) {
                /* @bug uninitialized: the cursor can take a floor's level
                 * that nothing set. a surface above the cursor alone passes
                 * the test (TST R0,R0 at 0x8c044326), and the level then read
                 * (0x8c04432c) is one this search never wrote */
                if (polygon_find_model_heights(&query, object->object)) {
                    work->pos.y = query.below.level;
                    break;
                }
                i++;
                /* @hack spelling: object = object + 1, which keeps
                 * slot_tasks' address in R12 and the 0 in R13 (0x8c04422c,
                 * 0x8c04424e); as ++, the two swap registers and 27 of 402
                 * words differ */
                object = object + 1;
            }
        } else if (per_info[0]->press & PDD_DGT_TY) {
            /* onto the highest floor between 10000 below the cursor and 30
             * above it, turned to its slope */
            top = work->pos.y + 30.0F;
            best = work->pos.y - 10000.0F;
            query.pos = work->pos;
            world_list_near(work->pos.x, work->pos.y, work->pos.z, 20.0F);
            i = 0;
            object = world_objects;
            while (i < world_count) {
                if (polygon_find_model_heights(&query, object->object) &&
                    query.below.found && query.below.level > best &&
                    query.below.level < top) {
                    best = query.below.level;
                    work->pos.y = best;
                    work->ang.x = query.below.ang_x;
                    work->ang.z = query.below.ang_z;
                }
                i++;
                /* @hack spelling: object = object + 1, which after the loop
                 * loads editor_kind's address into R7 (0x8c04443a); as ++, it
                 * loads into R5 and 22 of 402 words differ */
                object = object + 1;
            }
        }
    } else if (per_info[0]->on & PDD_DGT_TA) {
        if ((per_info[0]->press & (PDD_DGT_KU | PDD_DGT_KD)) ||
            (per_info[0]->on & (PDD_DGT_KL | PDD_DGT_KR))) {
            if (per_info[0]->press & PDD_DGT_KU) {
                editor_kind--;
                slot_change_kind(work, 0);
            } else if (per_info[0]->press & PDD_DGT_KD) {
                editor_kind++;
                slot_change_kind(work, 0);
            } else if (per_info[0]->on & PDD_DGT_KR) {
                kind_repeat++;
                if (kind_repeat > KIND_REPEAT_WAIT) {
                    kind_repeat = 0;
                    editor_kind++;
                    slot_change_kind(work, 0);
                }
            } else if (per_info[0]->on & PDD_DGT_KL) {
                kind_repeat++;
                if (kind_repeat > KIND_REPEAT_WAIT) {
                    kind_repeat = 0;
                    editor_kind--;
                    slot_change_kind(work, 0);
                }
            }
        } else if (per_info[0]->press & PDD_DGT_TX) {
            sub_mode++;
            if (sub_mode > SUB_SCALE) {
                sub_mode = SUB_POSITION;
            }
        } else if (per_info[0]->press & PDD_DGT_TB) {
            act_mode++;
            if (act_mode >= ACT_MODES) {
                act_mode = ACT_PLACE;
            }
            task_kill(slot_tasks[0]);
            slot_count = 0;
            slot_tasks[0] = NULL;
        } else if (per_info[0]->press & PDD_DGT_TY) {
            /* a press of Y does nothing, and keeps R and L from acting on
             * that step */
        } else if (per_info[0]->press & PDD_DGT_TR) {
            move_speed += 2.0F;
            if (move_speed > 20.0F) {
                move_speed = 20.0F;
            }
        } else if (per_info[0]->press & PDD_DGT_TL) {
            move_speed -= 2.0F;
            if (move_speed < 1.0F) {
                move_speed = 1.0F;
            }
        }
    } else if (!(per_info[0]->on & AXIS_BUTTONS)) {
        cursor_move(work);
    } else {
        switch (sub_mode) {
        case SUB_POSITION:
            edit_pos(&work->pos.x, slot_tasks[0]);
            break;
        case SUB_ROTATION:
            edit_ang(&work->ang.x, slot_tasks[0]);
            break;
        case SUB_SCALE:
            edit_scl(&work->scl.x, slot_tasks[0]);
            break;
        }
    }
    debug_draw_axes(work);
}

/* the multi mode: a row of objects, from the cursor away from the camera.
 * with Start held, A adds an object to the row, B takes one away, and X
 * and Y raise and lower the row. with A held, right and left space the row
 * out and in, and Y adds the row to the set. the rest is as in the place
 * mode */
static void editor_multi(struct task_work *work)
{
    struct task **slot;
    struct task_work *slot_work;
    Sint16 i;

    i = 0;
    /* @hack spelling: a test, then a do-while, which puts task_kill's address
     * ahead of 0.5 in the pool (0x8c044730, 0x8c044734); as a for loop, the
     * two swap places and 2 of 377 words differ */
    if (slot_count > 0) {
        do {
            if (slot_tasks[i] == NULL) {
                slot_create(work, i);
            }
            i++;
        } while (i < slot_count);
    }
    if (per_info[0]->on & PDD_DGT_ST) {
        if (per_info[0]->press & PDD_DGT_TA) {
            if (slot_count < OBJECT_SLOTS) {
                slot_create(work, slot_count);
                slot_count++;
            }
        } else if (per_info[0]->press & PDD_DGT_TB) {
            if (slot_count > 0) {
                slot_count--;
                task_kill(slot_tasks[slot_count]);
                slot_tasks[slot_count] = NULL;
            }
        } else if (per_info[0]->press & PDD_DGT_TX) {
            multi_height += 0.5F;
            /* nothing limits the row's height, as the test below is empty */
            /* @hack redundant: an empty test of the height against 50, which
             * keeps the load of 50.0 into FR5 (0x8c0446b0); without it, the
             * function runs 6 bytes short and 141 of 377 words differ */
            if (multi_height > 50.0F) {
            }
        } else if (per_info[0]->press & PDD_DGT_TY) {
            multi_height -= 0.5F;
            if (multi_height < 0.0F) {
                multi_height = 0.0F;
            }
        }
    } else if (per_info[0]->on & PDD_DGT_TA) {
        if ((per_info[0]->press & (PDD_DGT_KU | PDD_DGT_KD)) ||
            (per_info[0]->on & (PDD_DGT_KL | PDD_DGT_KR))) {
            if (per_info[0]->press & PDD_DGT_KU) {
                editor_kind--;
                for (i = 0; i < slot_count; i++) {
                    slot_change_kind(work, i);
                }
            } else if (per_info[0]->press & PDD_DGT_KD) {
                editor_kind++;
                for (i = 0; i < slot_count; i++) {
                    slot_change_kind(work, i);
                }
            } else if (per_info[0]->on & PDD_DGT_KR) {
                multi_spacing += 0.5F;
                if (multi_spacing > 50.0F) {
                    multi_spacing = 50.0F;
                }
            } else if (per_info[0]->on & PDD_DGT_KL) {
                multi_spacing -= 0.5F;
                if (multi_spacing < 0.0F) {
                    multi_spacing = 0.0F;
                }
            }
        } else if (per_info[0]->press & PDD_DGT_TX) {
            sub_mode++;
            if (sub_mode > SUB_SCALE) {
                sub_mode = SUB_POSITION;
            }
        } else if (per_info[0]->press & PDD_DGT_TB) {
            act_mode++;
            if (act_mode >= ACT_MODES) {
                act_mode = ACT_PLACE;
            }
            for (i = 0, slot = slot_tasks; i < slot_count; i++, slot++) {
                task_kill(*slot);
                *slot = NULL;
            }
            slot_count = 0;
        } else if (per_info[0]->press & PDD_DGT_TY) {
            i = 0;
            slot = slot_tasks;
            while (i < slot_count) {
                slot_work = (*slot++)->work;
                set_add(slot_work->pos, slot_work->ang, slot_work->scl);
                /* @hack spelling: i = i + 1, which puts MOV R10,R9 in the
                 * delay slot of the BT/S at 0x8c04492c; as ++, the function
                 * runs 2 bytes long and 77 of 377 words differ */
                i = i + 1;
            }
        } else if (per_info[0]->press & PDD_DGT_TR) {
            move_speed += 2.0F;
            if (move_speed > 20.0F) {
                move_speed = 20.0F;
            }
        } else if (per_info[0]->press & PDD_DGT_TL) {
            move_speed -= 2.0F;
            if (move_speed < 1.0F) {
                move_speed = 1.0F;
            }
        }
    } else if (!(per_info[0]->on & AXIS_BUTTONS)) {
        cursor_move(work);
    } else if (slot_tasks[0] != NULL) {
        switch (sub_mode) {
        case SUB_POSITION:
            edit_pos(&work->pos.x, slot_tasks[0]);
            break;
        case SUB_ROTATION:
            edit_ang(&work->ang.x, slot_tasks[0]);
            break;
        case SUB_SCALE:
            edit_scl(&work->scl.x, slot_tasks[0]);
            break;
        }
    }
    debug_draw_axes(work);
    multi_line_up(work);
}

/* the system mode. with A held, B goes back to the place mode and X
 * closes the editor. otherwise B makes every placement active, and X
 * switches between the common set and the player-dependent one */
static void editor_system(struct task_work *work)
{
    if (per_info[0]->on & PDD_DGT_TA) {
        if (per_info[0]->press & PDD_DGT_TB) {
            act_mode++;
            if (act_mode >= ACT_MODES) {
                act_mode = ACT_PLACE;
            }
            slot_count = 1;
            slot_create(work, 0);
        } else if (per_info[0]->press & PDD_DGT_TX) {
            debug_set_flag(0);
            work->mode = EDITOR_CLOSING;
            return;
        }
    } else if (per_info[0]->press & PDD_DGT_TB) {
        activate_placements();
    } else if (per_info[0]->press & PDD_DGT_TX) {
        /* @hack spelling: common_set compared with TRUE, which SHC compares
         * with 1 and branches on (CMP/EQ #1,R0 and BF at 0x8c044b36,
         * 0x8c044b38). common_set only ever holds TRUE or FALSE, so the
         * select is !common_set; as !common_set, it tests with TST and MOVT,
         * the function runs 6 bytes short and 13 of 67 words differ */
        common_set = (common_set == TRUE) ? FALSE : TRUE;
    }
}

/* make a task of editor_kind in the slot, posed as the cursor is, with a
 * placement of its own that keeps it in range. the slot is null when
 * task_create returns null. many of the editor's uses of a slot leave it
 * unchecked */
static void slot_create(struct task_work *work, Sint16 slot)
{
    struct object_kind *kind;
    struct task_work *to;
    /* @hack spelling: the kind's create through a local, which the image
     * keeps in the frame at 4, an 8-byte frame (ADD #-8,R15 at 0x8c044b56);
     * read in the call, the frame is 4 bytes, the function runs 6 bytes short
     * and 128 of 146 words differ */
    Uint8 create;

    kind = &placement_list->kinds[editor_kind];
    create = kind->create;
    slot_tasks[slot] = task_create(create, kind->level, kind->step);
    if (slot_tasks[slot] != NULL) {
        slot_placements[slot].leave.range = kind->range + LEAVE_MARGIN;
        slot_placements[slot].flags = PLACEMENT_LIVE;
        slot_placements[slot].flags |= PLACEMENT_ACTIVE;
        slot_placements[slot].flags |= PLACEMENT_IN_RANGE;
        slot_placements[slot].entry = &slot_entries[slot];
        if (common_set) {
            slot_placements[slot].entry->id = editor_kind;
        } else {
            slot_placements[slot].entry->id = editor_kind | SET_PLAYER;
        }
        if (kind->attr & KIND_RANGE) {
            slot_placements[slot].leave.range = kind->range + LEAVE_MARGIN;
        } else {
            slot_placements[slot].leave.range = PLACE_RANGE + LEAVE_MARGIN;
        }
        slot_tasks[slot]->placement = &slot_placements[slot];
        slot_tasks[slot]->destructor = task_idle;
        to = slot_tasks[slot]->work;
        if (to != NULL) {
            to->pos = work->pos;
            to->ang = work->ang;
            to->scl = work->scl;
            to->vars.p[0] = kind->param;
        }
    }
}

/* wrap editor_kind around the act's object list, and kill the slot's task.
 * the mode's next step makes it again, of the new kind */
static void slot_change_kind(struct task_work *work, Sint16 slot)
{
    if (editor_kind < 0) {
        editor_kind = editor_kind + placement_list->count;
    } else if (editor_kind >= placement_list->count) {
        editor_kind = editor_kind - placement_list->count;
    }
    task_kill(slot_tasks[slot]);
    slot_tasks[slot] = NULL;
}

/* the directions move the cursor relative to the camera, and L and R raise
 * and lower it. slot 0's task takes the cursor's position and x and z
 * angles */
static void cursor_move(struct task_work *work)
{
    Angle ang;

    ang = camera_work->ang.y;
    if (per_info[0]->on & PAD_DIRECTIONS) {
        if (per_info[0]->on & PDD_DGT_KU) {
            if (per_info[0]->on & PDD_DGT_KL) {
                ang += NJM_DEG_ANG(225);
            } else if (per_info[0]->on & PDD_DGT_KR) {
                ang += NJM_DEG_ANG(135);
            } else {
                ang += NJM_DEG_ANG(180);
            }
        } else if (per_info[0]->on & PDD_DGT_KD) {
            if (per_info[0]->on & PDD_DGT_KL) {
                ang += NJM_DEG_ANG(315);
            } else if (per_info[0]->on & PDD_DGT_KR) {
                ang += NJM_DEG_ANG(45);
            }
        } else if (per_info[0]->on & PDD_DGT_KL) {
            ang += NJM_DEG_ANG(270);
        } else if (per_info[0]->on & PDD_DGT_KR) {
            ang += NJM_DEG_ANG(90);
        }
        work->pos.z += njCos(ang) * move_speed;
        work->pos.x += njSin(ang) * move_speed;
    }
    if (per_info[0]->on & (PDD_DGT_TL | PDD_DGT_TR)) {
        if (per_info[0]->on & PDD_DGT_TL) {
            work->pos.y += move_speed;
        } else if (per_info[0]->on & PDD_DGT_TR) {
            work->pos.y -= move_speed;
        }
    }
    if (slot_tasks[0] != NULL && slot_tasks[0]->work != NULL) {
        slot_tasks[0]->work->pos = work->pos;
        slot_tasks[0]->work->ang.x = work->ang.x;
        slot_tasks[0]->work->ang.z = work->ang.z;
    }
}

/* while the task has work, edit the cursor's position, angles or scale, and
 * copy them to the task's. task must not be null */
static void edit_pos(Float pos[3], struct task *task)
{
    if (task->work != NULL) {
        edit_vector(pos, &task->work->pos);
    }
}

static void edit_ang(Angle ang[3], struct task *task)
{
    if (task->work != NULL) {
        edit_angles(ang, &task->work->ang);
    }
}

static void edit_scl(Float scl[3], struct task *task)
{
    if (task->work != NULL) {
        edit_vector(scl, &task->work->scl);
    }
}

/* append an entry of editor_kind at pos, ang and scl to the set file, in
 * the set common_set picks, and a placement for it */
static void set_add(NJS_POINT3 pos, struct angles ang, NJS_POINT3 scl)
{
    struct placement *placement;
    struct set_entry *entry;

    /* @bug memory: placement_count isn't tested against
     * PLACEMENT_COUNT, so at it the stores from 0x8c044f36 land past
     * placements */
    placement = &placements[placement_count];
    /* @hack redundant: placement tested for null, which an address in
     * placements never is, for the TST and BT at 0x8c044f2c; without it,
     * count unmeasured */
    if (placement != NULL) {
        placement->flags = PLACEMENT_ACTIVE;
        placement->tasks = 0;
        placement->state = 0;
        entry = &placement_entries[*placement_file_count];
        placement->entry = entry;
        if (common_set) {
            entry->id = editor_kind;
        } else {
            entry->id = editor_kind | SET_PLAYER;
        }
        entry->ang_x = ang.x;
        entry->ang_y = ang.y;
        entry->ang_z = ang.z;
        entry->pos.x = pos.x;
        entry->pos.y = pos.y;
        entry->pos.z = pos.z;
        entry->scl.x = scl.x;
        entry->scl.y = scl.y;
        entry->scl.z = scl.z;
        placement_count++;
        (*placement_file_count)++;
    }
}

/* take the nearest placement within TAKE_RANGE out of the set, into slot
 * 0, and put the cursor where it stood. a placement of a KIND_FIXED kind
 * stays. the last placement and entry fill the gaps it leaves */
static void set_take(struct task_work *work)
{
    struct placement *nearest;
    struct set_entry *entry;
    Float best;
    Float dx;
    Float dy;
    Float dz;
    Float distance;
    int i;

    best = TAKE_RANGE;
    for (i = 0; i < placement_count; i++) {
        struct placement *placement;
        int id;
        int index;

        /* @hack spelling: index set in the subscript, which has SHC work out
         * each placement's address from i (MOV #0,R11 at 0x8c044fec); indexed
         * by i, it advances a pointer instead, and 154 of 207 words differ */
        placement = &placements[index = i];
        entry = placements[index].entry;
        /* @bug memory: the whole id, SET_PLAYER and all (EXTU.W at
         * 0x8c04501e), so a player-dependent entry reads the attr of the kind
         * 0x8000 past its own (0x8c04502a) */
        id = entry->id;
        if (!(placement_list->kinds[id].attr & KIND_FIXED)) {
            dx = entry->pos.x - work->pos.x;
            dy = entry->pos.y - work->pos.y;
            dz = entry->pos.z - work->pos.z;
            distance = njSqrt((dx * dx) + (dy * dy) + (dz * dz));
            if (distance < best) {
                best = distance;
                nearest = placement;
            }
        }
    }
    if (best < TAKE_RANGE) {
        task_kill(slot_tasks[0]);
        editor_kind = slot_entries[0].id = nearest->entry->id & SET_KIND;
        slot_placements[0] = *nearest;
        slot_placements[0].entry = &slot_entries[0];
        if (nearest->flags & PLACEMENT_LIVE) {
            slot_tasks[0] = nearest->task;
        } else {
            slot_create(work, 0);
        }
        slot_tasks[0]->placement = &slot_placements[0];
        entry = nearest->entry;
        work->pos.x = entry->pos.x;
        work->pos.y = entry->pos.y;
        work->pos.z = entry->pos.z;
        work->ang.x = entry->ang_x;
        work->ang.y = entry->ang_y;
        work->ang.z = entry->ang_z;
        work->scl.x = entry->scl.x;
        work->scl.y = entry->scl.y;
        work->scl.z = entry->scl.z;
        placement_count--;
        *nearest = placements[placement_count];
        (*placement_file_count)--;
        *entry = placement_entries[*placement_file_count];
        nearest->entry = entry;
        if (nearest->task != NULL) {
            nearest->task->placement = nearest;
        }
    }
}

/* edit value while an axis button is held. right makes float_delta ten
 * times smaller, down to 0.01, and left ten times larger, up to 1000.
 * float_digit follows it. up and down add float_delta to the held button's
 * axis of value or take it away, and repeat while held. then copy value to
 * *to */
/* @hack type: the cursor's values taken as an array from their first member,
 * which makes SHC form &value[1] and &value[2] in R13 and R12 ahead of the
 * button tests (0x8c045208 to 0x8c04520c); as an NJS_POINT3, the function
 * runs 8 bytes short and 96 of 218 words differ */
static void edit_vector(Float value[3], NJS_POINT3 *to)
{
    Float *axis;

    if (per_info[0]->on & AXIS_BUTTONS) {
        if (per_info[0]->press & PDD_DGT_KR) {
            if (float_delta >= 0.1F) {
                float_digit--;
                if (float_digit == 0) {
                    float_digit = -1;
                }
                if (NEAR(0.1F, float_delta)) {
                    float_delta = 0.01F;
                } else if (NEAR(1.0F, float_delta)) {
                    float_delta = 0.1F;
                } else if (NEAR(10.0F, float_delta)) {
                    float_delta = 1.0F;
                } else if (NEAR(100.0F, float_delta)) {
                    float_delta = 10.0F;
                } else if (NEAR(1000.0F, float_delta)) {
                    float_delta = 100.0F;
                }
            }
        }
        if (per_info[0]->press & PDD_DGT_KL) {
            if (float_delta <= 101.0F) {
                float_digit++;
                if (float_digit == 0) {
                    float_digit = 1;
                }
                if (NEAR(0.01F, float_delta)) {
                    float_delta = 0.1F;
                } else if (NEAR(0.1F, float_delta)) {
                    float_delta = 1.0F;
                } else if (NEAR(1.0F, float_delta)) {
                    float_delta = 10.0F;
                } else if (NEAR(10.0F, float_delta)) {
                    float_delta = 100.0F;
                } else if (100.0F / float_delta > 0.0F &&
                           100.0F / float_delta < 1.1F) {
                    /* NEAR(100.0F, float_delta), but with a lower bound of
                     * 0, not 0.9 */
                    float_delta = 1000.0F;
                }
            }
        }
        /* one of AXIS_BUTTONS is held, as the test above found, so one of
         * these tests sets axis */
        if (per_info[0]->on & PDD_DGT_TX) {
            axis = &value[0];
        } else if (per_info[0]->on & PDD_DGT_TY) {
            axis = &value[1];
        } else if (per_info[0]->on & PDD_DGT_TB) {
            axis = &value[2];
        }
        if (pad_press_repeat[0] & PDD_DGT_KU) {
            *axis += float_delta;
        } else if (pad_press_repeat[0] & PDD_DGT_KD) {
            *axis -= float_delta;
        }
        to->x = value[0];
        to->y = value[1];
        to->z = value[2];
    }
}

/* edit_vector for angles: right and left move angle_delta a hex digit,
 * between 1 and 0x10000000 */
/* @hack type: the cursor's angles taken as an array from their first
 * member, as in edit_vector; as struct angles, the function runs 8 bytes
 * short and 81 of 90 words differ */
static void edit_angles(Angle value[3], struct angles *to)
{
    Angle *axis;

    if (per_info[0]->on & AXIS_BUTTONS) {
        if (per_info[0]->press & PDD_DGT_KR) {
            if (angle_delta > 1) {
                angle_delta >>= 4;
                angle_digit--;
            }
        }
        if (per_info[0]->press & PDD_DGT_KL) {
            if (angle_delta < 0x10000000) {
                angle_delta <<= 4;
                angle_digit++;
            }
        }
        /* as in edit_vector, one of these tests sets axis */
        if (per_info[0]->on & PDD_DGT_TX) {
            axis = &value[0];
        } else if (per_info[0]->on & PDD_DGT_TY) {
            axis = &value[1];
        } else if (per_info[0]->on & PDD_DGT_TB) {
            axis = &value[2];
        }
        if (pad_press_repeat[0] & PDD_DGT_KU) {
            *axis += angle_delta;
        } else if (pad_press_repeat[0] & PDD_DGT_KD) {
            *axis -= angle_delta;
        }
        to->x = value[0];
        to->y = value[1];
        to->z = value[2];
    }
}

Bool placement_editing(struct task *task)
{
    Sint16 i;

    for (i = 0; i < slot_count; i++) {
        if (task == slot_tasks[i]) {
            return TRUE;
        }
    }
    return FALSE;
}

/* set PLACEMENT_ACTIVE on every placement, so each is placed again
 * whenever its task isn't live */
static void activate_placements(void)
{
    struct placement *placement;

    for (placement = &placements[placement_count - 1];
         placement >= &placements[0]; placement--) {
        placement->flags |= PLACEMENT_ACTIVE;
    }
}

/* print the angle at x, y as eight hex digits. digit counts from 1 at the
 * right, and that digit blinks by BLINK_BIT of angle_blink_count. a digit
 * of 0 blinks none */
static void print_angle(const Angle *ang, int x, int y, Uint32 digit)
{
    Uint32 value;
    Uint32 i;
    Uint32 nibble;

    value = *ang;
    angle_blink_count++;
    for (i = 1; i < 8; i++) {
        nibble = value & 15;
        value >>= 4;
        if (digit != i || (angle_blink_count & BLINK_BIT)) {
            njPrintH(NJM_LOCATION(x - i + 8, y), nibble, 1);
        }
    }
    if (digit != i || (angle_blink_count & BLINK_BIT)) {
        njPrintH(NJM_LOCATION(x, y), value, 1);
    }
}

/* print the value at x, y as %08.2f. digit counts out from the point, 1 the
 * ones and -1 the tenths, and blinks as print_angle's does. the blank lands
 * on that digit only while five characters stand before the point. a digit
 * of 0 blinks none */
static void print_float(const Float *value, int x, int y, int digit)
{
    char text[20];
    /* @hack spelling: float_blink_count through a pointer, which keeps its
     * address in a saved register across sprintf (MOV.L R13,@-R15 at
     * 0x8c0456b8); by name, SHC loads it again after the call, the function
     * runs 2 bytes short and 31 of 46 words differ */
    Sint16 *blink = &float_blink_count;

    (*blink)++;
    sprintf(text, "%08.2f", *value);
    if (digit != 0 && (*blink & BLINK_BIT)) {
        *(text + 5 - digit) = ' ';
    }
    njPrintC(NJM_LOCATION(x, y), text);
}

/* put every slot after slot 0 in a row away from the camera, slot i
 * multi_spacing * i from the cursor. a slot stands multi_height above the
 * highest surface found there that is above the cursor's height less 100.
 * where none is, it stands multi_height above the cursor */
static void multi_line_up(struct task_work *work)
{
    NJS_POINT3 pos;
    struct line_query query;
    Float best;
    struct world_object *object;
    Sint16 i;
    Sint16 j;

    for (i = 1; i < slot_count; i++) {
        if (slot_tasks[i] != NULL && slot_tasks[i]->work != NULL) {
            pos = work->pos;
            pos.x -= multi_spacing * i * njSin(camera_work->ang.y);
            pos.z -= multi_spacing * i * njCos(camera_work->ang.y);
            best = work->pos.y - 100.0F;
            query.pos = pos;
            world_list_near(pos.x, pos.y, pos.z, 100.0F);
            object = world_objects;
            for (j = 0; j < world_count; j++) {
                if (polygon_find_nearest_height(&query, object->object,
                                                object->flags)) {
                    if (query.level > best) {
                        best = query.level;
                        pos.y = best;
                    }
                }
                /* @hack spelling: object = object + 1, which keeps work in
                 * R12 (MOV R4,R12 at 0x8c04576a); as ++, work takes R13 and
                 * 10 of 166 words differ */
                object = object + 1;
            }
            slot_tasks[i]->work->pos.x = pos.x;
            slot_tasks[i]->work->pos.y = pos.y + multi_height;
            slot_tasks[i]->work->pos.z = pos.z;
        }
    }
}

void editor_create(void)
{
    task_create(TASK_WORK, 1, editor_step);
}

int editor_request_open(void)
{
    if (editor_work == NULL) {
        return -1;
    }
    switch (editor_work->mode) {
    case EDITOR_CLOSED:
        editor_work->mode = EDITOR_OPENING;
        break;
    case EDITOR_CLOSING:
        return -1;
    }
    return 0;
}
