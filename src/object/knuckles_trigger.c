/* Knuckles' item trigger boxes, KN ITEM COLLI. the set editor draws and
 * labels each box. in the game a box does nothing: it ends for good once
 * player 0 is gone, and leaves once it is out of range */
#include <ninja.h>
#include <sg_xpt.h>

#include "debug/editor.h"
#include "draw/draw.h"
#include "object/knuckles_trigger.h"
#include "object/placement.h"
#include "player/player.h"
#include "system/frame.h"
#include "system/task.h"
#include "system/util.h"

/* a box's type, from the low byte of its ang.x. any value past
 * TRIGGER_RINGSPR is TRIGGER_BAD */
enum knuckles_trigger_type { TRIGGER_NOITEM, TRIGGER_RINGSPR, TRIGGER_BAD };

/* a box keeps its type in its work's timer */
#define TYPE(work) TASK_TIMER(work)

struct knuckles_trigger_vars {
    /* whether the box works once: 1 when bits 8 to 11 of ang.x aren't all 0,
     * else 0. only the set editor's label reads it */
    Uint8 one_shot;
};

#define VARS(work) ((struct knuckles_trigger_vars *)&(work)->vars)

#define SCALE_MAX 40.0F

/* masks of work->tick, which the draw and the labels each advance once a
 * frame. the name shows while NAME_BLINK's bit is set, for 8 frames in 16.
 * the box takes its first color while COLOR_BLINK's bits are clear, for 4
 * frames in 16 */
#define NAME_BLINK 0x10
#define COLOR_BLINK 0x18

/* each type's two colors, as red, green and blue. the box blinks between
 * them */
static const Float knuckles_trigger_colors[] = {
    1.0F, 0.6F, 0.6F, 1.0F, 0.0F, 0.0F, /* R_NOITEM: pink, then red */
    1.0F, 0.6F, 0.6F, 1.0F, 1.0F, 0.0F, /* O_RINGSPR: pink, then yellow */
    0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, /* bad: black, then black */
};

/* each type's name, and the label for each value of one_shot, as the set
 * editor shows them */
static const char *knuckles_trigger_names[] = {"R_NOITEM", "O_RINGSPR",
                                               "!!! BAD !!!"};
static const char *knuckles_trigger_shots[] = {"(REGULAR)", "(ONE SHOT)"};

static void knuckles_trigger_draw(struct task *task)
{
    struct task_work *work = task->work;
    int index;
    const Float *color;

    if (frame_step != 0) {
        return;
    }
    index = TYPE(work) * 6;
    if (++work->tick & COLOR_BLINK) {
        index += 3;
    }
    color = knuckles_trigger_colors + index;
    draw_set_material(0.01F, color[0], color[1], color[2]);
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    TURN(njRotateY, work->ang.y);
    njScaleV(NULL, &work->scl);
    njSimpleDrawObject(&marker_box_object);
    njPopMatrix(1);
    draw_set_material(0.0F, 0.0F, 0.0F, 0.0F);
}

static void knuckles_trigger_print_labels(struct task *task)
{
    struct task_work *work = task->work;

    if (frame_step != 0) {
        return;
    }
    work->tick++;
    if (placement_editing(task)) {
        if (work->tick & NAME_BLINK) {
            njPrintC(NJM_LOCATION(8, 8), "<KN ITEM COLLI>");
        } else {
            njPrintC(NJM_LOCATION(8, 8), "<             >");
        }
        njPrintC(EDITOR_LABEL(EDITOR_ROW_ANG_X), "-> TYPE ");
        njPrintC(NJM_LOCATION(25, EDITOR_ROW_ANG_X),
                 knuckles_trigger_names[TYPE(work)]);
        njPrintC(NJM_LOCATION(37, EDITOR_ROW_ANG_X),
                 knuckles_trigger_shots[VARS(work)->one_shot]);
        njPrintC(EDITOR_LABEL(EDITOR_ROW_ANG_Y), "-> ROT Y");
        njPrintC(EDITOR_LABEL(EDITOR_ROW_ANG_Z), "->      ");
        njPrintC(EDITOR_LABEL(EDITOR_ROW_SCL_X), "-> SCL X");
        njPrintC(EDITOR_LABEL(EDITOR_ROW_SCL_Y), "-> SCL Y");
        njPrintC(EDITOR_LABEL(EDITOR_ROW_SCL_Z), "-> SCL Z");
    }
}

/* hold each axis of the box's scale from 1 to SCALE_MAX, and take its type
 * and one_shot from ang.x */
static void knuckles_trigger_take_settings(struct task_work *work)
{
    if (work->scl.x < 1.0F) {
        work->scl.x = 1.0F;
    } else if (work->scl.x > SCALE_MAX) {
        work->scl.x = SCALE_MAX;
    }
    if (work->scl.y < 1.0F) {
        work->scl.y = 1.0F;
    } else if (work->scl.y > SCALE_MAX) {
        work->scl.y = SCALE_MAX;
    }
    if (work->scl.z < 1.0F) {
        work->scl.z = 1.0F;
    } else if (work->scl.z > SCALE_MAX) {
        work->scl.z = SCALE_MAX;
    }
    /* @hack spelling: the type assigned in the test, which compares the R0 it
     * stores (MOV.W R0,@(6,R4) at 0x8c0a89fe, EXTU.W R0,R0 at 0x8c0a8a00);
     * assigned apart, SHC loads it back first and 19 of 53 words differ */
    if ((TYPE(work) = (Uint8)work->ang.x) >= TRIGGER_BAD) {
        TYPE(work) = TRIGGER_BAD;
    }
    /* @hack spelling: one_shot assigned in the test, as the type is
     * (0x8c0a8a16, 0x8c0a8a18); assigned apart, 7 of 53 words differ */
    if ((VARS(work)->one_shot = (work->ang.x & 0xf00) >> 8) > 1) {
        VARS(work)->one_shot = 1;
    }
}

/* take the box's settings. in edit mode, draw and label the box and do no
 * more. with no player 0 the box ends for good, and out of range it
 * leaves */
static void knuckles_trigger_step(struct task *task)
{
    struct task_work *work = task->work;
    struct task_work *player = player_work[0];
    /* @hack redundant: two points whose address is stored and never read,
     * which give the image's 36-byte frame (ADD #-28,R15 at 0x8c0a8a2a);
     * without them, the frame is 12 bytes and 23 of 40 words differ */
    NJS_POINT3 unused[2];
    void *taken = unused;

    knuckles_trigger_take_settings(work);
    if (edit_mode) {
        knuckles_trigger_draw(task);
        knuckles_trigger_print_labels(task);
        return;
    }
    if (player == NULL) {
        placement_kill(task);
        return;
    }
    /* @hack redundant: a return where the function ends, which keeps the
     * image's TST R0,R0 of the result (0x8c0a8a9a); without it, 5 of 40 words
     * differ */
    if (placement_try_leave(task)) {
        return;
    }
}

void knuckles_trigger_start(struct task *task)
{
    knuckles_trigger_take_settings(task->work);
    knuckles_trigger_step(task);
    task->step = knuckles_trigger_step;
}
