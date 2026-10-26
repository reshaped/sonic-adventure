/* Zero's trigger boxes, which the set editor names EROB COLLI. Amy or Zero
 * coming into one sets it off. a box ends Zero or sends Zero off, brings
 * Zero in, blinds Zero to Amy, or pushes Zero back out */
#include <ninja.h>
#include <sg_xpt.h>

#include "debug/editor.h"
#include "draw/draw.h"
#include "enemy/zero.h"
#include "enemy/zero_handle.h"
#include "object/placement.h"
#include "object/zero_trigger.h"
#include "player/amy.h"
#include "player/player.h"
#include "system/frame.h"
#include "system/task.h"
#include "system/util.h"

/* what a box does to Zero, from the low byte of its ang.x. Amy coming
 * into the box sets off TRIGGER_KILL to TRIGGER_BLIND, and Zero coming in
 * sets off the next three. TRIGGER_ZERO_KILL and TRIGGER_ZERO_OUT do what
 * TRIGGER_KILL and TRIGGER_OUT do. any higher byte is TRIGGER_BAD, which
 * does nothing */
enum zero_trigger_type {
    TRIGGER_KILL, /* Zero ends */
    TRIGGER_OUT, /* Zero rises away, and goes */
    TRIGGER_APPEAR, /* Zero comes in, at the box's offset or above Amy */
    TRIGGER_BLIND, /* Zero loses sight of Amy, and she's told she's hidden */
    TRIGGER_ZERO_KILL,
    TRIGGER_ZERO_OUT,
    TRIGGER_NO_ENTRY, /* Zero is pushed back out */
    TRIGGER_BAD
};

/* a box keeps its type in its timer */
#define TYPE(work) TASK_TIMER(work)

/* a box reaches scl times HALF_BOX from its middle along each axis: half
 * marker_box_object, which is ten across */
#define HALF_BOX 5.0F
#define SCALE_MAX 40.0F

/* how far past a box's sides Amy and Zero still touch it */
#define AMY_REACH (0.71F * HALF_BOX)
#define ZERO_REACH (1.42F * HALF_BOX)
#define AMY_MIDDLE 5.0F /* where a box finds Amy, this far above her feet */

/* an appearing box's ang.z, which the set editor labels 0xAFH0. OFFSET_MASK
 * keeps its top three nibbles, A, F and H, which place Zero round the box.
 * OFFSET_ANG keeps A where it stands, an angle of A sixteenths of a turn.
 * Zero comes in F times OFFSET_STEP away at that angle, and H times
 * OFFSET_STEP up */
#define OFFSET_MASK 0xfff0
#define OFFSET_ANG(z) ((z) & 0xf000)
#define OFFSET_FAR(z) (((z) & 0xf00) >> 8)
#define OFFSET_UP(z) (((z) & 0xf0) >> 4)
#define OFFSET_STEP 10.0F
#define ABOVE_AMY 50.0F /* with no offset, Zero comes in this far above Amy */

/* masks of work->tick, which the draw and the labels each count up once a
 * frame. the name shows while NAME_BLINK's bit is set, 8 frames in 16. the
 * box shows who sets it off while COLOR_BLINK's bits are clear, 4 frames
 * in 16, and its type's own color the rest */
#define NAME_BLINK 0x10
#define COLOR_BLINK 0x18

/* @hack type: an appearing box's zero_start without its target, which
 * ZERO_STAND ignores. zero_trigger_step then takes 52 bytes for its locals
 * (ADD #-52,R15 at 0x8c0a83dc), with start at R15+12; as a struct
 * zero_start, it takes 12 more and 28 of 412 words differ */
struct appear_start {
    Sint32 how;
    NJS_POINT3 pos;
};

