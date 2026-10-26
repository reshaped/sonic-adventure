/* BUTTERFLY, a butterfly that flaps and wanders around its place */
#include <ninja.h>
#include <sg_xpt.h>
#include <stdlib.h>

#include "al_race/butterfly.h"
#include "al_race/texlists.h"
#include "debug/editor.h"
#include "draw/tree.h"
#include "object/placement.h"
#include "system/frame.h"
#include "system/task.h"
#include "system/util.h"

/* a wing, which a butterfly draws four times, two to a pair */
extern NJS_OBJECT dat_0cbc724c;

/* a butterfly's state, in its task's extra. flap, bob, wander_x and wander_z
 * are phases, as angles */
struct butterfly {
    /* from its place: the wander along x and z, and the bob along y */
    NJS_VECTOR offset;
    NJS_VECTOR unk_0c;
    Angle flap;
    Angle bob;
    Angle wander_x;
    Angle wander_z;
    Angle speed_x; /* what wander_x advances a step */
    Angle speed_z; /* what wander_z advances a step */
};

/* the set entry's scale, whose rows the set editor labels. scl.x is
 * how far the butterfly wanders from its place. scl.y is the most a wander
 * phase advances a step, in degrees */
#define RANGE(work) ((work)->scl.x)
#define TURN_MAX(work) ((work)->scl.y)

/* the flap's phase step, an Angle, and the wings' widest angle, in degrees.
 * then the bob's phase step, an Angle, and its height */
#define FLAP_SPEED 0xeee
#define FLAP_OPEN 75.0F
#define BOB_SPEED 0xb60
#define BOB_HEIGHT 1.15F

/* distance of the second pair of wings behind the first */
#define PAIR_GAP 0.18F

static void butterfly_start(struct task *task);
static void butterfly_destroy(struct task *task);
static void butterfly_draw_range(NJS_POINT3 *pos, NJS_VECTOR *offset);
static void butterfly_draw_wings(struct task *task, NJS_VECTOR *offset,
                                 Angle spread);
static void butterfly_draw(struct task *task);
static void butterfly_flap(struct task *task);
static void butterfly_wander(struct task *task);
static void butterfly_step(struct task *task);

/* a wing's root, from its pair's center */
static const NJS_VECTOR wing_root = {0.38F, 0.0F, -0.38F};

void butterflies_init(void)
{
}

void butterfly_init(struct task *task)
{
    if (placement_editing(task)) {
        if (frame_step == 0) {
            njPrintC(EDITOR_LABEL(EDITOR_ROW_SCL_X), "Move Range");
            njPrintC(EDITOR_LABEL(EDITOR_ROW_SCL_Y), "Move Ang Spd Max");
        }
        butterfly_start(task);
        butterfly_step(task);
    } else {
        butterfly_start(task);
        task->step = butterfly_step;
        task->draw = butterfly_draw;
        task->destructor = butterfly_destroy;
    }
}

static void butterfly_start(struct task *task)
{
    struct butterfly *butterfly;

    if (task->extra == NULL) {
        butterfly = heap_alloc(sizeof(*butterfly));
        task->extra = butterfly;
    } else {
        butterfly = task->extra;
    }
    butterfly->offset.x = butterfly->offset.y = butterfly->offset.z = 0.0F;
    butterfly->unk_0c.x = butterfly->unk_0c.y = butterfly->unk_0c.z = 0.0F;
    butterfly->flap = RANDOM_ANGLE();
    butterfly->bob = RANDOM_ANGLE();
}

static void butterfly_destroy(struct task *task)
{
}