/* each type's two colors: Amy's pink or Zero's green, then its own */
static const Float zero_trigger_colors[] = {
    1.0F, 0.6F, 0.6F, 1.0F, 0.0F, 0.0F, /* kill: red */
    1.0F, 0.6F, 0.6F, 1.0F, 1.0F, 0.0F, /* out: yellow */
    1.0F, 0.6F, 0.6F, 0.0F, 0.0F, 1.0F, /* appear: blue */
    1.0F, 0.6F, 0.6F, 0.0F, 0.0F, 0.0F, /* blind: black */
    0.0F, 1.0F, 0.0F, 1.0F, 0.0F, 0.0F, /* Zero kill: red */
    0.0F, 1.0F, 0.0F, 1.0F, 1.0F, 0.0F, /* Zero out: yellow */
    0.0F, 1.0F, 0.0F, 1.0F, 0.5F, 0.0F, /* no entry: orange */
    0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, /* bad: both black */
};

static const char *zero_trigger_names[] = {
    "(A>E_KILL)", "(A>E_OUT)", "(A>E_APPER)", "(A>E_BLIND)",
    "(E>E_KILL)", "(E>E_OUT)", "(E_NOENTRY)", "!!! BAD !!!",
};

/* the line from an appearing box to where Zero comes in */
static NJS_COLOR zero_trigger_line_colors[2] = {
#include "object/zero_trigger/zero_trigger_line_colors.inc"
};
static NJS_POINT3 zero_trigger_line_points[2];
static NJS_POINT3COL zero_trigger_line = {
#include "object/zero_trigger/zero_trigger_line.inc"
};

/* draw the box in its type's colors, on a frame's first step. an appearing
 * box with an offset adds a line to where Zero comes in, and a small box
 * there */
static void zero_trigger_draw(struct task *task)
{
    struct task_work *work = task->work;
    int index;
    const Float *color;
    NJS_POINT3 to;

    if (frame_step != 0) {
        return;
    }
    index = TYPE(work) * 6;
    if (++work->tick & COLOR_BLINK) {
        index += 3;
    }
    color = zero_trigger_colors + index;
    draw_set_material(0.01F, color[0], color[1], color[2]);
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    TURN(njRotateY, work->ang.y);
    njScaleV(NULL, &work->scl);
    njSimpleDrawObject(&marker_box_object);
    njPopMatrix(1);
    if (TYPE(work) == TRIGGER_APPEAR && (work->ang.z & OFFSET_MASK)) {
        to = work->pos;
        to.x += OFFSET_FAR(work->ang.z) * njCos(OFFSET_ANG(work->ang.z)) *
                OFFSET_STEP;
        to.z += OFFSET_FAR(work->ang.z) * njSin(OFFSET_ANG(work->ang.z)) *
                OFFSET_STEP;
        to.y += OFFSET_UP(work->ang.z) * OFFSET_STEP;
        zero_trigger_line_points[0] = work->pos;
        zero_trigger_line_points[1] = to;
        njDrawLine3D(&zero_trigger_line, 1, 0);
        njPushMatrix(NULL);
        njTranslateV(NULL, &to);
        njScale(NULL, 0.3F, 0.3F, 0.3F);
        njSimpleDrawObject(&marker_box_object);
        njPopMatrix(1);
    }
    draw_set_material(0.0F, 0.0F, 0.0F, 0.0F);
}

/* label the set editor's rows for the box it holds, on a frame's first
 * step */