static void butterfly_draw_range(NJS_POINT3 *pos, NJS_VECTOR *offset)
{
    NJS_POINT3COL line;
    NJS_POINT3 points[2];
    NJS_COLOR colors[2];

    line.p = points;
    line.col = colors;
    line.tex = NULL;
    line.num = 2;
    points[0].x = pos->x;
    points[0].y = pos->y;
    points[0].z = pos->z;
    points[1].x = pos->x + offset->x;
    points[1].y = pos->y + offset->y;
    points[1].z = pos->z + offset->z;
    colors[0].color = colors[1].color = 0xffff0000;
    njDrawLine3D(&line, 1, NJD_DRAW_NORMAL);
}

/* draw a pair of wings at offset from the butterfly, flapped by work->ang.z
 * and opened by spread about y */
static void butterfly_draw_wings(struct task *task, NJS_VECTOR *offset,
                                 Angle spread)
{
    struct task_work *work = task->work;
    struct butterfly *butterfly = task->extra;
    Angle turn;

    njPushMatrix(NULL);
    njTranslateV(NULL, offset);
    njTranslateV(NULL, &work->pos);
    njTranslateV(NULL, &butterfly->offset);
    TURN_VIA(njRotateY, work->ang.y, turn);
    njPushMatrix(NULL);
    TURN_VIA(njRotateX, work->ang.z, turn);
    TURN_VIA(njRotateY, spread + 0x2000, turn);
    njTranslateV(NULL, (NJS_VECTOR *)&wing_root);
    draw_tree(&dat_0cbc724c);
    njPopMatrix(1);
    TURN_VIA(njRotateX, -work->ang.z, turn);
    TURN_VIA(njRotateY, 0xa000 - spread, turn);
    njTranslateV(NULL, (NJS_VECTOR *)&wing_root);
    draw_tree(&dat_0cbc724c);
    njPopMatrix(1);
}

static void butterfly_draw(struct task *task)
{
    if (frame_step == 0) {
        struct task_work *work = task->work;
        struct butterfly *butterfly = task->extra;
        NJS_VECTOR offset;
        Float sin;
        Float cos;

        if (placement_editing(task)) {
            butterfly_draw_range(&work->pos, &butterfly->offset);
        }
        draw_control_save();
        draw_attr_save();
        njControl3D(NJD_CONTROL_3D_CONSTANT_ATTR);
        njSetConstantAttr(0xffffffff, NJD_FLAG_IGNORE_LIGHT);
        njSetTexture(&dat_0cbc089c);
        /* @hack spelling: offset cleared through &offset, whose address SHC
         * works out again for each store (0x0cb9540c to 0x0cb95428), as the
         * image does. as offset.x, offset.y and offset.z, the code after the
         * function starts 32 bytes earlier, and 69 of 97 words differ */
        (&offset)->x = (&offset)->y = (&offset)->z = 0.0F;
        butterfly_draw_wings(task, &offset, 0x71c);
        _builtin_fsca(work->ang.y, &sin, &cos);
        offset.x += PAIR_GAP * cos;
        offset.z += -(PAIR_GAP * sin);
        butterfly_draw_wings(task, &offset, -0xaaa);
        draw_attr_restore();
        draw_control_restore();
    }
}

/* flap the wings and bob the body. the wings swing from FLAP_OPEN degrees
 * to -FLAP_OPEN and back, 9 steps each way, by the cosine of the flap's
 * phase. the body bobs BOB_HEIGHT either side of its place, by the cosine
 * of the bob's phase */
static void butterfly_flap(struct task *task)
{
    struct task_work *work = task->work;
    struct butterfly *butterfly = task->extra;
    Angle flap = butterfly->flap & 0xffff;
    Float sin;
    Float cos;

    if (flap >= 0x8000) {
        flap += FLAP_SPEED;
        if (flap > 0x10000) {
            flap = 0;
        }
    } else {
        flap += FLAP_SPEED;
        if (flap > 0x8000) {
            flap = 0x8000;
        }
    }
    butterfly->flap = flap;
    _builtin_fsca(flap, &sin, &cos);
    work->ang.z = cos * (FLAP_OPEN * ANGLE_PER_DEGREE);
    butterfly->bob += BOB_SPEED;
    _builtin_fsca(butterfly->bob, &sin, &cos);
    butterfly->offset.y = cos * BOB_HEIGHT;
}

static void butterfly_wander(struct task *task)
{
    struct task_work *work = task->work;
    struct butterfly *butterfly = task->extra;
    Float sin;
    Float cos;
    Float limit_x;
    Float limit_z;
    /* @hack redundant: keep_x and keep_z, which nothing reads. taking the
     * limits' addresses keeps limit_x at @(12,R15) and limit_z at @(8,R15)
     * (0x0cb9560a, 0x0cb95644), as the image does. without them, the code after
     * the function starts 32 bytes earlier, and 104 of 148 words differ */
    Float *keep_x = &limit_x;
    Float *keep_z = &limit_z;

    butterfly->speed_x += ((rand() * RAND_SCALE) - 0.5F) * 910.0F;
    butterfly->speed_z += ((rand() * RAND_SCALE) - 0.5F) * 910.0F;
    {
        Angle speed = butterfly->speed_x;
        Angle lower;

        /* @hack spelling: speed's low 16 bits sign-extended by hand, which SHC
         * tests and ORs (TST R5,R3 at 0x0cb955de), as the image does. as
         * (Sint16)speed, SHC writes EXTS.W, and 111 of 148 words differ */
        speed = (speed & 0x8000) ? speed | 0xffff0000 : speed & 0xffff;
        /* @hack redundant: a block that never runs, which stores 1/360 and
         * 65536.0. it gives the two constants the image's registers
         * (0x0cb955ee to 0x0cb955fc). without it, FR4, FR5 and FR6 rotate,
         * and 14 of 148 words differ */
        if (0) {
            sin = 1.0F / 360.0F;
            cos = 65536.0F;
        }
        /* NOLINTBEGIN(readability-avoid-nested-conditional-operator) */
        speed = (speed < (lower = -((limit_x = TURN_MAX(work) * 65536.0F) *
                                    (1.0F / 360.0F))))
                    ? lower
                    : ((speed > (Angle)(limit_x * (1.0F / 360.0F)))
                           ? (Angle)(limit_x * (1.0F / 360.0F))
                           : speed);
        /* NOLINTEND(readability-avoid-nested-conditional-operator) */
        butterfly->speed_x = speed;
    }
    {
        Angle speed = butterfly->speed_z;
        Angle lower;

        /* @hack spelling: speed's low 16 bits sign-extended by hand, which SHC
         * tests and ORs (TST R4,R5 at 0x0cb95620), as the image does. as
         * (Sint16)speed, SHC writes EXTS.W, and 109 of 148 words differ */
        speed = (speed & 0x8000) ? speed | 0xffff0000 : speed & 0xffff;
        /* NOLINTBEGIN(readability-avoid-nested-conditional-operator) */
        speed = (speed < (lower = -((limit_z = TURN_MAX(work) * 65536.0F) *
                                    (1.0F / 360.0F))))
                    ? lower
                    : ((speed > (Angle)(limit_z * (1.0F / 360.0F)))
                           ? (Angle)(limit_z * (1.0F / 360.0F))
                           : speed);
        /* NOLINTEND(readability-avoid-nested-conditional-operator) */
        butterfly->speed_z = speed;
    }
    butterfly->wander_x += butterfly->speed_x;
    butterfly->wander_z += butterfly->speed_z;
    _builtin_fsca(butterfly->wander_x, &sin, &cos);
    butterfly->offset.x = sin * RANGE(work);
    _builtin_fsca(butterfly->wander_z, &sin, &cos);
    butterfly->offset.z = sin * RANGE(work);
}

static void butterfly_step(struct task *task)
{
    if (placement_far_from_camera(task)) {
        task_kill(task);
    } else {
        butterfly_flap(task);
        butterfly_wander(task);
        butterfly_draw(task);
    }
}