static void zero_trigger_print_labels(struct task *task)
{
    struct task_work *work = task->work;

    if (frame_step != 0) {
        return;
    }
    work->tick++;
    if (placement_editing(task)) {
        if (work->tick & NAME_BLINK) {
            njPrintC(NJM_LOCATION(8, 8), "<EROB COLLI>");
        } else {
            njPrintC(NJM_LOCATION(8, 8), "<          >");
        }
        njPrintC(EDITOR_LABEL(EDITOR_ROW_ANG_X), "-> TYPE ");
        njPrintC(NJM_LOCATION(25, EDITOR_ROW_ANG_X),
                 zero_trigger_names[TYPE(work)]);
        njPrintC(EDITOR_LABEL(EDITOR_ROW_ANG_Y), "-> ROT Y");
        njPrintC(EDITOR_LABEL(EDITOR_ROW_ANG_Z), TYPE(work) == TRIGGER_APPEAR
                                                     ? "-> OFFSET(0xAFH0)"
                                                     : "->      ");
        njPrintC(EDITOR_LABEL(EDITOR_ROW_SCL_X), "-> SCL X");
        njPrintC(EDITOR_LABEL(EDITOR_ROW_SCL_Y), "-> SCL Y");
        njPrintC(EDITOR_LABEL(EDITOR_ROW_SCL_Z), "-> SCL Z");
    }
}

/* take the box's settings from its set entry: each scale held from 1 to
 * SCALE_MAX, and the type from the low byte of ang.x. it clears ang.z's low
 * nibble, which the offset leaves out */
static void zero_trigger_take_settings(struct task_work *work)
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
     * stores (MOV.W R0,@(6,R4) at 0x8c0a83ac, EXTU.W R0,R0 at 0x8c0a83ae);
     * assigned apart, SHC loads the type back from the work first, the
     * function runs 2 bytes long and 10 of 44 words differ */
    if ((TYPE(work) = (Uint8)work->ang.x) >= TRIGGER_BAD) {
        TYPE(work) = TRIGGER_BAD;
    }
    work->ang.z &= OFFSET_MASK;
}

/* a box that has done its work waits to leave range */
static void zero_trigger_done(struct task *task)
{
    placement_try_leave(task);
}

/* while Amy, or Zero for Zero's types, is in the box, do what its type
 * says. while the set editor is open, the box draws and labels itself
 * instead. with no player 0, the box ends for good */
static void zero_trigger_step(struct task *task)
{
    struct task_work *work = task->work;
    struct task_work *amy = player_work[0];
    NJS_VECTOR push;
    NJS_POINT3 at;
    struct appear_start start;
    Float reach;
    Float sine;
    Float cosine;
    Float x;
    Float z;
    Float dx;
    Float dy;
    Float dz;
    Float push_x;
    Float push_z;
    Float *out;
    Float *scale;
    Float *along;
    Float *other_a;
    Float *other_b;

    zero_trigger_take_settings(work);
    if (edit_mode) {
        zero_trigger_draw(task);
        zero_trigger_print_labels(task);
        return;
    }
    if (amy == NULL) {
        placement_kill(task);
        return;
    }
    if (placement_try_leave(task)) {
        return;
    }
    switch (TYPE(work)) {
    case TRIGGER_ZERO_KILL:
    case TRIGGER_ZERO_OUT:
    case TRIGGER_NO_ENTRY:
        if (!zero_get_pos(&at)) {
            return;
        }
        reach = ZERO_REACH;
        break;
    default:
        at = amy->pos;
        at.y += AMY_MIDDLE;
        reach = AMY_REACH;
        break;
    }
    /* where Amy or Zero is from the box's middle, in the box's frame */
    njSubVector(&at, &work->pos);
    if (njAbs(at.y) > (work->scl.y * HALF_BOX) + reach) {
        return;
    }
    sine = njSin(work->ang.y);
    cosine = njCos(work->ang.y);
    x = (at.x * cosine) - (at.z * sine);
    z = (at.x * sine) + (at.z * cosine);
    if (njAbs(x) > (work->scl.x * HALF_BOX) + reach) {
        return;
    }
    if (njAbs(z) > (work->scl.z * HALF_BOX) + reach) {
        return;
    }
    /* push Zero back out along the axis it's least far into the box on,
     * then turn the push from the box's frame into the world's */
    if (TYPE(work) == TRIGGER_NO_ENTRY) {
        dx = (work->scl.x * HALF_BOX) + reach - njAbs(x);
        dy = (work->scl.y * HALF_BOX) + reach - njAbs(at.y);
        dz = (work->scl.z * HALF_BOX) + reach - njAbs(z);
        if (dx < dy && dx < dz) {
            out = &push.x;
            scale = &work->scl.x;
            along = &x;
            other_a = &push.y;
            other_b = &push.z;
        } else if (dz < dx && dz < dy) {
            out = &push.z;
            scale = &work->scl.z;
            along = &z;
            other_a = &push.x;
            other_b = &push.y;
        } else {
            out = &push.y;
            scale = &work->scl.y;
            along = &at.y;
            other_a = &push.x;
            other_b = &push.z;
        }
        *other_a = *other_b = 0.0F;
        if (*along > 0.0F) {
            *out = (*scale * HALF_BOX) - *along + reach;
        } else {
            *out = (-*scale * HALF_BOX) - *along - reach;
        }
        sine = njSin(work->ang.y);
        cosine = njCos(work->ang.y);
        push_x = push.x;
        push_z = push.z;
        push.x = (push_x * cosine) + (push_z * sine);
        push.z = (push_z * cosine) - (push_x * sine);
    }
    switch (TYPE(work)) {
    case TRIGGER_KILL:
    case TRIGGER_ZERO_KILL:
        /* @hack spelling: zero_kill's result compared with TRUE, CMP/EQ #1
         * (0x8c0a8634). zero_kill returns only TRUE or FALSE; as a plain
         * test, TST, and 2 of 412 words differ */
        /* @hack redundant: an else that breaks, as TRIGGER_APPEAR's does, which
         * has this case branch to that case's store of zero_trigger_done
         * (BRA 0x8c0a8722 at 0x8c0a863c); without it, the function runs 2
         * bytes long and 145 of 412 words differ */
        if (zero_kill() == TRUE) {
            task->step = zero_trigger_done;
        } else {
            break;
        }
        break;
    case TRIGGER_OUT:
    case TRIGGER_ZERO_OUT:
        /* @hack spelling: zero_leave's result compared with TRUE, CMP/EQ #1
         * (0x8c0a8666). zero_leave returns only TRUE or FALSE; as a plain
         * test, TST, and 17 of 412 words differ */
        if (zero_leave() == TRUE) {
            task->step = zero_trigger_done;
        }
        break;
    case TRIGGER_APPEAR:
        start.how = ZERO_STAND;
        if (work->ang.z & OFFSET_MASK) {
            start.pos = work->pos;
            start.pos.x += OFFSET_FAR(work->ang.z) *
                           njCos(OFFSET_ANG(work->ang.z)) * OFFSET_STEP;
            start.pos.z += OFFSET_FAR(work->ang.z) *
                           njSin(OFFSET_ANG(work->ang.z)) * OFFSET_STEP;
            start.pos.y += OFFSET_UP(work->ang.z) * OFFSET_STEP;
        } else {
            start.pos = amy->pos;
            start.pos.y += ABOVE_AMY;
        }
        /* @hack spelling: zero_appear's result compared with TRUE, CMP/EQ
         * #1 (0x8c0a871e). zero_appear returns only TRUE or FALSE; as a plain
         * test, TST, and 2 of 412 words differ */
        /* @hack redundant: an else that breaks, as TRIGGER_KILL's does, which
         * lets that case share this one's store of zero_trigger_done
         * (0x8c0a8722 to 0x8c0a8726); without it, the function runs 2 bytes
         * long and 145 of 412 words differ */
        if (zero_appear((struct zero_start *)&start) == TRUE) {
            task->step = zero_trigger_done;
        } else {
            break;
        }
        break;
    case TRIGGER_BLIND:
        zero_blind();
        amy_call_hidden();
        break;
    case TRIGGER_NO_ENTRY:
        zero_push(&push);
        break;
    }
}

void zero_trigger_start(struct task *task)
{
    zero_trigger_take_settings(task->work);
    zero_trigger_step(task);
    task->step = zero_trigger_step;
}
